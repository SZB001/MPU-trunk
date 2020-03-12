/******************************************************
�ļ�����	CanMsgUL.c

������	
Data			Vasion			author
2019/2/6		V1.0			liujian
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
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include "init.h"
#include "log.h"
#include "diag.h"
#include "fault_sync.h"
#include "gb32960_api.h"
#include "hozon_PP_api.h"
#include "hozon_SP_api.h"
#include "../PrvtProt.h"
#include "com_app_def.h"
#include "dir.h"
#include "file.h"
#include "uds.h"
#include "dev_api.h"
#include "nm_api.h"
#include "can_api.h"
#include "../../support/protocol.h"
#include "cfg_api.h"
#include "udef_cfg_api.h"
#include "CanMsgUL.h"


/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/
static pthread_mutex_t datmtx = PTHREAD_MUTEX_INITIALIZER;
static PP_CanMsgUL_t CanMsgUL;
static char *Pfilepathname = NULL;
static char Sefilepathname[128] = {0};
static PP_CanMsgUL_busStatistic_t CanMsgUL_busS[PP_CANMSGUL_CANBUSNUM];

/*Static function declaration*/
static void *PP_CanMsgUL_main(void);

const PP_CanMsgUL_week_t CanMsgUL_week[7] = 
{
	{"Sun"},
	{"Mon"},
	{"Tue"},
	{"Wed"},
	{"Thur"},
	{"Fri"},
	{"Sat"}
};

const PP_CanMsgUL_Mon_t CanMsgUL_Mon[12] = 
{
	{"Jan"},
	{"Feb"},
	{"Mar"},
	{"Apr"},
	{"May"},
	{"Jun"},
	{"Jul"},
	{"Aug"},
	{"Sep"},
	{"Oct"},
	{"Nov"},
	{"Dec"}
};

static void PP_CanMsgUL_creatNewFile(void);
static void PP_CanMsgUL_canBusStatistic(PP_CanMsgUL_busStatistic_t *busS,char *datout);
static void PP_CanMsgUL_BeginTrig(char *datout);
static void PP_CanMsgUL_logDirStart(float relT,char *datout);
static void PP_CanMsgUL_logTrigEvt(float relT,char *datout);
static void PP_CanMsgUL_logDirStop(float relT,char *datout);
static void PP_CanMsgUL_dataMsgEvt(CanMsg_t *canMsg,char *datout);
static void PP_CanMsgUL_extDtMsgEvt(CanMsg_t *canMsg,char *datout);
static void PP_CanMsgUL_rmtMsgEvt(CanMsg_t *canMsg,char *datout);
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
*InitPP_CanMsgUL_Parameter

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void InitPP_CanMsgUL_Parameter(void)
{
	memset(&CanMsgUL,0,sizeof(PP_CanMsgUL_t));
	memset(&CanMsgUL_busS[0],0,sizeof(PP_CanMsgUL_busStatistic_t));
	memset(&CanMsgUL_busS[1],0,sizeof(PP_CanMsgUL_busStatistic_t));

	CanMsgUL.BaseClctime = tm_get_time();
	CanMsgUL.BaseStatistictime = CanMsgUL.BaseClctime;
}

