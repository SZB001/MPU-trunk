/****************************************************************
file:         main.c
description:  Upload files to https using libcurl
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
#include <pthread.h>
#include "file.h"
#include "log.h"
#include <time.h> 
#include "dir.h"
#include "../app/base/minizip/zip.h"
#include "curl.h"
#include <sys/stat.h> 
#include <dirent.h>
#include <sys/inotify.h>  
#include "curl_commshm.h"

#define CAN_FILE_UPPATH "/media/sdcard/CanFileload/"
#define LOG_FILE_UPPATH "/media/sdcard/log/"



unsigned long curl_get_filesize(char *name);
static int curl_Post_GbFile(char *name);
static int curl_Post_CanFile(char *name);
static int curl_Post_LogFile(char *name);

//static void curl_CanFile_pkgzip(char *name);
static void curl_CanFile_handler(void);
static void curl_LogFile_handler(void);

static int curl_timestamp_ultoa(unsigned long value, char *string, int radix);
//static void curl_send_zipfile(void);

static char gb_file_path[30] = "/media/sdcard/fileUL/";
static char can_file_path[30] = "/media/sdcard/CanMsgFileUL/";
static char gb_memory[100] = {0};
static char can_memory[100] = {0};
static char log_memory[100] = {0};
static char vin[18] = {0};
static char tboxsn[19] = {0};
static pthread_t curl_gb_tid;
static pthread_t curl_can_tid;
static pthread_t curl_log_tid;
static short canfile_upload_time = 0;
static int before_flag = 0;
static struct timeval timestamp;
static struct timeval log_timestamp;
static unsigned int log_starttime = 0;
static unsigned short log_uptime = 0;
static char log_eventId[20] = {0};
static int eventid = 0;


/**
     * @brief    时间戳转化字符串
     * @param[in] file_name.
     * @return    unsigned long, bytes.
*/
int curl_timestamp_ultoa(unsigned long value, char *string, int radix)
{
	char tmp[33] = {0};
	char *tp = tmp;
	int i;
	
	if (radix > 36 || radix <= 1)
	{
		return -1;
	}
	
	if(value != 0)
	{
		while(value)
		{
			i = value % radix;
			value = value / radix;
			if (i < 10)
				*tp++ = i +'0';
			else
				*tp++ = i + 'a' - 10;
		}
	
		while(tp > tmp)
		{
			*string++ = *(--tp);
		}
	}
	else
	{
		*string++ = '0';
	}
	
	*string = 0;
	
	return 0;
}


