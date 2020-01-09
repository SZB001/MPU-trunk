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
  	char file_name[80] = {0};
  	strcat(file_name,file_path);
  	strcat(file_name,name);
  	sprintf(filesize, "%lu", get_file_size(file_name));

  	static const char buf[] = "Expect:";

  	curl_global_init(CURL_GLOBAL_ALL);
   
   	/* Fill in the file upload field */ 
	printf("filename = %s",file_name);
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_FILE, file_name,			   
               	CURLFORM_CONTENTTYPE, "application/octet-stream", 
               	CURLFORM_END);
   
   	//printf("curl_formadd filesize\n");  
  	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "filesize",
               	CURLFORM_COPYCONTENTS, filesize,
                CURLFORM_END); 
	
  
   	//printf("curl_formadd submit\n");  
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_COPYCONTENTS, "commit",
               	CURLFORM_END);
    
   	//printf("curl curl_easy_init\n");
   	curl = curl_easy_init();
  	/* initalize custom header list (stating that Expect: 100-continue is not
     	wanted */ 
  	headerlist = curl_slist_append(headerlist, buf);
  	if(curl) {
	//printf("curl true begin post\n");
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
	int shmid = 0;
	shmid = CreateShm(4096);   //创建共享内存
	char *addr = shmat(shmid,NULL,0); //共享内存映射到本地进程
	for(;;)
	{
		usleep(20000);

		if(strcmp(addr,"upload") == 0)//上传文件
		{
			memset(addr,0,512);
			if((dir = opendir(file_path)) != NULL)
			{
				//printf("%p\n",dir);
				printf("open file_path success \n");
		
				while((ptr = readdir(dir)) != NULL)
				{
					if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
					{
						continue;
					}
					printf("%s\n",ptr->d_name);
    				if(0==sendPostFile(ptr->d_name))
					{
						printf("sendPostFile success\n"); 
						//system("cd /media/sdcard/fileUL/");
						//system("pwd");
						//system("rm ptr->d_name");
					}
					else
					{					
						printf("sendPostFile failed\n"); 
					}
				}
			}	
	
			else
			{
				printf("open file_path fail\n");
			}
		}
	}
	
	closedir(dir);
	printf("closedir finish");
	return 0;
}


