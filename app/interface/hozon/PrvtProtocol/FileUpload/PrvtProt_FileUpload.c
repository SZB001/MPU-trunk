/******************************************************
�ļ�����	PrvtProt_FileUpload.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <dirent.h>

#include <sys/time.h>
#include <semaphore.h>

#include "timer.h"
#include <sys/prctl.h>
#include "dir.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include "init.h"
#include "log.h"
#include "file.h"
#include "curl_commshm.h"
#include "../../../../base/minizip/zip.h"
#include "gb32960_api.h"
#include "hozon_PP_api.h"
#include "hozon_SP_api.h"
#include "../PrvtProt.h"
#include "com_app_def.h"
#include "dir.h"
#include "file.h"
#include "dev_api.h"
#include "../../support/protocol.h"
#include "PrvtProt_FileUpload.h"


/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/
static PP_FileUpload_t PP_FileUL;

/*Static function declaration*/
static void *PP_FileUpload_main(void);
static void *PP_FileSend_main(void);

static void PP_FileUpload_datacollection(void);
static void PP_FileUpload_pkgzip(void);
static void PP_FileUpload_delfile(void);
static int PP_FileUpload_escapeData(uint8_t *datain,int datainlen,uint8_t *dataout,int *dataoutlen);
/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPP_FileUpload_Parameter

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void InitPP_FileUpload_Parameter(void)
{
	memset(&PP_FileUL,0,sizeof(PP_FileUpload_t));
}

