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
#include <sys/socket.h>
#include <time.h> 
#include "curl.h"
#include <sys/stat.h> 
#include <dirent.h>
#include <sys/inotify.h>  
//#include "dev_api.h"
#include "log.h"
#include "curl_commshm.h"
#include "shell_api.h"


 
static char file_path[30]="/media/sdcard/fileUL/";

union semum
{
     int                 val;
     struct semid_ds     *buf;
     unsigned short      *array;
 }arg;

int sendPostFile(char *name);

unsigned long get_file_size(char *name);
 
unsigned long get_file_size(char *name)  
{  
	unsigned long filesize = -1;	  
	struct stat statbuff;
	if(stat(name, &statbuff) < 0){  
		return filesize;  
	}else{	
		filesize = statbuff.st_size;  
	}  
	return filesize;  
}  

 
int sendPostFile(char *name){
  	CURL *curl;
  	CURLcode res;
  	struct curl_httppost *formpost=NULL;
  	struct curl_httppost *lastptr=NULL;
  	struct curl_slist *headerlist=NULL;
 	char filesize[15]={0};
  	sprintf(filesize, "%lu", get_file_size(name));

  	static const char buf[] = "Expect:";

  	curl_global_init(CURL_GLOBAL_ALL);
   

   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_FILE, name,			   
               	CURLFORM_CONTENTTYPE, "application/octet-stream", 
               	CURLFORM_END);
   

  	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "filesize",
               	CURLFORM_COPYCONTENTS, filesize,
                CURLFORM_END); 
	
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_COPYCONTENTS, "commit",
               	CURLFORM_END);
    

   	curl = curl_easy_init();

  	headerlist = curl_slist_append(headerlist, buf);
  	if(curl) {
    /* what URL that receives this POST */ 
    curl_easy_setopt(curl, CURLOPT_URL, "https://file-uat.chehezhi.cn/fileApi/1.0/pickData");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	
    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK){
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
 
int main(int argc, char *argv[])
{  
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	char cmd[60] = {0};
	int shmid = 0;
	//sem_t  mutex;

	shmid = CreateShm(4096);   //创建共享内存
	
	char *addr = shmat(shmid,NULL,0); //共享内存映射到本地进程
	for(;;)
	{
		usleep(20000);

		if(strcmp(addr,"upload") == 0)//上传文件
		{
			//sem_wait(mutex);    //请求信号量
			memset(addr,0,4096);
			//sem_post(mutex);    //释放信号量
			if((dir = opendir(file_path)) != NULL)
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
				}
				closedir(dir);
				printf("closedir finish\n");
			}	
			else
			{
				printf("open file_path fail\n");
			}
		}
	}
	return 0;
}


