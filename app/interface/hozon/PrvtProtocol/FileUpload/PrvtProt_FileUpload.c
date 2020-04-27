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
#include "at.h"
#include "pm_api.h"
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
#include "diag.h"
#include "fault_sync.h"
#include "dev_api.h"
#include "nm_api.h"
#include "shell_api.h"
#include "../PrvtProt_SigParse.h"
#include "../../support/protocol.h"
#include "cfg_api.h"
#include "udef_cfg_api.h"
#include "uds.h"
#include "PrvtProt_FileUpload.h"


/*******************************************************
description�� function declaration
*******************************************************/
extern unsigned char PP_rmtCtrl_cfg_CrashOutputSt(void);

/*Global function declaration*/
static PP_FileUpload_t PP_FileUL;

static int PP_tsp_flag = 0;

static PP_can_upload_t pp_up_can;
static int canupload_en = 0;
	
static PP_log_upload_t PP_up_log;
/*Static function declaration*/
static void *PP_FileUpload_main(void);
static void *PP_GbFileSend_main(void);
static void *PP_CanFileSend_main(void);
static void *PP_LogFileSend_main(void);


static void PP_FileUpload_datacollection(void);
static void PP_FileUpload_pkgzip(void);
static double PP_GetCanfile_Size(void);

static int PP_FileUpload_Tspshell(int argc, const char **argv);

static void PP_GbFile_delfile(void);
static void PP_CanFile_delfile(void);
static int PP_FileUpload_escapeData(uint8_t *datain,int datainlen,uint8_t *dataout,int *dataoutlen);
static int PP_FileUpload_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg);
static unsigned char PP_FileUpload_signTrigSt(unsigned char obj);
static void PP_FileUpload_set_log(unsigned char en);
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
	int infoCollectCycle;
	memset(&PP_FileUL,0,sizeof(PP_FileUpload_t));
	nm_register_status_changed(PP_FileUpload_nm_callback);

	PP_FileUL.pkgnum = PP_FILEUPLOAD_PACKNUM_DEF;
	infoCollectCycle = getPP_rmtCfg_infoCollectCycle();
	if(0 != infoCollectCycle)
	{
		if(infoCollectCycle > PP_FILEUPLOAD_PACKNUM_MAX)
		{
			PP_FileUL.pkgnum = PP_FILEUPLOAD_PACKNUM_MAX;
		}
		else
		{
			PP_FileUL.pkgnum = infoCollectCycle;
		}
	}
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

	pthread_t gb_tid;
	pthread_attr_t gb_ta;

    pthread_attr_init(&gb_ta);
    pthread_attr_setdetachstate(&gb_ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&gb_tid, &gb_ta, (void *)PP_GbFileSend_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "file upload pthread create failed, error: %s", strerror(errno));
    }

	pthread_t can_tid;
	pthread_attr_t can_ta;

    pthread_attr_init(&can_ta);
    pthread_attr_setdetachstate(&can_ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&can_tid, &can_ta, (void *)PP_CanFileSend_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "file upload pthread create failed, error: %s", strerror(errno));
    }

	//临时做一条shell命令
	shell_cmd_register("hozon_canfile_en", PP_FileUpload_Tspshell, "hozon_canfile_en");	

	pthread_t log_tid;
	pthread_attr_t log_ta;

    pthread_attr_init(&log_ta);
    pthread_attr_setdetachstate(&log_ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&log_tid, &log_ta, (void *)PP_LogFileSend_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "file upload pthread create failed, error: %s", strerror(errno));
    }
	
	unsigned int cfglen;
	unsigned int canfile_en;
	cfglen = 1;
	cfg_get_para(CFG_ITEM_EN_CANFILE, &canfile_en, &cfglen);
	if(canfile_en != 0)
	{
		canfile_en = 0;
		cfg_set_para(CFG_ITEM_EN_CANFILE, (unsigned char *)&canfile_en, 1);
	}

	unsigned char en;
	cfg_get_para(CFG_ITEM_LOG_ENABLE, &en, &cfglen); 
	if(en != 0)
	{
		en = 0;
    	PP_FileUpload_set_log(en); 
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
		unsigned int i;
		if( (1 == gb32960_gbCanbusActiveSt())		&& \
			(0 == GetPP_rmtCtrl_fotaUpgrade()) 		&& \
		    (DIAG_EMMC_OK == dev_diag_get_emmc_status()) 	&& \
			(0 == get_factory_mode()) 				&& \
			(1 == PP_rmtCfg_enable_dcEnabled()))
		{
			PP_FileUpload_datacollection();
			PP_FileUpload_pkgzip();
		}
		else
		{
			for(i = 0;i < PP_FILEUPLOAD_BUFNUM;i++)
			{
				memset(&PP_FileUL.buffer[i],0,sizeof(PP_FileUpload_Buf_t));
			}
		}

		usleep(100000);
    }

    return NULL;
}