/******************************************************
*PP_CanMsgUL_run

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
int PP_CanMsgUL_run(void)
{
    int ret;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&tid, &ta, (void *)PP_CanMsgUL_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "can msg file upload pthread create failed, error: %s", strerror(errno));
    }

	return 0;
}

/******************************************************
*PP_CanMsgUL_main

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
static void *PP_CanMsgUL_main(void)
{
	log_o(LOG_HOZON, "can msg file upload thread running");
    prctl(PR_SET_NAME, "CANMSG_UPLOAD");
    while(1)
    {
		unsigned char index;
		int cnt;
		char dataout[256];
		char Lenewfilepathname[128] = {0};
		int fnamelen;
		long relTStamp;
		CanMsg_t *canMsgin;
		long long currClctime;

		if(DIAG_EMMC_OK == flt_get_by_id(EMMC))
		{
		
		if(dir_exists(PP_CANMSGUL_PATH) == 0 &&
			dir_make_path(PP_CANMSGUL_PATH, S_IRUSR | S_IWUSR, false) != 0)
		{
			log_e(LOG_HOZON, "create the path %s fail\n",PP_CANMSGUL_PATH);
		}
		else
		{
				if((1 == gb32960_gbCanbusActiveSt()) && (1 == dev_get_KL15_signal())  && \
						 (0 == get_factory_mode()) && (0 == GetPP_rmtCtrl_fotaUpgrade()))
			{
				if(Pfilepathname == NULL)
				{
					log_i(LOG_HOZON, "\ncurrent timestamp 00: %d\n",tm_get_time());
					PP_CanMsgUL_creatNewFile();
					Pfilepathname = Sefilepathname;
					CanMsgUL.firstframeflag = 1;
					log_i(LOG_HOZON, "\ncurrent timestamp 01: %d\n",tm_get_time());
				}

				for(index = 0;index < PP_CANMSGUL_BUFNUM;index++)
				{
					if(1 == CanMsgUL.buffer[index].successflag)
					{
						log_i(LOG_HOZON, "\ncurrent timestamp 10: %d\n",tm_get_time());
						CanMsgUL.buffer[index].successflag = 0;
						if(1 == CanMsgUL.firstframeflag)
						{
							CanMsgUL.firstframeflag = 0;
							CanMsgUL_busS[0].fsttime = CanMsgUL.buffer[index].msg[0].miscUptime;
							CanMsgUL_busS[1].fsttime = CanMsgUL.buffer[index].msg[0].miscUptime;
						}
						//文件操作--打开文件
						FILE *pfid = fopen(Sefilepathname,"a+");
						for(cnt = 0;cnt < CanMsgUL.buffer[index].cnt;cnt++)
						{
							memset(dataout,0,sizeof(dataout));
							canMsgin = &CanMsgUL.buffer[index].msg[cnt];

							if(!canMsgin->exten)//标准帧
							{
								if(!canMsgin->isRTR)//数据帧
								{
									CanMsgUL_busS[canMsgin->port-1].Dcnt++;
									PP_CanMsgUL_dataMsgEvt(canMsgin,dataout);
								}
								else//远程帧
								{
									CanMsgUL_busS[canMsgin->port-1].Rcnt++;
									PP_CanMsgUL_rmtMsgEvt(canMsgin,dataout);
								}
							}
							else//扩展帧
							{
								if(!canMsgin->isRTR)//数据帧
								{
									CanMsgUL_busS[canMsgin->port-1].XDcnt++;
									PP_CanMsgUL_extDtMsgEvt(canMsgin,dataout);
								}
								else//远程帧
								{}
							}

							//文件操作--格式数据写入文件
							fwrite(dataout,strlen(dataout),1,pfid);
						}
						CanMsgUL_busS[0].lsttime = CanMsgUL.buffer[index].msg[cnt-1].miscUptime;
						CanMsgUL_busS[1].lsttime = CanMsgUL.buffer[index].msg[cnt-1].miscUptime;
						CanMsgUL.buffer[index].cnt = 0;

						currClctime = tm_get_time();
						if((currClctime - CanMsgUL.BaseStatistictime) >= 1000)
						{
							log_i(LOG_HOZON, "\ncurrent timestamp 20: %d\n",tm_get_time());
							CanMsgUL.BaseStatistictime = currClctime;

							memset(dataout,0,sizeof(dataout));
							PP_CanMsgUL_canBusStatistic(&CanMsgUL_busS[0],dataout);
							fwrite(dataout,strlen(dataout),1,pfid);
							memset(dataout,0,sizeof(dataout));
							PP_CanMsgUL_canBusStatistic(&CanMsgUL_busS[1],dataout);
							fwrite(dataout,strlen(dataout),1,pfid);

							log_i(LOG_HOZON, "\ncurrent timestamp 21: %d\n",tm_get_time());
						}

						//文件操作--关闭文件
						fclose(pfid);

						log_i(LOG_HOZON, "\ncurrent timestamp 11: %d\n",tm_get_time());
					}
				}

				currClctime = tm_get_time();
				if((currClctime - CanMsgUL.BaseClctime) >= PP_CANMSGUL_PACKTIME)//采集累计时间1min
				{
					log_i(LOG_HOZON, "\ncurrent timestamp 30: %d\n",tm_get_time());
					CanMsgUL.BaseClctime = currClctime;
					relTStamp = currClctime - CanMsgUL.BaseTstamp;
					//文件操作--打开文件
					FILE *pfid = fopen(Sefilepathname,"a+");

					//文件操作--格式数据写入文件
					memset(dataout,0,sizeof(dataout));
					PP_CanMsgUL_logDirStop((float)relTStamp,dataout);
					fwrite(dataout,strlen(dataout),1,pfid);
					memset(dataout,0,sizeof(dataout));
					PP_CanMsgUL_logTrigEvt((float)relTStamp,dataout);
					fwrite(dataout,strlen(dataout),1,pfid);
					fwrite(PP_CANMSGUL_ENDTRIGBOLCK,sizeof(PP_CANMSGUL_ENDTRIGBOLCK)-1,1,pfid);

					//文件操作--关闭文件
					fclose(pfid);

					CanMsgUL.renameflag = 1;
					log_i(LOG_HOZON, "\ncurrent timestamp 31: %d\n",tm_get_time());
				}
			}
			else
			{
				pthread_mutex_lock(&datmtx);

				memset(&CanMsgUL,0,sizeof(PP_CanMsgUL_t));
				CanMsgUL.BaseClctime = tm_get_time();
				CanMsgUL.BaseStatistictime = CanMsgUL.BaseClctime;
				if(Pfilepathname != NULL)
				{
					CanMsgUL.renameflag = 1;
				}

				pthread_mutex_unlock(&datmtx);
			}

			if(1 == CanMsgUL.renameflag)
			{
				log_i(LOG_HOZON, "\ncurrent timestamp 40: %d\n",tm_get_time());
				CanMsgUL.renameflag = 0;
				//便于上传处理，重新命名文件
				memcpy(Lenewfilepathname,Sefilepathname,128);
				fnamelen = strlen(Lenewfilepathname);
				Lenewfilepathname[fnamelen-5] = '1';
				log_o(LOG_HOZON, "old name:%s\n new name:%s\n",Sefilepathname,Lenewfilepathname);
				rename(Sefilepathname, Lenewfilepathname);
				Pfilepathname = NULL;
				log_i(LOG_HOZON, "\ncurrent timestamp 41: %d\n",tm_get_time());
				}
			}
		}
		
		usleep(PP_CANMSGUL_USSLEEPTIME);
    }

    return NULL;
}


/******************************************************
*PP_CanMsgUL_datacollection

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
void PP_CanMsgUL_datacollection(void *msg)
{
	static uint64_t Sebasetime = 0;
	uint64_t	Lecurrtime;
	CAN_MSG *	canMsg  = msg;
	if((0 == dev_get_KL15_signal()) || (1 == get_factory_mode()) \
									|| (1 == GetPP_rmtCtrl_fotaUpgrade()))
	{
		return;
	}

	Lecurrtime = tm_get_time();

	pthread_mutex_lock(&datmtx);

	CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].miscUptime = Lecurrtime;
	CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].MsgID = canMsg->MsgID;
	CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].len = canMsg->len;
	CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].isRx = canMsg->isRx;
	CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].port = canMsg->port;
	int i;
	for(i = 0;i < canMsg->len;i++)
	{
		CanMsgUL.buffer[CanMsgUL.index].msg[CanMsgUL.buffer[CanMsgUL.index].cnt].Data[i] = canMsg->Data[i];
	}

	CanMsgUL.buffer[CanMsgUL.index].cnt++;
	if(((Lecurrtime - Sebasetime) >= 1000) || \
		(PP_CANMSGUL_MSGNUM_MAX == CanMsgUL.buffer[CanMsgUL.index].cnt))
	{//采集累计时间1s || 缓存满
		log_i(LOG_HOZON, "\ncurrent timestamp 50: %d\n",Sebasetime);
		Sebasetime = Lecurrtime;
		CanMsgUL.buffer[CanMsgUL.index].successflag = 1;
		CanMsgUL.index++;
		log_i(LOG_HOZON, "\ncurrent timestamp 51: %d\n",Lecurrtime);
		log_i(LOG_HOZON, "CanMsgUL.buffer[%d].successflag = %d\n",CanMsgUL.index,\
									CanMsgUL.buffer[CanMsgUL.index].successflag);
	}
	else
	{}

	if(PP_CANMSGUL_BUFNUM == CanMsgUL.index)
	{
		CanMsgUL.index = 0;
	}

	pthread_mutex_unlock(&datmtx);
}

/*
* 生成新的asc格式文件
*/
static void PP_CanMsgUL_creatNewFile(void)
{
	char filename[64] = {0};
	char stringVal[32] = {0};
	char vin[18] = {0};
	uint64_t timestamp;
	char buff[64] = {0};
	
	CanMsgUL.BaseTstamp = tm_get_time();

	gb32960_getvin(vin);
	memcpy(filename,vin,17);
	memcpy(filename + 17,"_",1);
	timestamp = PrvtPro_getTimestamp();
	PP_rmtCfg_ultoa(timestamp,stringVal,10);
	memcpy(filename + 17 + 1,stringVal,strlen(stringVal));
	memcpy(filename + 17 + 1 + strlen(stringVal),"_0",2);
	log_i(LOG_HOZON, "file name = %s\n",filename);

	snprintf(Sefilepathname, sizeof(Sefilepathname), "%s%s%s", PP_CANMSGUL_PATH,filename, ".asc");
	log_i(LOG_HOZON, "file path and name = %s\n",Sefilepathname);

	//文件操作--打开文件
	FILE *pfid = fopen(Sefilepathname,"a+");
	//文件操作--格式数据写入文件
	fwrite(PP_CANMSGUL_HEADER_DATA,sizeof(PP_CANMSGUL_HEADER_DATA)-1,1,pfid);
	fwrite(PP_CANMSGUL_HEADER_BASETS,sizeof(PP_CANMSGUL_HEADER_BASETS)-1,1,pfid);
	fwrite(PP_CANMSGUL_HEADER_VER,sizeof(PP_CANMSGUL_HEADER_VER)-1,1,pfid);
	
	//Begin Triggerblock Mon Mar 7 01:21:51 pm 2005
	memset(buff,0,sizeof(buff));
	PP_CanMsgUL_BeginTrig(buff);
	fwrite(buff,strlen(buff),1,pfid);
	memset(buff,0,sizeof(buff));
	PP_CanMsgUL_logDirStart(0,buff);
	fwrite(buff,strlen(buff),1,pfid);
	memset(buff,0,sizeof(buff));
	PP_CanMsgUL_logTrigEvt(0,buff);
	fwrite(buff,strlen(buff),1,pfid);

	//文件操作--关闭文件
	fclose(pfid);
}


