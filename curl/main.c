/****************************************************************
file:         main.c
description:  the source file of shell client implemention
date:         2020/1/6
author        wangzhiwei
****************************************************************/
#include <stdio.h>
#include <sys/io.h> 
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <time.h> 
#include "curl.h"
#include <sys/stat.h> 
#include <dirent.h>
#include <sys/inotify.h>  
#include "curl_commshm.h"

int sendPostFile(char *name);
unsigned long get_file_size(char *name);

static char file_path[30]="/media/sdcard/fileUL/";
static char vin[18] = {0};
static char tboxsn[19] = {0};

unsigned long get_file_size(char *name)  
{  
	unsigned long filesize = -1;	  
	struct stat statbuff;
	
	if(stat(name, &statbuff) < 0)
	{  
		return filesize;  
	}
	else
	{	
		filesize = statbuff.st_size;  
	}  
	return filesize;  
}  

int sendPostFile(char *name)
{
  	CURL *curl;
  	CURLcode res;
  	struct curl_httppost *formpost=NULL;
  	struct curl_httppost *lastptr=NULL;
  	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
 	char filesize[15]={0};
	
  	sprintf(filesize, "%lu", get_file_size(name));
  	
  	curl_global_init(CURL_GLOBAL_ALL);
   
    /*******************读取文件*************************/
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_FILE, name,			   
               	CURLFORM_CONTENTTYPE, "application/octet-stream", 
               	CURLFORM_END);
	
   /********************配置参数type*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "type",  
   				CURLFORM_COPYCONTENTS, "TBOX",   //这个固定写死
   				CURLFORM_END);
   
    /********************配置参数deviceId****************/
    curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "deviceId",  
   				CURLFORM_COPYCONTENTS, tboxsn, 
   				CURLFORM_END);
	
	/********************配置参数uuid*******************/
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "uuid",  
   				CURLFORM_COPYCONTENTS, vin, 
   				CURLFORM_END);

	/********************配置参数fileName****************/
	char timestamp[11] = {0};
	strncpy(timestamp,name+sizeof("/media/sdcard/fileUL/LUZAGAAEP30A00701_")-1,10);
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "fileName",  
				CURLFORM_COPYCONTENTS, timestamp, 
   				CURLFORM_END);
	
	/********************配置参数filesize****************/
  	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "filesize",
               	CURLFORM_COPYCONTENTS, filesize,
                CURLFORM_END); 

   	curl = curl_easy_init();

  	headerlist = curl_slist_append(headerlist, buf);
	
  	if(curl) 
	{
    	/* what URL that receives this POST */ 
    	curl_easy_setopt(curl, CURLOPT_URL, "https://file-uat.chehezhi.cn/fileApi/1.0/pickData");
		
    	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		
    	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
		
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);//不检查证书
	
    	/* Perform the request, res will get the return code */ 
    	res = curl_easy_perform(curl);
    	/* Check for errors */ 
    	if(res != CURLE_OK)
		{
      		fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
			
	  		printf("curl url_easy_perform() failed:%s \n",curl_easy_strerror(res));
			
	  		return -1;
		}
		else
		{
			printf("\nFile uploaded %s successfully\n",name);
		}
    	/* always cleanup */ 
    	curl_easy_cleanup(curl); 
    	/* then cleanup the formpost chain */ 
    	curl_formfree(formpost);
    	/* free slist */ 
    	curl_slist_free_all (headerlist);
  	}
  	return 0;
}
void get_tbox_para(char *pt)
{
	if(pt != NULL)
	{
		strncpy(vin,pt + CMD_LENGTH,TBOX_VIN_LENGTH);  //获取VIN
		
		strncpy(tboxsn,pt+CMD_LENGTH+TBOX_VIN_LENGTH,TBOX_SN_LENGTH);//获取TBOXSN
	}
}
int main(int argc, char *argv[])
{  
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	char cmd[60] = {0};
	int shmid = 0;
	int semid = 0;
	
	shmid = CreateShm(4096);   //创建共享内存
	
	char *addr = shmat(shmid,NULL,0); //共享内存映射到本地进程

	semid = Commsem();//创建信号量
	
	set_semvalue(semid);//初始化信号量值为1

	for(;;)
	{
		sleep(15);   //延时15s
		
		sem_p(semid); //申请信号量，操作共享内存
		
		if(strncmp(addr,"upload",6) == 0) //上传文件
		{
			get_tbox_para(addr);         //得到终端的VIN、TBOXSN
			
			memset(addr,0,4096);         //清掉共享内存
			
			sem_v(semid);                //释放信号量，
			
			if((dir = opendir(file_path)) != NULL)     //打开文件夹
			{
				printf("open file_path success \n");
	
				while((ptr = readdir(dir)) != NULL)   
				{
					if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
					{
						continue;
					}
					memset(file_name,0,sizeof(file_name));
					
					memset(cmd,0,sizeof(cmd));
					
					strcat(file_name,file_path);
					
  					strcat(file_name,ptr->d_name);
					
    				if(0==sendPostFile(file_name))
					{
						printf("sendPostFile success\n"); 
						
						sprintf(cmd,"rm %s",file_name);
						
						system(cmd);
					}
					else
					{					
						printf("sendPostFile failed\n");
					}
					
					sleep(5);
				}
				
				closedir(dir);
				
				printf("closedir finish\n");
			}	
			else
			{
				
				printf("open file_path fail\n");
			}
		}
		else
		{
			sem_v(semid);
		}
	}
	return 0;
}