/**
     * @brief   文件夹压缩打包
     * @param[in] file_name.
     * @return    unsigned long, bytes.
*/
static void curl_CanFile_handler(void)
{
	int ret;
	char file_name[80] = {0};
	char zipfile_name[80] = {0};
	char stringVal[32] = {0};
	char buf[BUFFSIZE] = {0};
	FILE *fp = NULL;
	DIR *dir = NULL;
	char cmd[200] = {0};
	struct dirent *ptr;
	int i = 0;
	
	if(dir_exists(CAN_FILE_UPPATH) == 0 &&
				dir_make_path(CAN_FILE_UPPATH, S_IRUSR | S_IWUSR, false) != 0)
	{
		printf("path not exist and creat fail");
		sleep(1);
		return;
	}
				
	curl_timestamp_ultoa(timestamp.tv_sec,stringVal,10);  //时间戳转换成字符串
	
	memset(zipfile_name,0,sizeof(zipfile_name));
	strcpy(zipfile_name,CAN_FILE_UPPATH);
	strcat(zipfile_name,vin);
	strcat(zipfile_name,"_");
	strncat(zipfile_name,stringVal,sizeof(stringVal));
	strcat(zipfile_name,".zip");    //zip文件的时间就是触发时候的时间戳
	
	// /media/sdcard/CanFileload/LUZAGAAEP30A00701_xxxxxxxxxx.zip
	if((dir = opendir(CAN_FILE_UPPATH)) != NULL)     //打开文件夹
	{
		zipFile zfile;
		zfile = zipOpen64(zipfile_name, 0);   
		if (zfile == NULL) 
		{
			printf("zipOpen64 error\n");
			sleep(1);
			return;
		}
		
		while((ptr = readdir(dir)) != NULL)   
		{
			
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
		
			memset(file_name,0,sizeof(file_name));
			
			if(strncmp(ptr->d_name+17+1+10+2,".asc",3) == 0)
			{
			
				strcpy(file_name,CAN_FILE_UPPATH);
				strcat(file_name,ptr->d_name);
				// /media/sdcard/CanFileload/LUZAGAAEP30A00701_xxxxxxxxxx_1.asc
				
				zip_fileinfo zinfo;
				
				memset(&zinfo,0,sizeof(zinfo));
				
				ret = zipOpenNewFileInZip(zfile, ptr->d_name, &zinfo, NULL,0,NULL,0,NULL, Z_DEFLATED, 1);
				if (ret != ZIP_OK) 
				{
					printf("zipOpenNewFileInZip error\n");
					sleep(1);
					return;
				}
				
				fp = fopen(file_name, "r");   //以只读的方式打开原始文件
				if (NULL == fp)    // 检查打开情况
    			{
        			printf("fopen %s failed\n",file_name);
        			return ;
    			}
				
				while(1)
				{
		
					fgets(buf,CURL_BUF_SIZE,fp);
					
					//ret = fread(buf, 1, BUF_SIZE, fp);


					ret = zipWriteInFileInZip(zfile,buf,strlen(buf));

					if (ret != ZIP_OK) 
					{
						printf("zipWriteInFileInZip error\n");
						sleep(1);
						return;
					}
					
					if(feof(fp))
					{
						break;
					}
		
				}
				
				fclose(fp);  //关闭文件描述符
				
				memset(cmd,0,sizeof(cmd));
				
				sprintf(cmd,"rm -rf %s",file_name);

				system(cmd);  //删除文件
				
				usleep(200000);
				
			}
		}
		ret = zipCloseFileInZip(zfile);
		if (ret != ZIP_OK) 
		{
			printf("zipCloseFileInZip error\n");
			sleep(1);
			return;
		}
				
		ret = zipClose(zfile, NULL);
		if (ret != ZIP_OK) 
		{
			printf("zipClose error\n");
			sleep(1);
			return;
		}
		closedir(dir);
	}
	
	for(i=0;i<3;i++)
	{
		if(0 == curl_Post_CanFile(zipfile_name))   //上传成功之后删除压缩文件并执行改名
		{
			printf("curl_Post_CanFile success\n"); 
			
			memset(cmd,0,sizeof(cmd));
			
			sprintf(cmd,"rm -rf %s",zipfile_name);
			
			system(cmd);    //删除zip文件

			memset(cmd,0,sizeof(cmd));
			
			sprintf(cmd,"rm -rf %s","/media/sdcard/CanFileload/*");
			//成功之后确保清除/media/sdcard/CanFileload/下的文件
			system(cmd);
			
			break;
		}
		else
		{					
			printf("curl_Post_CanFile failed\n");
			
			memset(cmd,0,sizeof(cmd));
			
			sprintf(cmd,"rm -rf %s","/media/sdcard/CanFileload/*");
			//成功之后确保清除/media/sdcard/CanFileload/下的文件
			system(cmd);
		}
		sleep(1);
	}
}

/*
static void curl_CanFile_pkgzip(char *name)
{
	int ret;
	char zipfile_name[80] = {0};
	char buf[BUFFSIZE] = {0};
	FILE *fp = NULL;
	if(dir_exists(can_file_path) == 0 &&
				dir_make_path(can_file_path, S_IRUSR | S_IWUSR, false) != 0)
	{
		printf("path not exist and creat fail");
		sleep(1);
		return;
	}		
	strncpy(zipfile_name,name,sizeof(can_file_path)+17+1+10-3);
	strcat(zipfile_name,".zip");
	zipFile zfile;
	memset(&zfile,0,sizeof(zfile));
	zfile = zipOpen64(zipfile_name, 0);
	if (zfile == NULL) 
	{
		printf("zipOpen64 error\n");
		sleep(1);
		return;
	}
	
	zip_fileinfo zinfo;
	ret = zipOpenNewFileInZip(zfile, name+sizeof(can_file_path)-3, &zinfo, NULL,0,NULL,0,NULL, Z_DEFLATED, 1);
	//printf("name = %s\n",name+sizeof(can_file_path)-3);
	if (ret != ZIP_OK) 
	{
		printf("zipOpenNewFileInZip error\n");
		sleep(1);
		return;
	}
	fp = fopen(name, "r");
	if (NULL == fp)    // 检查打开情况
    {
        printf("fopen %s failed\n",name);
        return ;
    }

	while(1)
	{
		
		fgets(buf,BUF_SIZE,fp);

		if(feof(fp))
		{
			break;
		}

		ret = zipWriteInFileInZip(zfile,buf,strlen(buf));
		
		if (ret != ZIP_OK) 
		{
			printf("zipWriteInFileInZip error\n");
			sleep(1);
			return;
		}

		
	}
	fclose(fp);

	ret = zipCloseFileInZip(zfile);
	if (ret != ZIP_OK) 
	{
		printf("zipCloseFileInZip error\n");
		sleep(1);
		return;
	}
	ret = zipClose(zfile, NULL);
	if (ret != ZIP_OK) 
	{
		printf("zipClose error\n");
		sleep(1);
		return;
	}
} */

