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
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>
#include "dir.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include "init.h"
#include "log.h"
#include "file.h"
#include "gb32960_api.h"
#include "hozon_PP_api.h"
#include "../PrvtProt.h"
#include "PrvtProt_FileUpload.h"

/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/
static PP_FileUpload_t PP_FileUL;

/*Static function declaration*/
static void *PP_FileUpload_main(void);
static void PP_FileUpload_datacollection(void);
static void PP_FileUpload_pkgzip(void);
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
void PP_FileUpload_run(void)
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
		PP_FileUpload_datacollection();
		PP_FileUpload_pkgzip();
		usleep(10*1000);
    }

    return NULL;
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
	uint8_t data[1024];
	int len;
	if(1 == gb_data_perReportPack(data,&len))
	{
		memcpy(PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].data, \
				data,1024);
		PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].len = len;
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
	char filename[64] = {0};
	char filepathname[128] = {0};
	char fileCompInstr[128] = {0};
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

			snprintf(filepathname, sizeof(filepathname), "%s%s%s", PP_FILEUPLOAD_PATH,filename, ".txt");
			log_i(LOG_HOZON, "file path and name = %s\n",filepathname);

			FILE *fp;
			fp = fopen(filepathname,"a+");
			if(fp==NULL)
			{
				log_e(LOG_HOZON, "open file fail\n");
			}

			for(j = 0;j < PP_FILEUPLOAD_PACKNUM;j++)
			{
				fwrite(PP_FileUL.buffer[i].pack[j].data, 1, PP_FileUL.buffer[i].pack[j].len, fp); /* 写文件*/
				fwrite("\r\n\r\n", 1, 4, fp);
			}

			fclose(fp);

			snprintf(fileCompInstr, sizeof(fileCompInstr), "%s%s%s%s%s", "cd /media/sdcard/fileUL/ && tar -cvf ", \
																						filename, ".tar ",filename,".txt");
			log_i(LOG_HOZON, "file Comp Instr = %s\n",fileCompInstr);
			system(fileCompInstr);
			//system("cd /media/sdcard/fileUL/ && tar -cvf LUZAGAAA6JA000655_1576312873.tar LUZAGAAA6JA000655_1576312873.txt");
			unlink(filepathname);
		}
	}
}