/******************************************************
*PP_FileUpload_run

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
int PP_FileUpload_run(void)
{
    int ret;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &ta, (void *)PP_FileUpload_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "file upload pthread create failed, error: %s", strerror(errno));
    }

	pthread_t send_tid;
	pthread_attr_t send_ta;

    pthread_attr_init(&send_ta);
    pthread_attr_setdetachstate(&send_ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&send_tid, &send_ta, (void *)PP_FileSend_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "file upload pthread create failed, error: %s", strerror(errno));
    }

	return 0;
}

/******************************************************
*PP_FileUpload_main

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
static void *PP_FileUpload_main(void)
{
	log_o(LOG_HOZON, "file upload thread running");
    prctl(PR_SET_NAME, "FILE_UPLOAD");
    while(1)
    {
		if(gb32960_networkSt())
		{
			PP_FileUpload_datacollection();
			PP_FileUpload_pkgzip();
		}

		usleep(100*1000);
    }

    return NULL;
}
static void *PP_FileSend_main(void)
{
	log_o(LOG_HOZON, "file send thread running");
    prctl(PR_SET_NAME, "FILE_SEND");
	usleep(20);
    while(1)
    {	
		int shmid = GetShm(4096);
		char *addr = shmat(shmid,NULL,0);
		
    	if((dev_get_KL15_signal() == 1)&&(0 == PP_netstatus_pubilcfaultsts(NULL)))
    	{	
			strcpy(addr,"upload");
    	}
		else
		{
			PP_FileUpload_delfile();
		}
		shmdt(addr);
		sleep(30);	
    }

    return NULL;
}

static void PP_FileUpload_delfile(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	int cnt = 0;
	char cmd[80] = {0};
	char stamp_min[12] = {0};
	if((dir = opendir(PP_FILEUPLOAD_PATH)) != NULL)
	{
		log_o(LOG_HOZON,"open %s success",PP_FILEUPLOAD_PATH);
	
		while((ptr = readdir(dir)) != NULL)
		{
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			cnt++;
			if(cnt == 1)
			{
				strncpy(stamp_min,ptr->d_name+18,10);
				sprintf(cmd,"rm -rf %s%s",PP_FILEUPLOAD_PATH,ptr->d_name);
			}
		}
		closedir(dir);
	}
	if(cnt > 10)
	{
		if((dir = opendir(PP_FILEUPLOAD_PATH)) != NULL)
		while((ptr = readdir(dir)) != NULL)
		{
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			if(strncmp(ptr->d_name+18,stamp_min,10) < 0)
			{
				strncpy(stamp_min,ptr->d_name+18,10);
				memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"rm -rf %s%s",PP_FILEUPLOAD_PATH,ptr->d_name);
			}
		}
		system(cmd);
		log_o(LOG_HOZON,"carried out %s success",cmd);
		closedir(dir);
	}
}

/******************************************************
*PP_FileUpload_datacollection

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
static void PP_FileUpload_datacollection(void)
{
	uint8_t data[PP_FILEUPLOAD_DATALEN];
	int len;
	if(1 == gb_data_perReportPack(data,&len))
	{
		//protocol_dump(LOG_HOZON, "zip original data", data, len, 1);
		if(0 == PP_FileUpload_escapeData(data,len, \
					PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].data,\
					&PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].len))
		{
			PP_FileUL.buffer[PP_FileUL.index].cnt++;
			if(PP_FILEUPLOAD_PACKNUM == PP_FileUL.buffer[PP_FileUL.index].cnt)
			{
				PP_FileUL.buffer[PP_FileUL.index].cnt = 0;
				PP_FileUL.buffer[PP_FileUL.index].successflag = 1;
				PP_FileUL.index++;
			}

			if(PP_FILEUPLOAD_BUFNUM == PP_FileUL.index)
			{
				PP_FileUL.index = 0;
			}

			log_i(LOG_HOZON, "index = %d,cnt = %d\n",PP_FileUL.index,PP_FileUL.buffer[PP_FileUL.index].cnt);
		}
	}
}

/******************************************************
*PP_FileUpload_pkgzip

*��  �Σ�

*����ֵ��

*��  ������������

*��
******************************************************/
static void PP_FileUpload_pkgzip(void)
{
	uint8_t i,j;
	int ret;
	char filename[64] = {0};
	char filepathname[128] = {0};
	char stringVal[32] = {0};
	char vin[18] = {0};
	uint64_t timestamp;

	if(dir_exists(PP_FILEUPLOAD_PATH) == 0 &&
				dir_make_path(PP_FILEUPLOAD_PATH, S_IRUSR | S_IWUSR, false) != 0)
	{
		log_e(LOG_HOZON, "path not exist and creat fail");
		sleep(1);
		return;
	}

	for(i=0;i<PP_FILEUPLOAD_BUFNUM;i++)
	{
		if(1 == PP_FileUL.buffer[i].successflag)
		{
			log_i(LOG_HOZON, "buffer %d is filled in,start build file\n",i);
			PP_FileUL.buffer[i].successflag = 0;
			gb32960_getvin(vin);
			memcpy(filename,vin,17);
			memcpy(filename + 17,"_",1);
			timestamp = PrvtPro_getTimestamp();
			PP_rmtCfg_ultoa(timestamp,stringVal,10);
			memcpy(filename + 17 + 1,stringVal,strlen(stringVal));
			log_i(LOG_HOZON, "file name = %s\n",filename);

			snprintf(filepathname, sizeof(filepathname), "%s%s%s", PP_FILEUPLOAD_PATH,filename, ".zip");
			log_i(LOG_HOZON, "file path and name = %s\n",filepathname);

			zipFile zfile;
			zfile = zipOpen64(filepathname, 0);
			if (zfile == NULL) 
			{
				log_e(LOG_HOZON, "zipOpen64 error\n");
				sleep(1);
				return;
			}

			zip_fileinfo zinfo;
			memset(&zinfo,0,sizeof(zip_fileinfo));
			memcpy(filename + 17 + 1 + strlen(stringVal),".txt",strlen(".txt"));
			ret = zipOpenNewFileInZip(zfile, filename, &zinfo, NULL,0,NULL,0,NULL, Z_DEFLATED, 1);
			if (ret != ZIP_OK) 
			{
				log_e(LOG_HOZON, "zipOpenNewFileInZip error\n");
				sleep(1);
				return;
			}

			for(j = 0;j < PP_FILEUPLOAD_PACKNUM;j++)
			{
				ret = zipWriteInFileInZip(zfile, PP_FileUL.buffer[i].pack[j].data, PP_FileUL.buffer[i].pack[j].len);
				if (ret != ZIP_OK) 
				{
					log_e(LOG_HOZON,"zipWriteInFileInZip error\n");
					sleep(1);
					return;
				}
			}

			ret = zipCloseFileInZip(zfile);
			if (ret != ZIP_OK) 
			{
				log_e(LOG_HOZON, "zipCloseFileInZip error\n");
				sleep(1);
				return;
			}

			ret = zipClose(zfile, NULL);
			if (ret != ZIP_OK) 
			{
				log_e(LOG_HOZON, "zipClose error\n");
				sleep(1);
				return;
			}
		}
	}
}

/******************************************************
*PP_FileUpload_escapeData

*��  �Σ�

*����ֵ��

*��  ������������

*��	数据转义
******************************************************/
static int PP_FileUpload_escapeData(uint8_t *datain,int datainlen,uint8_t *dataout,int *dataoutlen)
{
	int i = 0;
	int j = 0;

	for(i = 0;i < datainlen;i++)
	{
		switch(datain[i])
		{
			case 0xa:
			{
				dataout[j++] = 0x9;
				dataout[j++] = 0x2;
			}
			break;
			case 0x9:
			{
				dataout[j++] = 0x9;
				dataout[j++] = 0x1;
			}
			break;
			case 0xd:
			{
				dataout[j++] = 0xc;
				dataout[j++] = 0x2;
			}
			break;
			case 0xc:
			{
				dataout[j++] = 0xc;
				dataout[j++] = 0x1;
			}
			break;
			default:
			{
				dataout[j++] = datain[i];
			}
			break;
		}

		if(j >= (PP_FILEUPLOAD_DATALEN -2))
		{
			return -1;
		}
	}

	dataout[j++] = 0xd;
	dataout[j++] = 0xa;

	*dataoutlen = j;
	//protocol_dump(LOG_HOZON, "zip escape data", dataout, *dataoutlen, 1);
	return 0;
}