static void curl_LogFile_handler(void)
{
	int ret;
	char file_name[80] = {0};
	char zipfile_name[80] = {0};
	char stringVal[32] = {0};
	char buf[BUFFSIZE] = {0};
	char cmd[200] = {0};
	FILE *fp = NULL;
	DIR *dir = NULL;
	struct dirent *ptr;
	int i = 0;
	if(dir_exists(LOG_FILE_UPPATH) == 0 &&
				dir_make_path(LOG_FILE_UPPATH, S_IRUSR | S_IWUSR, false) != 0)
	{
		printf("path not exist and creat fail");
		sleep(1);
		return;
	}
				
	curl_timestamp_ultoa(log_timestamp.tv_sec,stringVal,10);  //时间戳转换成字符串
				
	memset(zipfile_name,0,sizeof(zipfile_name));
	strcpy(zipfile_name,LOG_FILE_UPPATH);
	strcat(zipfile_name,vin);
	strcat(zipfile_name,"_");
	strncat(zipfile_name,stringVal,sizeof(stringVal));
	strcat(zipfile_name,".zip");    //zip文件的时间就是触发时候的时间戳
		// /media/sdcard/log/LUZAGAAEP30A00701_xxxxxxxxxx.zip
	if((dir = opendir(LOG_FILE_UPPATH)) != NULL)     //打开文件夹
	{
		zipFile zfile;
		zfile = zipOpen64(zipfile_name, 0);   
		if (zfile == NULL) 
		{
			printf("zipOpen64 error\n");
			sleep(1);
			return;
		}
		while((ptr = readdir(dir)) != NULL)   
		{
			
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			memset(file_name,0,sizeof(file_name));
			
			if(strncmp(ptr->d_name+17+1+10,".zip",3) != 0)
			{
				strcpy(file_name,LOG_FILE_UPPATH);
				
				strcat(file_name,ptr->d_name);
				// /media/sdcard/log/LUZAGAAEP30A00701_xxxxxxxxxx.txt
				
				zip_fileinfo zinfo;
				ret = zipOpenNewFileInZip(zfile, ptr->d_name, &zinfo, NULL,0,NULL,0,NULL, Z_DEFLATED, 1);
				if (ret != ZIP_OK) 
				{
					printf("zipOpenNewFileInZip error\n");
					sleep(1);
					return;
				}
				
				fp = fopen(file_name, "r");   //以只读的方式打开原始文件
				if (NULL == fp)    // 检查打开情况
    			{
        			printf("fopen %s failed\n",file_name);
        			return ;
    			}

				while(1)
				{
		
					fgets(buf,CURL_BUF_SIZE,fp);

					if(feof(fp))
					{
						break;
					}

					ret = zipWriteInFileInZip(zfile,buf,strlen(buf));
		
					if (ret != ZIP_OK) 
					{
						printf("zipWriteInFileInZip error\n");
						sleep(1);
						return;
					}
		
				}
				fclose(fp);  //关闭文件描述符
				
				memset(cmd,0,sizeof(cmd));
				
				sprintf(cmd,"rm -rf %s",file_name);

				//system(cmd);  //删除文件
				
				usleep(200000);
			}
		}
		ret = zipCloseFileInZip(zfile);
		if (ret != ZIP_OK) 
		{
			printf("zipCloseFileInZip error\n");
			sleep(1);
			return;
		}
				
		ret = zipClose(zfile, NULL);
		if (ret != ZIP_OK) 
		{
			printf("zipClose error\n");
			sleep(1);
			return;
		}
		closedir(dir);
	}
	
	for(i=0;i<3;i++)
	{
		if(0 == curl_Post_LogFile(zipfile_name))   //上传成功之后删除压缩文件并执行改名
		{
			printf("curl_Post_LogFile success\n"); 

			memset(cmd,0,sizeof(cmd));
			
			sprintf(cmd,"rm -rf %s",zipfile_name);
			
			system(cmd);
			
			break;
		}
		else
		{					
			printf("curl_Post_LogFile failed\n");
		}
		sleep(1);
	}
}