/*
* asc格式--can bus统计
*/// 3.0100 1 Statistic: D 6 R 0 XD 0 XR 0 E 0 O 0 B 0.2%
static void PP_CanMsgUL_canBusStatistic(PP_CanMsgUL_busStatistic_t *busS,char *datout)
{
	int datlen;

	busS->relT = busS->lsttime - CanMsgUL.BaseTstamp;
	busS->Bcnt = (float)(((busS->Dcnt + busS->Rcnt)*11 + (busS->XDcnt + busS->XRcnt)*13) << 3) \
					/ (float)((busS->lsttime - busS->fsttime) << 9);

	sprintf(datout," %6.4f %d Statistic: D %d R %d XD %d XR %d E %d O %d B %4.2f", \
							busS->relT/1000, busS->canP,busS->Dcnt,busS->Rcnt,busS->XDcnt,busS->XRcnt, \
							busS->Ecnt,busS->Ocnt,busS->Bcnt);
	datlen = strlen(datout); 
	memcpy(datout + datlen,"%\r\n",3);
}	

/*
* asc格式--Begin Trigger
*/
static void PP_CanMsgUL_BeginTrig(char *datout)
{
	RTCTIME time;

	tm_get_abstime(&time);
	sprintf(datout," %s %s %s %d %d:%d:%d %d\r\n","Begin Triggerblock", \
				    CanMsgUL_week[time.week].Wday,CanMsgUL_Mon[time.mon - 1].Wmon, \
					time.mday,time.hour,time.min,time.sec,time.year);
}