void PP_FileUpload_CanMsgRequest(PP_can_upload_t can_para)
{
	PP_tsp_flag = 1;
	log_o(LOG_HOZON,"PP_tsp_flag  %d",can_para.PP_tsp_time);
	pp_up_can.PP_tsp_time = can_para.PP_tsp_time;
	pp_up_can.PP_tsp_eventId = can_para.PP_tsp_eventId;
	
	unsigned int canfile_en;
	canfile_en = 1;
	cfg_set_para(CFG_ITEM_EN_CANFILE, (unsigned char *)&canfile_en, 1);
	log_o(LOG_HOZON,"TSP request upload can file");
}

void PP_FileUpload_LogRequest(PP_log_upload_t log_para)
{
	PP_up_log.log_flag = 1;
	PP_up_log.log_grade = log_para.log_grade;  //日志等级
	PP_up_log.log_start_time = log_para.log_start_time;  //开始上传日志的时间
	PP_up_log.log_up_time = log_para.log_up_time;    //采集日志的时间
	PP_up_log.log_eventId = log_para.log_eventId;
	
	log_o(LOG_HOZON,"TSP request upload log file");
}

static int PP_FileUpload_Tspshell(int argc, const char **argv)
{
	unsigned int canfile_en;
    if (argc != 1)
    {
        shellprintf(" usage:set pki en error\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &canfile_en);
	cfg_set_para(CFG_ITEM_EN_CANFILE, (unsigned char *)&canfile_en, 1);
	shellprintf(" set canfile ok\r\n");
	return 0;
}

//国标一分钟文件上传线程
static void *PP_GbFileSend_main(void)
{
	log_o(LOG_HOZON, "gbfile send thread running");
    prctl(PR_SET_NAME, "GBFILE_SEND");
	char vin[18] = {0};
	char tboxsn[19] = {0};
	char buf[200] = {0};
	unsigned int len;
	int semid = Commsem();
	set_semvalue(semid);//初始化信号量值为1
    while(1)
    {	
    	sleep(30);	
		int shmid = GetShm(4096);
		char *addr = shmat(shmid,NULL,0);
		
    	if((0 == GetPP_rmtCtrl_fotaUpgrade()) 	&& \
		   (0 == PP_netstatus_pubilcfaultsts(NULL)) && \
		   (1 == PP_rmtCfg_enable_dcEnabled()) 	&& \
		   (1 == PP_FileUL.network) 			&& \
		   (0 == get_factory_mode()))
    	{	
    		
    		//国标文件上传
			gb32960_getvin(vin);
			len = sizeof(tboxsn);
			cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXSN,tboxsn,&len);
			buf[0] = m_start;
			buf[1] = 0x01;     //代表国标触发
			sprintf(buf+2,"%s%s",vin,tboxsn);
			buf[37] = m_end;
			sem_p(semid);
			memcpy(addr,buf,38);
			sem_v(semid);	
    	}
		shmdt(addr);
		
    	if(dev_diag_get_emmc_status() == DIAG_EMMC_OK)
    	{
			PP_GbFile_delfile();
    	}
		
    }

    return NULL;
}

//整车报文上传线程
static void *PP_CanFileSend_main(void)
{
	log_o(LOG_HOZON, "canfile send thread running");
    prctl(PR_SET_NAME, "CANFILE_SEND");
	char buf[200] = {0};
	char vin[18] = {0};
	struct timeval TSP_request_timestamp;
	int semid = Commsem();
	set_semvalue(semid);//初始化信号量值为1
	while(1)
    {	
		unsigned char obj;

		sleep(20);	
		int shmid = GetShm(4096);
		char *addr = shmat(shmid,NULL,0);
		
		unsigned int cfglen;
		cfglen = 1;
		cfg_get_para(CFG_ITEM_EN_CANFILE, &canupload_en, &cfglen);
		
		if(	(canupload_en == 1)					&& \
			(dev_get_KL15_signal() == 1) 		&& \
			(0 == GetPP_rmtCtrl_fotaUpgrade()) 	&& \
			(0 == PP_netstatus_pubilcfaultsts(NULL)) && \
			(1 == PP_FileUL.network) 			&& \
			(0 == get_factory_mode()))
		{
			//整车报文文件上传
			for(obj = 0;obj < PP_CANFILEUL_SIGN_WARN_MAX;obj++)
			{
				PP_FileUL.warnSign[obj].newSt = PP_FileUpload_signTrigSt(obj);
				if(PP_FileUL.warnSign[obj].newSt != PP_FileUL.warnSign[obj].oldSt)
				{
					PP_FileUL.warnSign[obj].oldSt = PP_FileUL.warnSign[obj].newSt;
					if(1 == PP_FileUL.warnSign[obj].newSt)
					{
						//PP_FileUL.signTrigFlag = 1;
						break;
					}
				}
			}
		
			if(1 == PP_FileUL.signTrigFlag)    //整车报文触发条件
			{	
				gb32960_getvin(vin);
				memset(buf,0,sizeof(buf));
				buf[0] = m_start;
				buf[1] = 0x02;
				buf[2] = 0x01;
				buf[3] = 0;     //故障触发第3、4字节填0补充
				buf[4] = 0;
				buf[5] = 0;     //eventid四个字节
				buf[6] = 0;
				buf[7] = 0;
				buf[8] = 0;
				sprintf(buf+9,"%s",vin);
				buf[26] = m_end;
				sem_p(semid);
				memcpy(addr,buf,27);
				sem_v(semid);
				log_o(LOG_HOZON,"PP_CanFileSend_main sem_v");
				PP_FileUL.signTrigFlag = 0;
			}
			
			if(PP_tsp_flag == 1)                   //平台请求触发
			{
				gettimeofday(&TSP_request_timestamp, NULL);  //故障触发时间戳
				gb32960_getvin(vin);
				memset(buf,0,sizeof(buf));
				buf[0] = m_start;
				buf[1] = 0x02;
				buf[2] = 0x02;
				buf[3] = (char)(pp_up_can.PP_tsp_time >> 8);
				buf[4] = (char)pp_up_can.PP_tsp_time;
				buf[5] = (char)(pp_up_can.PP_tsp_eventId >> 24);     //eventid四个字节
				buf[6] = (char)(pp_up_can.PP_tsp_eventId >> 16);
				buf[7] = (char)(pp_up_can.PP_tsp_eventId >> 8);
				buf[8] = (char)(pp_up_can.PP_tsp_eventId );
				sprintf(buf+9,"%s",vin);
				buf[26] = m_end;
				sem_p(semid);
				memcpy(addr,buf,27);
				log_o(LOG_HOZON,"addr[4] = %d",addr[4]);
				sem_v(semid);
				PP_tsp_flag = 0;
			}
		}
		else
		{
			PP_tsp_flag = 0;
		}
		
		if(canupload_en == 1)  //
		{
			struct timeval nowtime_stamp;
			gettimeofday(&nowtime_stamp, NULL);  
			if(nowtime_stamp.tv_sec > TSP_request_timestamp.tv_sec + pp_up_can.PP_tsp_time * 60 + 60)
			{
				unsigned int canfile_en;
				canfile_en = 0;
				cfg_set_para(CFG_ITEM_EN_CANFILE, (unsigned char *)&canfile_en, 1);
			}
		}
		shmdt(addr);

		//整车报文文件删除
		if(dev_diag_get_emmc_status() == DIAG_EMMC_OK)
		{
			PP_CanFile_delfile();
		}
    }
}
static void *PP_LogFileSend_main(void)
{
	log_o(LOG_HOZON, "logfile send thread running");
    prctl(PR_SET_NAME, "LOGFILE_SEND");

	int i;
	char vin[18] = {0};
	unsigned char en = 0;
	char cmd[200] = {0};
	static int start_flag = 0;
	static uint32_t start_time = 0;
	static uint16_t up_time = 0;
	struct timeval up_timestamp;
	int semid = Commsem();
	set_semvalue(semid);//初始化信号量值为1
    while(1)
    {	
    	sleep(10);	
		int shmid = GetShm(4096);
		char *addr = shmat(shmid,NULL,0);
		
    	if((dev_get_KL15_signal() == 1) &&	\
		   (0 == PP_netstatus_pubilcfaultsts(NULL)) && \
		   (1 == PP_FileUL.network) 	&& \
		   (0 == get_factory_mode()))
    	{	
    		if((PP_up_log.log_flag == 1) && (PP_up_log.log_stop_flag == 0))
    		{
    			en = 1;
    			PP_FileUpload_set_log(en); //打开日志文件生成
    			memset(cmd,0,sizeof(cmd));
				sprintf(cmd,"rm -rf %s","/media/sdcard/log/*");
				system(cmd);
				start_time = PP_up_log.log_start_time;
				up_time = PP_up_log.log_up_time;
				start_flag = 1;
				gettimeofday(&up_timestamp, NULL);  //故障触发时间戳
				sem_p(semid);
				addr[0] = m_start;
				addr[1] = 0x03;     //代表log触发
				addr[2] = 0x01;     //开始上传
				addr[3] = (char)(PP_up_log.log_start_time >> 24);
				addr[4] = (char)(PP_up_log.log_start_time >> 16);
				addr[5] = (char)(PP_up_log.log_start_time >> 8);
				addr[6] = (char)(PP_up_log.log_start_time );
				addr[7] = (char)(PP_up_log.log_up_time >> 8);
				addr[8] = (char)(PP_up_log.log_up_time );
				addr[9] = (char)(PP_up_log.log_eventId >> 24);
				addr[10] = (char)(PP_up_log.log_eventId >> 16);
				addr[11] = (char)(PP_up_log.log_eventId >> 8);
				addr[12] = (char)(PP_up_log.log_eventId );
				for(i=0;i<17;i++)
					addr[13+i] = vin[i];
				gb32960_getvin(vin);
				addr[30] = m_end;
				sem_v(semid);
				PP_up_log.log_flag = 0;
    		}
			else if((PP_up_log.log_flag == 1) && (PP_up_log.log_stop_flag == 1))
			{
				en = 0;
    			PP_FileUpload_set_log(en); //打开日志文件生成
				sem_p(semid);
				addr[0] = m_start;
				addr[1] = 0x03;     //代表log触发
				addr[2] = 0x02;     //停止上传
				addr[30] = m_end;
				sem_v(semid);
				PP_up_log.log_flag = 0;
				PP_up_log.log_stop_flag = 0;
			}
    	}
		shmdt(addr);	
		if(start_flag == 1)
		{
			struct timeval nowtime_stamp;
			gettimeofday(&nowtime_stamp, NULL); 
			if(start_time == 0)
			{
				if(nowtime_stamp.tv_sec - (up_timestamp.tv_sec + up_time) > 0)
				{
					en = 0;
    				PP_FileUpload_set_log(en); //打开日志文件生成
    				start_flag = 0;
				}
			}
			else
			{
				if(nowtime_stamp.tv_sec - (start_time+up_time) > 0)
				{
					en = 0;
    				PP_FileUpload_set_log(en); //打开日志文件生成
    				start_flag = 0;
				}
			}
		}
    }

    return NULL;
}

static double PP_GetCanfile_Size(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	struct stat buf;
	char file_name[80] = {0};
	double total_size = 0;
	if((dir = opendir(PP_CANFILEUPLOAD_PATH)) != NULL)
	{
		
		while((ptr = readdir(dir)) != NULL)
		{
			memset(file_name,0,sizeof(file_name));
			
			if((strcmp(ptr->d_name,".") == 0)||(strcmp(ptr->d_name,"..") == 0))
			{
				continue;
			}
			
			strcpy(file_name,PP_CANFILEUPLOAD_PATH);
			strcat(file_name,ptr->d_name);

			if(at_get_pm_mode() == PM_LISTEN_MODE)
			{
				closedir(dir);
				return 0;
			}
			
			if(stat(file_name,&buf) < 0)
			{
				log_e(LOG_HOZON,"stat failed");
				closedir(dir);
				return 0;
			}
			else
			{
				total_size += buf.st_size;   //总字节大小
			}
			
		}
		closedir(dir);
	}
	return total_size/1024/1024/1024;
}

static void PP_CanFile_delfile(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	char cmd[80] = {0};
	double size = 0;
	char stamp_min[12] = {'9','9','9','9','9','9','9','9','9','9','0','0'};
	
	if((size = PP_GetCanfile_Size()) > 2)//保存整车报文大小大于2G
	{
		
		if((dir = opendir(PP_CANFILEUPLOAD_PATH)) != NULL)
		{
			
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
					
					sprintf(cmd,"rm -rf %s%s",PP_CANFILEUPLOAD_PATH,ptr->d_name);
				}
			}
			system(cmd);
			
			log_i(LOG_HOZON,"carried out %s success",cmd);
			
			closedir(dir);
		}
		
	}
	else
	{
		log_i(LOG_HOZON,"size = %lf",size);
	}
}