/**
     * @brief    获取文件的大小.
     * @param[in] file_name.
     * @return    unsigned long, bytes.
*/
unsigned long curl_get_filesize(char *name)  
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

/*
static void curl_send_zipfile(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	if((dir = opendir(CAN_FILE_UPPATH)) != NULL)     //打开文件夹
	{
		while((ptr = readdir(dir)) != NULL)   
		{
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			if(strncmp(ptr->d_name+17+1+10,".zip",2) == 0)
			{
				memset(file_name,0,sizeof(file_name));
				strcpy(file_name,CAN_FILE_UPPATH);
				strcat(file_name,ptr->d_name);
			}
			
		}
	}
} */

/**
     * @brief    一分钟国标数据上传.
     * @param[in] file_name.
     * @return   int.
*/
int curl_Post_GbFile(char *name)
{
  	CURL *curl;
  	CURLcode res;
  	struct curl_httppost *formpost=NULL;
  	struct curl_httppost *lastptr=NULL;
  	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
 	char filesize[15]={0};
	
  	sprintf(filesize, "%lu", curl_get_filesize(name));
  	
  	//curl_global_init(CURL_GLOBAL_ALL);
   
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
      		fprintf(stderr, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
			
	  		printf("curl url_easy_perform() failed:%s",curl_easy_strerror(res));
			
	  		return -1;
		}
		else
		{
			printf("File uploaded %s successfully\n",name);
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

/**
     * @brief    一分整车can报文数据上传.
     * @param[in] file_name.
     * @return   int.
*/
int curl_Post_CanFile(char *name)
{
  	CURL *curl;
  	CURLcode res;
  	struct curl_httppost *formpost=NULL;
  	struct curl_httppost *lastptr=NULL;
  	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
 	char filesize[15]={0};
	
  	sprintf(filesize, "%lu", curl_get_filesize(name));
  	
  	//curl_global_init(CURL_GLOBAL_ALL);
   
    /*******************读取文件*************************/
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_FILE, name,			   
               	CURLFORM_CONTENTTYPE, "application/octet-stream", 
               	CURLFORM_END);
 
   /********************配置参数aid*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "aid",  
   				CURLFORM_COPYCONTENTS, "140",  
   				CURLFORM_END);
   
    /********************配置参数vin*********************/
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "vin",  
   				CURLFORM_COPYCONTENTS, vin, 
   				CURLFORM_END);
   /********************配置参数mid*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "mid",  
   				CURLFORM_COPYCONTENTS, "9",  
   				CURLFORM_END);
  
	curl_formadd(&formpost,  
                 &lastptr,  
                 CURLFORM_COPYNAME, "mid",  
                 CURLFORM_COPYCONTENTS, "\t", 
                 CURLFORM_END);
	
	/********************配置参数timestamp*********************/
	char timestamp[11] = {0};
	strncpy(timestamp,name+sizeof("/media/sdcard/CanFileload/LUZAGAAEP30A00701_")-1,10);
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "timestamp",  
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
    	curl_easy_setopt(curl, CURLOPT_URL, "https://file-uat.chehezhi.cn/fileApi/1.0/uploadData");
		
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
			printf("File uploaded %s successfully\n",name);
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

int curl_Post_LogFile(char *name)
{
  	CURL *curl;
  	CURLcode res;
  	struct curl_httppost *formpost=NULL;
  	struct curl_httppost *lastptr=NULL;
  	struct curl_slist *headerlist=NULL;
	static const char buf[] = "Expect:";
 	char filesize[15]={0};
	
  	sprintf(filesize, "%lu", curl_get_filesize(name));
  	
  	//curl_global_init(CURL_GLOBAL_ALL);
   
    /*******************读取文件*************************/
   	curl_formadd(&formpost,
               	&lastptr,
               	CURLFORM_COPYNAME, "file",
               	CURLFORM_FILE, name,			   
               	CURLFORM_CONTENTTYPE, "application/octet-stream", 
               	CURLFORM_END);
 
   /********************配置参数aid*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "aid",  
   				CURLFORM_COPYCONTENTS, "140",  
   				CURLFORM_END);
   
    /********************配置参数vin*********************/
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "vin",  
   				CURLFORM_COPYCONTENTS, vin, 
   				CURLFORM_END);
   /********************配置参数mid*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "mid",  
   				CURLFORM_COPYCONTENTS, "10",  
   				CURLFORM_END);
  
   /********************配置参数eventId*********************/
   curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "eventId",  
   				CURLFORM_COPYCONTENTS, log_eventId,  
   				CURLFORM_END);
	
	/********************配置参数timestamp*********************/
	char timestamp[11] = {0};
	strncpy(timestamp,name+sizeof("/media/sdcard/log/LUZAGAAEP30A00701_")-1,10);
	curl_formadd(&formpost,  
   				&lastptr,  
   				CURLFORM_COPYNAME, "timestamp",  
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
    	curl_easy_setopt(curl, CURLOPT_URL, "https://file-uat.chehezhi.cn/fileApi/1.0/uploadData");
		
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
			printf("File uploaded %s successfully\n",name);
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

/**
     * @brief    获取TBOX VIN、SN.
     * @param[in] void.
     * @return   void.
*/
void curl_get_tboxgbpara(char *pt)
{
	if(pt != NULL)
	{
		strncpy(vin,pt + 2,TBOX_VIN_LENGTH);  //获取VIN
		
		strncpy(tboxsn,pt+2+TBOX_VIN_LENGTH,TBOX_SN_LENGTH);//获取TBOXSN
	}
}

/**
     * @brief    获取TBOX VIN
     * @param[in] void.
     * @return   void.
*/
/*
void curl_get_tboxcanpara(char *pt)
{
	int i = 0;
	if(pt != NULL)
	{
		strncpy(vin,pt + 5,TBOX_VIN_LENGTH);  //获取VIN
		for(i =0;i<17;i++)
		printf("%c\n",*(pt+5+i));
	}
}
*/
/**
     * @brief    获取TBOX VIN eventId
     * @param[in] void.
     * @return   void.
*/
/*
void curl_get_tboxlogpara(char *pt)
{
	unsigned long eventId = 0;
	int i = 0;
	if(pt != NULL)
	{
		eventId = (((unsigned long)(pt[9]) )| ((unsigned long)(pt[10])) |  \
				  ((unsigned long)(pt[11]))| ((unsigned long)(pt[12])));
		printf("eventId = %ld\n",eventId);
		sprintf(log_eventId, "%lu", eventId);
		//for(i = 0; i<20;i++)
		//	printf("%d ",log_eventId[i]);
		strncpy(vin,pt + 13,TBOX_VIN_LENGTH);  //获取VIN
		for(i = 0; i<17;i++)
			printf("%d ",vin[i]);
	}
}
*/

/**
     * @brief    故障点之前报文上传处理函数.
     * @param[in] void.
     * @return   void.
*/
void curl_canfile_handle_before_time(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	char newfile_name[80] = {0};
	char zipfile_name[80] = {0};
	char cmd[200] = {0};
	
	struct stat buf;

	struct timeval now_timestamp;

	gettimeofday(&now_timestamp, NULL); 

	if(dir_exists(CAN_FILE_UPPATH) == 0 &&
				dir_make_path(CAN_FILE_UPPATH, S_IRUSR | S_IWUSR, false) != 0)
	{
		printf("path not exist and creat fail");
		sleep(1);
		return;
	}		
				
	if((dir = opendir(can_file_path)) != NULL)     //打开文件夹
	{
		while((ptr = readdir(dir)) != NULL)   
		{
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			memset(file_name,0,sizeof(file_name));
			
			strcpy(file_name,can_file_path);
			
			strcat(file_name,ptr->d_name);
			
			//printf("file_name = %s\n",file_name);
			stat(file_name,&buf);

			if((buf.st_mtime -60 > timestamp.tv_sec - 60 -  5 * 60)  \
				&& (buf.st_mtime -60 < timestamp.tv_sec +60))
			{
			
				if(strncmp(ptr->d_name+17+1+10,"_1",2) == 0)   //判断一下文件是否已经写完
				{
					//usleep(200000);
					
					//printf("canfile_handle_before_time\n");
				
					//LUZAGAAEP30A00701_xxxxxxxxxx_1.asc
					//curl_CanFile_pkgzip(file_name);

					memset(newfile_name,0,sizeof(newfile_name));
					memset(zipfile_name,0,sizeof(zipfile_name));
					
					strncpy(newfile_name,file_name,sizeof(can_file_path)+17+1+10-3);
					strcat(newfile_name,"_2.asc");         //将原始文件名标记已上传
					/*	
					strncpy(zipfile_name,file_name,sizeof(can_file_path)+17+1+10-3);  
					strcat(zipfile_name,".zip");          //压缩之后的文件名  
					
					if(0==sendPostCanFile(zipfile_name))   //上传成功之后删除压缩文件并执行改名
					{
						printf("sendPostFile success\n"); 
						
						sprintf(cmd,"rm -rf %s",zipfile_name);   
						
						system(cmd);                      //删除压缩文件

						memset(cmd,0,sizeof(cmd));

						sprintf(cmd,"mv %s %s",file_name,newfile_name);  //执行改名

						system(cmd);  
					}
					else
					{					
						printf("sendPostFile failed\n");
					} */
					sprintf(cmd,"cp -arf %s %s",file_name,CAN_FILE_UPPATH);   
						
					system(cmd);
					
					memset(cmd,0,sizeof(cmd));

					sprintf(cmd,"mv %s %s",file_name,newfile_name);  //执行改名

					system(cmd);  
				} 
				
			}
		}
		closedir(dir);
	}	
	else
	{		
	}
}

/**
     * @brief    整车报文上传处理函数.
     * @param[in] int.
     * @return   void.
*/
void curl_canfile_handle_after_time(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	char newfile_name[80] = {0};
	char zipfile_name[80] = {0};
	char cmd[200] = {0};
	struct stat buf;
	
	if(dir_exists(CAN_FILE_UPPATH) == 0 &&
				dir_make_path(CAN_FILE_UPPATH, S_IRUSR | S_IWUSR, false) != 0)
	{
		printf("path not exist and creat fail");
		sleep(1);
		return;
	}	
				
	if((dir = opendir(can_file_path)) != NULL)     //打开文件夹
	{
		while((ptr = readdir(dir)) != NULL)   
		{
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}

			memset(file_name,0,sizeof(file_name));

			strcpy(file_name,can_file_path);
			
			strcat(file_name,ptr->d_name);

			//printf("file_name = %s\n",file_name);

			stat(file_name,&buf);
			
			//sleep(1);
			
			//printf("canfile_handle_after_time11\n");
			
			if((buf.st_mtime - 60 > timestamp.tv_sec - 60  )  \
				&& (buf.st_mtime - 60 < timestamp.tv_sec + 60 + canfile_upload_time * 60))
			{
				if(strncmp(ptr->d_name+17+1+10,"_1",2) == 0)   //判断一下文件是否已经写完
				{
					//usleep(200000);
					//printf("canfile_handle_after_time22\n");
					//LUZAGAAEP30A00701_xxxxxxxxxx_1.asc
					//curl_CanFile_pkgzip(file_name);

					memset(newfile_name,0,sizeof(newfile_name));

					memset(zipfile_name,0,sizeof(zipfile_name));
					
					strncpy(newfile_name,file_name,sizeof(can_file_path)+17+1+10-3);
					strcat(newfile_name,"_2.asc");         //将原始文件名标记已上传
					/*
					strncpy(zipfile_name,file_name,sizeof(can_file_path)+17+1+10-3);  
					strcat(zipfile_name,".zip");          //压缩之后的文件名  
					
					if(0 == sendPostCanFile(zipfile_name))   //上传成功之后删除压缩文件并执行改名
					{
						printf("sendPostFile success\n"); 
						
						memset(cmd,0,sizeof(cmd));
						
						sprintf(cmd,"rm -rf %s",zipfile_name);   
						
						system(cmd);                      //删除压缩文件

						memset(cmd,0,sizeof(cmd));

						sprintf(cmd,"mv %s %s",file_name,newfile_name);  //执行改名

						system(cmd);  
					}
					else
					{					
						printf("sendPostFile failed\n");
					} */
					sprintf(cmd,"cp -arf %s %s",file_name,CAN_FILE_UPPATH);   
						
					system(cmd);
					
					memset(cmd,0,sizeof(cmd));

					sprintf(cmd,"mv %s %s",file_name,newfile_name);  //执行改名

					system(cmd);  
				}
			}
		}
		closedir(dir);
	}	
	else
	{		
	}
}