/*
* asc格式--log direct start
*/
static void PP_CanMsgUL_logDirStart(float relT,char *datout)
{
	sprintf(datout," %6.4f log direct start (0ms)\r\n",relT/1000);
}

/*
* asc格式--log direct stop
*/
static void PP_CanMsgUL_logDirStop(float relT,char *datout)
{
	sprintf(datout," %6.4f log direct stop (0ms)\r\n",relT/1000);
}

/*
* asc格式--log trigger event
*/
static void PP_CanMsgUL_logTrigEvt(float relT,char *datout)
{
	sprintf(datout," %6.4f log trigger event\r\n",relT/1000);
}

/*
* asc格式--can data message event
*///2.5009 1 64 Rx d 8 00 01 02 03 04 05 06 07 
static void PP_CanMsgUL_dataMsgEvt(CanMsg_t *canMsg,char *datout)
{
	int i = 0;
	char dtbuf[200] = {0};
	char pbuf[4] = {0};
	float uptime = 0;

	if(canMsg->miscUptime > CanMsgUL.BaseTstamp)
	{
		uptime  = canMsg->miscUptime - CanMsgUL.BaseTstamp;
	}

	for(i = 0;i < canMsg->len;i++)
	{
		sprintf(pbuf,"%02X ",canMsg->Data[i]);
		strncat((char*)dtbuf,pbuf,3);
	}

	sprintf(datout," %6.4f %d %02X %s d %d %s\r\n",uptime/1000,canMsg->port,canMsg->msgid, \
						(canMsg->isRx?"Rx":"Tx"),canMsg->len,dtbuf);
}