static void PP_GbFile_delfile(void)
{
	DIR *dir = NULL;
	struct dirent *ptr;
	int cnt = 0;
	char cmd[80] = {0};
	char stamp_min[12] = {0};
	
	if((dir = opendir(PP_FILEUPLOAD_PATH)) != NULL)
	{
	
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
	
	if(cnt > PP_FILEUPLOAD_MAXPKG)
	{
		if((dir = opendir(PP_FILEUPLOAD_PATH)) != NULL)
		{
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
			
			log_i(LOG_HOZON,"carried out %s success",cmd);
			
			closedir(dir);
		}
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
		memset(PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].data,0,2*PP_FILEUPLOAD_DATALEN);
		if(0 == PP_FileUpload_escapeData(data,len, \
					PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].data,\
					&PP_FileUL.buffer[PP_FileUL.index].pack[PP_FileUL.buffer[PP_FileUL.index].cnt].len))
		{
			PP_FileUL.buffer[PP_FileUL.index].cnt++;
			if(PP_FileUL.pkgnum == PP_FileUL.buffer[PP_FileUL.index].cnt)
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

			for(j = 0;j < PP_FileUL.pkgnum;j++)
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
	char pbuf[3] = {0};

	for(i = 0;i < datainlen;i++)
	{
		sprintf(pbuf,"%02X",datain[i]);
		strncat((char*)dataout,pbuf,2);
		j = j + 2;
		if(j >= (2*PP_FILEUPLOAD_DATALEN -2))
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

/*
* 获取网络状态
*/
static int PP_FileUpload_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
{
    if (NM_PUBLIC_NET != type)
    {
        return 0;
    }

    switch (nmmsg)
    {
        case NM_REG_MSG_CONNECTED:
		{
            PP_FileUL.network = 1;
		}
        break;
        case NM_REG_MSG_DISCONNECTED:
		{
            PP_FileUL.network = 0;
		}
        break;
        default:
            return -1;
    }

	return 0;
}

/*
* 获取网络状态
*/
static unsigned char PP_FileUpload_signTrigSt(unsigned char obj)
{
	unsigned char trigSt = 0;
	unsigned char tempVal;
	switch(obj)
	{
		case PP_CANFILEUL_SIGN_VCU5SYSFLT:
		{
			trigSt = PrvtProt_SignParse_SysFaultSt();
		}
		break;
		case PP_CANFILEUL_SIGN_BMSCOMMFLT:
		{
			trigSt = gb_data_BMSCommFaultSt();
		}
		break;
		case PP_CANFILEUL_SIGN_GASPEDELFLT:
		{
			trigSt = gb_data_GasPedalFault();
		}
		break;
		case PP_CANFILEUL_SIGN_MCU2FLTLVL:
		{
			trigSt = (PrvtProt_SignParse_Mcu2FltLvl() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_TEMPRISEFAST:
		{
			trigSt = (gb_data_BattTempRiseFast() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_CELLOVERTEMP:
		{
			trigSt = (gb_data_cellOverTemp() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_UNDERVOLT:
		{
			tempVal = gb_data_underVoltSts();
			trigSt = ((tempVal == 0x2) || (tempVal == 0x3))?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_OVERCURRENT:
		{
			trigSt = (PrvtProt_SignParse_overCurrSt() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_ISOISUPER:
		{
			trigSt = (gb_data_IsolSuperSts() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_TEMPDIFF:
		{
			trigSt = (gb_data_tempDiffSts() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_BMS7DIAGSTS:
		{
			trigSt = (PrvtProt_SignParse_DiagSts() == 0x3)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_EGSMERR://
		{
			trigSt = PrvtProt_SignParse_EGSMErrSt();
		}
		break;
		case PP_CANFILEUL_SIGN_BRAKEWARNLAMP://
		{
			trigSt = (PrvtProt_SignParse_BkWarnLampSt() != 0)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_AVHLAMPREQ://
		{
			trigSt = (PrvtProt_SignParse_AVHLampSts() == 0x1)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_EPBSTLAMPREQ:
		{
			trigSt = (gb_data_EPBLampSt() == 0x2)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_EHBFAIL://
		break;
		case PP_CANFILEUL_SIGN_ESCLFAILIND://
		break;
		case PP_CANFILEUL_SIGN_RLTYREPRESS://左后
		{
			tempVal = gb_data_tyrePressWarnSts(2);
			trigSt = ((tempVal == 0x2) || (tempVal == 0x3))?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_RLTYRETEMP:
		{
			trigSt = gb_data_tyreHighTempWarnSts(2);
		}
		break;
		case PP_CANFILEUL_SIGN_RLTYREQUCIKLK:
		{
			trigSt = gb_data_tyreLeakWarnSts(2);
		}
		break;
		case PP_CANFILEUL_SIGN_RRTYREPRESS://右后
		{
			tempVal = gb_data_tyrePressWarnSts(3);
			trigSt = ((tempVal == 0x2) || (tempVal == 0x3))?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_RRTYRETEMP:
		{
			trigSt = gb_data_tyreHighTempWarnSts(3);
		}
		break;
		case PP_CANFILEUL_SIGN_RRTYREQUCIKLK:
		{
			trigSt = gb_data_tyreLeakWarnSts(3);
		}
		break;
		case PP_CANFILEUL_SIGN_FLTYREPRESS://左前
		{
			tempVal = gb_data_tyrePressWarnSts(0);
			trigSt = ((tempVal == 0x2) || (tempVal == 0x3))?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_FLTYRETEMP:
		{
			trigSt = gb_data_tyreHighTempWarnSts(0);
		}
		break;
		case PP_CANFILEUL_SIGN_FLTYREQUCIKLK:
		{
			trigSt = gb_data_tyreLeakWarnSts(0);
		}
		break;
		case PP_CANFILEUL_SIGN_FRTYREPRESS://右前
		{
			tempVal = gb_data_tyrePressWarnSts(1);
			trigSt = ((tempVal == 0x2) || (tempVal == 0x3))?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_FRTYRETEMP:
		{
			trigSt = gb_data_tyreHighTempWarnSts(1);
		}
		break;
		case PP_CANFILEUL_SIGN_FRTYREQUCIKLK:
		{
			trigSt = gb_data_tyreLeakWarnSts(1);
		}
		break;
		case PP_CANFILEUL_SIGN_BDMSYSFAIL:
		{
			trigSt = (gb_data_BDMSysFail() != 0)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_AIRBAGFAIL:
		{
			trigSt = (gb_data_AirBagFailSts() == 0x2)?1:0;
		}
		break;
		case PP_CANFILEUL_SIGN_CRASHOUTPUT:
		{
			trigSt = PP_rmtCtrl_cfg_CrashOutputSt();
		}
		break;
		case PP_CANFILEUL_SIGN_EPSELESTRFAIL:
		{
			trigSt = (gb_data_EPSFaultSts() == 0x1)?1:0;
		}
		break;
		default:
		break;
	}

	return trigSt;
}

/*
 * 开启/关闭 log 采集存储
*/
static void PP_FileUpload_set_log(unsigned char en)
{
    if (en && !bfile_exists("/dev/mmcblk0p1"))
    {
       	log_e(LOG_HOZON,"set log,eMMC has not been formated!!!\r\n");
        return;
    }

    if (en && !path_exists(COM_LOG_DIR) &&
        dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false) != 0)
    {
        log_e(LOG_HOZON," make log directory(%s) failed\r\n", COM_LOG_DIR);
        return;
    }

    if (cfg_set_para(CFG_ITEM_LOG_ENABLE, &en, 1) != 0)
    {
        log_e(LOG_HOZON," save log configuration failed!\r\n");
        return;
    }

    log_save_ctrl(en ? dev_get_reboot_cnt() : 0, COM_LOG_DIR);

    if (en)
    {
        dev_print_softver_delay();
    }

	log_o(LOG_HOZON," set log success\r\n");
}