/**
     * @brief    国标上传线程.
     * @param[in] void.
     * @return   void.
*/
static void *curl_gbdata_main(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	char file_name[80] = {0};
	char cmd[60] = {0};
	
	for(;;)
	{
		sleep(15);   //延时15s
		
		if( gb_memory[1] == 0x01 ) //国标请求上传文件
		{

			memset(gb_memory,0,CURL_BUF_SIZE);
			
			if((dir = opendir(gb_file_path)) != NULL)     //打开文件夹
			{
			
				//printf("open gb_file_path success\n");
	
				while((ptr = readdir(dir)) != NULL)   
				{
					if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
					{
						continue;
					}
					memset(file_name,0,sizeof(file_name));
					
					memset(cmd,0,sizeof(cmd));
					
					strcat(file_name,gb_file_path);
					
  					strcat(file_name,ptr->d_name);
					
    				if(0 == curl_Post_GbFile(file_name))
					{
						log_i(LOG_CURL,"curl_Post_GbFile success\n"); 
						
						sprintf(cmd,"rm %s",file_name);
						
						system(cmd);
					}
					else
					{					
						printf("curl_Post_GbFile failed\n");
					}
					
					sleep(3);
				}
				
				closedir(dir);
				
				//printf("closedir finish\n");
			}	
			else
			{
				printf("open file_path fail\n");
			}
		}
	}
	return NULL;
}