/*
* asc格式--CAN Extended Message Event
*///2.5009 1 64x Rx d 8 00 01 02 03 04 05 06 07 
static void PP_CanMsgUL_extDtMsgEvt(CanMsg_t *canMsg,char *datout)
{
	int i = 0;
	char dtbuf[200] = {0};
	char pbuf[4] = {0};
	float uptime = 0;

	if(canMsg->miscUptime > CanMsgUL.BaseTstamp)
	{
		uptime  = canMsg->miscUptime - CanMsgUL.BaseTstamp;
	}

	for(i = 0;i < canMsg->len;i++)
	{
		sprintf(pbuf,"%02X ",canMsg->Data[i]);
		strncat((char*)dtbuf,pbuf,3);
	}

	sprintf(datout," %6.4f %d %02Xx %s d %d %s\r\n",uptime/1000,canMsg->port,canMsg->msgid, \
						(canMsg->isRx?"Rx":"Tx"),canMsg->len,dtbuf);
}

/*
* asc格式--CAN Remote Frame Event
*///2.5010 1 200 Tx r
static void PP_CanMsgUL_rmtMsgEvt(CanMsg_t *canMsg,char *datout)
{
	float uptime = 0;

	if(canMsg->miscUptime > CanMsgUL.BaseTstamp)
	{
		uptime  = canMsg->miscUptime - CanMsgUL.BaseTstamp;
	}

	sprintf(datout," %6.4f %d %02X %s r\r\n",uptime/1000,canMsg->port,canMsg->msgid, \
						(canMsg->isRx?"Rx":"Tx"));
}