/********************************************************************
     * @brief    整车报文上传线程.
     * @param[in] void.
     * @return   void.
*********************************************************************/
static void *curl_candata_main(void)
{
	struct timeval now_timestamp;
	
	while(1)
	{
		usleep(30000);
		/*****************故障触发前10分钟的处理**********************/
		if(before_flag == 1)   
		{
			if(can_memory[2] == 0x01) 
			{
				curl_canfile_handle_before_time();       //前十分钟文件
				
				before_flag = 0;
			}
		}
		
		
		/*****************故障触发的后10分钟处理或者TSP的请求处理**********************/
		
		
		if(can_memory[1] == 0x02)
		{
			
			curl_canfile_handle_after_time();//拷贝需要上传的文件到另外一个文件

			gettimeofday(&now_timestamp, NULL);  //故障触发时间戳
			
			if(now_timestamp.tv_sec > timestamp.tv_sec + canfile_upload_time*60 + 60)
			{
				memset(can_memory,0,CURL_BUF_SIZE);
				
				curl_CanFile_handler();

				printf("canfile finished\n");
			}
			
		}	
		
	}
	return NULL;
}

static void *curl_logdata_main(void)
{
	struct timeval now_timestamp;
	
	while(1)
	{
		usleep(30000);

		gettimeofday(&now_timestamp, NULL);  //故障触发时间戳
		
		if(log_memory[2] == 0x01)
		{
			if(log_starttime == 0)
			{
				if(now_timestamp.tv_sec > log_timestamp.tv_sec + log_uptime)
				{
					memset(log_memory,0,CURL_BUF_SIZE);
				
					curl_LogFile_handler();
				}
			}
			else
			{
				if(now_timestamp.tv_sec > log_starttime + log_uptime)
				{
					memset(log_memory,0,CURL_BUF_SIZE);
				
					curl_LogFile_handler();
				}
			}
		}
		
	}
	return NULL;
}

/**
     * @brief    上传进程主函数.
     * @param[in] int.
     * @return   int.
*/
int main(int argc, char *argv[])
{  
	int ret;
	
	int shmid = 0;
	
	int semid = 0;
	
	curl_global_init(CURL_GLOBAL_ALL);
	
	shmid = CreateShm(4096);   //创建共享内存
	
	char *addr = shmat(shmid,NULL,0); //共享内存映射到本地进程

	semid = Commsem();//创建信号量
	
	set_semvalue(semid);//初始化信号量值为1
	
    //创建国标数据上传线程
	pthread_attr_t ta;
	
    pthread_attr_init(&ta);
	
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED); //分离线程属性

    /* create thread and monitor the incoming data */
    ret = pthread_create(&curl_gb_tid, &ta, (void *)curl_gbdata_main, NULL);
	
    if (ret != 0)
    {
       printf("pthread_create gbdata_main failed\n");
    }
	
	//创建整车报文上传线程
	ret = pthread_create(&curl_can_tid, &ta, (void *)curl_candata_main, NULL);
    if (ret != 0)
    {
       printf("pthread_create candata_main failed\n");
    }

	//创建整车log上传线程
	ret = pthread_create(&curl_log_tid, &ta, (void *)curl_logdata_main, NULL);
    if (ret != 0)
    {
       printf("pthread_create logdata_main failed\n");
    }
	
	while(1)
	{
		usleep(5000);   //延时5ms
		
		sem_p(semid); //申请信号量，操作共享内存

		if(addr[0] == m_start)
		{
			if(addr[1] == 0x01)
			{
				printf("GB one-minute file upload request\n");
			
				strncpy(gb_memory,addr,CURL_BUF_SIZE);
				
				//curl_get_tboxgbpara(gb_memory);         //得到终端的VIN、TBOXSN
				strncpy(vin,addr + 2,TBOX_VIN_LENGTH);  //获取VIN
		
				strncpy(tboxsn,addr+2+TBOX_VIN_LENGTH,TBOX_SN_LENGTH);//获取TBOXSN
			
				memset(addr,0,4096);         //清掉共享内存	
			}
			else if(addr[1] == 0x02)
			{
				printf("Vehicle can upload request\n");

				strncpy(vin,addr+5,TBOX_VIN_LENGTH);  //获取VIN

				if(addr[2] == 0x01) //故障触发整车报文上传
				{
					gettimeofday(&timestamp, NULL);  //故障触发时间戳

					strncpy(can_memory,addr,CURL_BUF_SIZE);
					
					//curl_canfile_handle_before_time();       //前十分钟文件
					
					before_flag = 1;

					canfile_upload_time = 5;
					
					printf("Vehicle failure trigger!!!\n");
				}
				else if (addr[2] == 0x02)    //TSP触发
				{
					struct timeval now_timestamp;
					
					gettimeofday(&now_timestamp, NULL);  //故障触发时间戳

					if(before_flag == 1)
					{
						if((now_timestamp.tv_sec >timestamp.tv_sec) &&  \
							(now_timestamp.tv_sec < timestamp.tv_sec + 10*60))
						{
						
							memset(addr,0,4096);         //清掉共享内存	

							sem_v(semid); //申请信号量，操作共享内存
						
							continue;
						}
					}
						
					gettimeofday(&timestamp, NULL);
					
					canfile_upload_time = ((short)addr[4]) ;

					printf("Request to upload a %d-minute message\n",canfile_upload_time );

					strncpy(can_memory,addr,CURL_BUF_SIZE);	
					
				}
				
				//curl_get_tboxcanpara(can_memory);
				
				memset(addr,0,4096);         //清掉共享内存	
				
			}
			else if(addr[1] == 0x03)    //整车日志上传
			{
				printf("log file upload request\n");
			
				strncpy(log_memory,addr,CURL_BUF_SIZE);

				//curl_get_tboxlogpara(log_memory);

				if(addr[2] == 0x01)
				{
					gettimeofday(&log_timestamp, NULL); //日志上传时间戳
					
					//开始采集日志时间
					log_starttime = (((unsigned int)addr[3] <<24) |((unsigned int)addr[4] <<16) |
									((unsigned int)addr[5] <<8) | (unsigned int)addr[6]);
					//采集日志时长
					log_uptime =  ((unsigned short)addr[7] <<8) | ((unsigned short)addr[8]);

					eventid = (((unsigned int)addr[9] <<24) |((unsigned int)addr[10] <<16) |
									((unsigned int)addr[11] <<8) | (unsigned int)addr[12]);

					strncpy(vin,addr+13,17);
					
					printf("log_starttime = %d ",log_starttime);

					printf("log_uptime = %d ",log_uptime);
					
					printf("eventid = %d\n",eventid);
					
					sprintf(log_eventId, "%lu", (long)eventid);
				}
				else if(addr[2] == 0x02)
				{
					//停止采集日志
					memset(log_memory,0,sizeof(log_memory));
				}
				memset(addr,0,4096);         //清掉共享内存	
			}
		}
		sem_v(semid); //申请信号量，操作共享内存
			
	}
	return 0;
}


