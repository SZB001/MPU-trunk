/******************************************************
�ļ�����	sockproxy.c
������	����tsp�Խ�socket��·�Ľ������Ͽ�����/�����ݴ���	
Data			Vasion			author
2019/4/17		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include "file.h"
#include "timer.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "dev_api.h"
#include "sock_api.h"
#include "gb32960_api.h"
#include "nm_api.h"
#include "../../support/protocol.h"
#include "hozon_PP_api.h"
#include "hozon_SP_api.h"
#include "pm_api.h"
#include "at.h"
#include "sockproxy_rxdata.h"
#include "sockproxy_txdata.h"
#include "../PrvtProtocol/PrvtProt_queue.h"
#include "hozon_ver_api.h"
#include "../PrvtProtocol/PrvtProt.h"
#include "tboxsock.h"
#include "sockproxy.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


#ifdef HOZON_PRD
#define TSP_URL_IP "172.16.10.204"
#endif

#ifdef HOZON_UAT
#define TSP_URL_IP "172.16.20.237"
#endif

#ifdef HOZON_PRE
#define TSP_URL_IP "60.12.185.130"
#endif

static sockproxy_stat_t sockSt;
static pthread_mutex_t sendmtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
static pthread_mutex_t closemtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
static pthread_mutex_t rcvmtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��

static uint64_t socketopentimer = 0;
static uint64_t socketclosetimer = 0;
static int SP_sockFd;
static int Sg_sockFd;
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void *sockproxy_rcvmain(void);
static void *sockproxy_sendmain(void);
static int 	sockproxy_do_checksock(sockproxy_stat_t *state);
static int 	sockproxy_do_receive(sockproxy_stat_t *state);
static void sockproxy_gbMakeupMsg(uint8_t *data,int len);
static void sockproxy_privMakeupMsg(uint8_t *data,int len);
static int sockproxy_do_send(sockproxy_stat_t *state);
static void *sockproxy_socketmain(void);
static int sockproxy_sgLink(sockproxy_stat_t *state);
static int sockproxy_BDLink(sockproxy_stat_t *state);
static void sockproxy_nm_dial_recall(void);
#ifdef SOCKPROXY_TEST
static void sockproxy_testTask(void);
#endif
static int PP_shell_setPKIenable(int argc, const char **argv);
static int sockproxy_get_link_status(void);
static int sockproxy_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������sockproxy_init
*��  �Σ�void
*����ֵ��void
*��  ������ʼ��
*��  ע��
******************************************************/
int sockproxy_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			memset(&sockSt, 0 , sizeof(sockproxy_stat_t));
			sockSt.state = PP_CLOSED;
			sockSt.rcvType = PP_RCV_UNRCV;
			sockSt.rcvstep = PP_RCV_IDLE;
			sockSt.apnType  = 0;//默认单apn
			SockproxyData_Init();
		}
        break;
        case INIT_PHASE_RESTORE:
		{
			
		}
        break;
        case INIT_PHASE_OUTSIDE:
		{
			/*init SSL*/
			//HzTboxInit();
			unsigned int cfglen;
			unsigned char localapn[32] = {0};
			shell_cmd_register("hozon_pkien", PP_shell_setPKIenable, "set pki en");
			SP_data_init();
			socketopentimer = tm_get_time();
	  		socketclosetimer = tm_get_time();
			cfglen = 1;
			ret |= cfg_get_para(CFG_ITEM_EN_PKI, &sockSt.pkiEnFlag, &cfglen);
			ret |= nm_register_get_ota_status(sockproxy_get_link_status);
			ret |= nm_register_status_changed(sockproxy_nm_callback);

			cfglen = 32;
    		ret = cfg_get_para(CFG_ITEM_LOCAL_APN, localapn, &cfglen);
			if((0 != strlen((char*)localapn)) && (1 == sockSt.pkiEnFlag))
			{
				sockSt.apnType  = 1;
			}
		}
        break;
    }

    return ret;
}

static  pthread_t sockttid;
static  pthread_attr_t socktta;
static	pthread_t 		rcvtid;
static  pthread_attr_t 	rcvta;
static  pthread_t sendtid;
static  pthread_attr_t sendta;

/******************************************************
*��������sockproxy_run
*��  �Σ�void
*����ֵ��void
*��  �������������߳�
*��  ע��
******************************************************/
int sockproxy_run(void)
{
    int ret = 0;

    pthread_attr_init(&socktta);
    pthread_attr_setdetachstate(&socktta, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&sockttid, &socktta, (void *)sockproxy_socketmain, NULL);

    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create socketmain failed, error: %s", strerror(errno));
        return ret;
    }

    pthread_attr_init(&rcvta);
    pthread_attr_setdetachstate(&rcvta, PTHREAD_CREATE_JOINABLE);
    ret = pthread_create(&rcvtid, &rcvta, (void *)sockproxy_rcvmain, NULL);
	log_i(LOG_HOZON, "rcvtid = %d\n",rcvtid);
    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create rcvmain failed, error: %s", strerror(errno));
        return ret;
    }

    pthread_attr_init(&sendta);
    pthread_attr_setdetachstate(&sendta, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&sendtid, &sendta, (void *)sockproxy_sendmain, NULL);

    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create sendmain failed, error: %s", strerror(errno));
        return ret;
    }

	return 0;
}

/******************************************************
*��������sockproxy_rcvmain
*��  �Σ�void
*����ֵ��void
*��  ���������������߳�
*��  ע��
******************************************************/
static void *sockproxy_socketmain(void)
{
	int res = 0;
	log_o(LOG_SOCK_PROXY, "socket proxy  of sockrtmain thread running");
    prctl(PR_SET_NAME, "SOCK_PROXY");

	if(!sockSt.pkiEnFlag)
	{
		if ((sockSt.socket = sock_create("sockproxy", SOCK_TYPE_SYNCTCP)) < 0)
		{
			log_e(LOG_SOCK_PROXY, "create socket failed, thread exit");
			return NULL;
		}
	}

    while (1)
    {
        res = sockproxy_do_checksock(&sockSt);
		usleep(5000);
    }
	(void)res;
	if(!sockSt.pkiEnFlag)
	{
		sock_delete(sockSt.socket);
	}
	else
	{
		if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
		{
			(void)SgHzTboxClose();
		}
		else
		{
			(void)HzTboxClose();
		}
	}
    return NULL;
}

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
/******************************************************
*��������sockproxy_rcvmain
*��  �Σ�void
*����ֵ��void
*��  ���������������߳�
*��  ע��
******************************************************/
static void *sockproxy_rcvmain(void)
{
	int res = 0;
	log_o(LOG_SOCK_PROXY, "socket proxy  of rcvmain thread running");
    prctl(PR_SET_NAME, "SOCK_PROXY_RCV");
	
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);           //允许退出线程
    //pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,   NULL);   //设置立即取消
    while (1)
    {
		pthread_testcancel();
        res = sockproxy_do_receive(&sockSt);
		pthread_testcancel();
    }
    (void)res;
    return NULL;
}

/******************************************************
*��������sockproxy_sendmain
*��  �Σ�void
*����ֵ��void
*��  ���������������߳�
*��  ע��
******************************************************/
static void *sockproxy_sendmain(void)
{
	int res = 0;
	log_o(LOG_SOCK_PROXY, "socket proxy  of sendmain thread running");
    prctl(PR_SET_NAME, "SOCK_PROXY_SEND");

    while (1)
    {
    	res = sockproxy_do_send(&sockSt);
		usleep(2500);
    }

	(void)res;
    return NULL;
}

/******************************************************
*��������sockproxy_socketState
*��  �Σ�
*����ֵ��
*��  ����socket open/colse state
*��  ע��ͬ������
******************************************************/
void sockproxy_socketclose(int type)
{
	if(pthread_mutex_trylock(&closemtx) == 0)
	{
		if(sockSt.state == PP_OPENED)
		{
			log_i(LOG_SOCK_PROXY, "ready to shut down socket");
			sockSt.state = PP_CLOSE_WAIT;//ر�̬
			sockSt.asynCloseFlg = 1;
			sockSt.asynCloseType = type;
			sockSt.closewaittime = tm_get_time();
		}
		pthread_mutex_unlock(&closemtx);
	}
}

/******************************************************
*��������sockproxy_do_checksock
*��  �Σ�void
*����ֵ��void
*��  �������socket����
*��  ע��
******************************************************/
static int sockproxy_do_checksock(sockproxy_stat_t *state)
{
	static uint64_t time = 0;

	if(!sockSt.pkiEnFlag)
	{
		if((1 == sockSt.asynCloseFlg) && (pthread_mutex_lock(&rcvmtx) == 0))
		{
			if(pthread_mutex_trylock(&sendmtx) == 0)//
			{
				if(sockSt.state != PP_CLOSED)
				{
					log_o(LOG_SOCK_PROXY, "socket closed");
					sock_close(sockSt.socket);
					sockSt.state = PP_CLOSED;
					sockSt.tsplinkstatus = 0;
					time = tm_get_time();
				}
				pthread_mutex_unlock(&sendmtx);
				sockSt.asynCloseFlg = 0;
			}

			pthread_mutex_unlock(&rcvmtx);
			return -1;
		}
		
		sockproxy_getURL(&state->sock_addr);
		if(sockproxy_SkipSockCheck() || !state->sock_addr.port || !state->sock_addr.url[0])
		{
			//log_e(LOG_SOCK_PROXY, "state.network = %d",sockproxy_SkipSockCheck());
			return -1;
		}

		switch(state->state)
		{
			case PP_CLOSED:
			{
				if(sock_status(state->socket) == SOCK_STAT_CLOSED)
				{
					if((time == 0) || (tm_get_time() - time > SOCK_SERVR_TIMEOUT))
					{
						log_i(LOG_SOCK_PROXY, "start to connect with server");
						if (sock_open(NM_PUBLIC_NET,state->socket, state->sock_addr.url, state->sock_addr.port) != 0)
						{
							log_e(LOG_SOCK_PROXY, "open socket failed, retry later");
						}

						time = tm_get_time();
					}
				}
				else if(sock_status(state->socket) == SOCK_STAT_OPENED)
				{
					log_i(LOG_SOCK_PROXY, "socket is open success");
					state->state = PP_OPENED;
					sockSt.tsplinkstatus = 1;
					if (sock_error(state->socket) || sock_sync(state->socket))
					{
						log_e(LOG_SOCK_PROXY, "socket error, reset protocol");
						sockproxy_socketclose((int)(PP_SP_COLSE_SP));
					}
				}
				else
				{}
			}
			break;
			default:
			{
				if(sock_status(state->socket) == SOCK_STAT_OPENED)
				{
					if (sock_error(state->socket) || sock_sync(state->socket))
					{
						log_e(LOG_SOCK_PROXY, "socket error, reset protocol");
						sockproxy_socketclose((int)(PP_SP_COLSE_SP + 1));
					}
				}
			}
			break;
		}
	}
	else
	{
		if(1 != sockSt.network)
		{
			log_e(LOG_HOZON, "network is not ok\n");
			sleep(1);
			return -1;
		}

#ifdef HOZON_PRE
	log_e(LOG_HOZON, "pre does not support\n");
	sleep(1);
	return -1;
#endif

		switch(sockSt.linkSt)
		{
			case SOCKPROXY_CHECK_CERT:
			{
				if((0 == sockSt.sleepFlag) && (0 == GetPP_rmtCtrl_fotaUpgrade()))
				{
					if(sockSt.cancelRcvphreadFlag)
					{
						sockSt.cancelRcvphreadFlag = 0;
						log_i(LOG_HOZON, "thread sockproxy_rcvmain not exist\n");
						pthread_attr_init(&rcvta);
						pthread_attr_setdetachstate(&rcvta, PTHREAD_CREATE_JOINABLE);
						pthread_create(&rcvtid, &rcvta, (void *)sockproxy_rcvmain, NULL);
						log_i(LOG_HOZON, "rcvtid = %d\n",rcvtid);
					}

					if(GetPP_CertDL_allowBDLink() == 0)//
					{//建立单向连接
						log_o(LOG_HOZON, "Cert inValid,set up sglink\n");
						sockSt.waittime = tm_get_time();
						sockSt.sglinkSt =  SOCKPROXY_SGLINK_INIT;
						sockSt.linkSt = SOCKPROXY_SETUP_SGLINK;
					}
					else
					{//建立双向连接
						log_o(LOG_HOZON, "Cert Valid,set up BDLlink\n");
						sockSt.waittime = tm_get_time();
						sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
						sockSt.linkSt = SOCKPROXY_SETUP_BDLLINK;
					}
				}
			}
			break;
			case SOCKPROXY_SETUP_SGLINK:
			{
				if(sockproxy_sgLink(state) < 0)
				{//sg 退出
					sockSt.linkSt = SOCKPROXY_CHECK_CERT;
				}
				else
				{}
			}
			break;
			case SOCKPROXY_SETUP_BDLLINK:
			{
				if(sockproxy_BDLink(state) < 0)
				{//BDL 退出
					sockSt.linkSt = SOCKPROXY_CHECK_CERT;
				}
				else
				{}
			}
			break;
			default:
			break;
		}
	}

	static uint64_t	checkclosedelaytime;
	if(sockSt.pkiEnFlag)
	{
		if(sockSt.state == PP_CLOSE_WAIT)
		{
			if((tm_get_time() - checkclosedelaytime) >= SOCK_CHECKCLOSEDTIMEOUT)
			{
				if(pthread_mutex_lock(&sendmtx) == 0)
				{
					void *ret=NULL;
					int retcancel;
					log_i(LOG_HOZON, "rcvtid = %d\n",rcvtid);
					retcancel = pthread_cancel(rcvtid);
					pthread_join(rcvtid, &ret);					
					log_i(LOG_HOZON, "thread sockproxy_rcvmain cancel = %d\n",retcancel);
					sockSt.cancelRcvphreadFlag = 1;
					sockSt.asynCloseFlg = 0;
					sockSt.rcvflag = 0;
					log_o(LOG_SOCK_PROXY, "socket closed\n");
					if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
					{
						(void)SgHzTboxClose();
					}
					else
					{
						(void)HzTboxClose();
					}
					sockSt.state = PP_CLOSED;
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.linkSt = SOCKPROXY_CHECK_CERT;
					log_i(LOG_HOZON, "#####################tm_get_time() = %d\n",tm_get_time());
					sleep(1);
					pthread_mutex_unlock(&sendmtx);//解锁
				}
			}
		}
		else
		{
			checkclosedelaytime = tm_get_time();
		}
	}

	if(0 == dev_get_KL15_signal())
	{
		if((1 == gb32960_gbLogoutSt()) && (GetPrvtProt_Sleep()))
		{
			if(sockSt.state == PP_OPENED)
			{
				//if((tm_get_time() - sockSt.sleepwaittime) > 3000)
				{
					//log_i(LOG_HOZON, "start to sleep\n");
					sockSt.sleepwaittimeoutcnt = 0;
					sockSt.sleepFlag = 1;
				}
			}
			else
			{
				if(sockSt.sleepwaittimeoutcnt < 3)
				{
					if((tm_get_time() - sockSt.sleepwaittime) > 15000)
					{
						sockSt.sleepwaittimeoutcnt++;
						sockSt.sleepFlag = 1;
					}
					else
					{
						sockSt.sleepFlag = 0;
					}
				}
				else
				{
					sockSt.sleepFlag = 1;
				}
			}
		}
		else
		{
			sockSt.sleepwaittime = tm_get_time();
		}
	}
	else
	{
		sockSt.sleepwaittime = tm_get_time();
		sockSt.sleepFlag = 0;
		sockSt.sleepwaittimeoutcnt = 0;
	}

	sockproxy_nm_dial_recall();//重新拨号
#ifdef SOCKPROXY_TEST
	sockproxy_testTask();
#endif
    return 0;
}

/******************************************************
*��������sockproxy_sgLink
*��  �Σ�void
*����ֵ��void
*��  �������socket����
*��  ע��
******************************************************/
static int sockproxy_sgLink(sockproxy_stat_t *state)
{
	int iRet = 0;
	char	OnePath[64]="\0";
	char	ScdPath[64]="\0";

	switch(sockSt.sglinkSt)
	{
		case SOCKPROXY_SGLINK_INIT:
		{
			if((tm_get_time() - sockSt.waittime) >= 1000)
			{
				sockSt.tsplinkstatus = 0;
				sockSt.sglinkSt = SOCKPROXY_SGLINK_CREAT;
				sockSt.waittime = tm_get_time();
			}
		}
		break;
		case SOCKPROXY_SGLINK_CREAT:
		{
			if(sockSt.state == PP_CLOSED)
			{
				#if 0
				getPP_rmtCfg_certAddrPort(sockSt.sgLinkAddr,&sockSt.sgPort);
				if((sockSt.sgLinkAddr[0] == 0) || (sockSt.sgPort == 0))
				{
					log_e(LOG_SOCK_PROXY,"invalid addr %s or port %d+++++++\n", \
											sockSt.sgLinkAddr,sockSt.sgPort);
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}
				#endif
				/*port ipaddr*/
				if(sockSt.apnType != 1)
				{
					#ifdef HOZON_UAT
					{	
						iRet = HzPortAddrCft(22000, 1,"47.102.130.222",NULL);
					}
					#endif
				}
				else
				{
					iRet = HzPortAddrCft(22000, 1,TSP_URL_IP,NULL);//TBOX端口地址配置初始化
				}

				if(iRet != SOCKPROXY_SG_ADDR_INIT_SUCCESS)
				{
					log_e(LOG_SOCK_PROXY,"HzPortAddrCft error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				/*create random string*/
				sprintf(OnePath, "%s",PP_CERTDL_HOZONCACER_PATH);
				sprintf(ScdPath, "%s",PP_CERTDL_TSPCACER_PATH);

				iRet = SgHzTboxCertchainCfg(OnePath, ScdPath);
				if(iRet != SOCKPROXY_SG_CCIC_SUCCESS)
				{
					log_e(LOG_SOCK_PROXY,"SgHzTboxCertchainCfg +++++++++++++++iRet[%d] \n", iRet);
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				/*init SSL*/
				if(access(PP_CERTDL_TBOXCRL,F_OK) != 0)//文件不存在
				{
					int fd = file_create(PP_CERTDL_TBOXCRL, 0644);
					if(fd < 0)
					{
						log_e(LOG_SOCK_PROXY,"creat file /usrdata/pem/tbox.crl fail\n");
						sleep(1);
						return -1;
					}

					close(fd);
				}

				iRet = SgHzTboxInit(PP_CERTDL_TBOXCRL);
				if(iRet != SOCKPROXY_SG_INIT_SUCCESS)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxInit error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}
				SgHzTboxSocketFd(&Sg_sockFd);
				
				if(sockSt.apnType == 1)
				{
					struct ifreq nif;
					memset(&nif,0x00,sizeof(nif));
    				char *inface = "rmnet_data0";
					strcpy(nif.ifr_name, inface);
					/* 绑定接口 */
					if (setsockopt(Sg_sockFd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&nif, sizeof(nif)) < 0)
					{
						printf("bind interface fail, errno: %d \r\n", errno);
						sleep(1);
						return -1;		
					}
					else
					{
						printf("setsockopt interface success \r\n");
					}
				}
				/*Initiate a connection server request*/
				iRet = SgHzTboxConnect(Sg_sockFd);
				if(iRet != SOCKPROXY_SG_CONN_SUCCESS)
				{
					log_e(LOG_SOCK_PROXY,"SgHzTboxConnect error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				log_o(LOG_HOZON, "set up sglink success\n");
				sockSt.state = PP_OPENED;
				sockSt.tsplinkstatus = 1;
				setPrvtProt_sendHeartbeat();
			}
			else
			{
				if(sockSt.state == PP_OPENED)
				{
					return 0;
				}

				if(sockSt.asynCloseFlg == 1)
				{
					sockSt.asynCloseFlg = 0;
					setPrvtProt_sendHeartbeat();
					log_i(LOG_SOCK_PROXY, "sockSt.asynCloseFlg == 1 ,start to close sg socket\n");
					sockSt.sglinkSt = SOCKPROXY_SGLINK_CLOSE;
				}
			}
		}
		break;
		case SOCKPROXY_SGLINK_CLOSE:
		{
			/*release all resources and close all connections*/
			if((0 == sockSt.rcvflag) && (pthread_mutex_trylock(&sendmtx) == 0))//
			{
				sockSt.waittime = tm_get_time();
				sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
				if(sockSt.state != PP_CLOSED)
				{
					log_o(LOG_SOCK_PROXY, "close sg socket\n");
					SgHzTboxClose();
					sockSt.state = PP_CLOSED;
				}
				pthread_mutex_unlock(&sendmtx);

				return -2;
			}
			else
			{
				log_i(LOG_SOCK_PROXY, "wait close socket\n");
			}
		}
		break;
		default:
		break;
	}

    return 0;
}



/******************************************************
*��������sockproxy_BDLink
*��  �Σ�void
*����ֵ��void
*��  �������socket����
*��  ע��
******************************************************/
static int sockproxy_BDLink(sockproxy_stat_t *state)
{
	int 	iRet = 0;
	char	OnePath[64]="\0";
	char	ScdPath[64]="\0";
	//char 	destIP[32];

	switch(sockSt.BDLlinkSt)
	{
		case SOCKPROXY_BDLLINK_INIT:
		{
			sockSt.tsplinkstatus = 0;
			sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_CREAT;
			sockSt.waittime = tm_get_time();
		}
		break;
		case SOCKPROXY_BDLLINK_CREAT:
		{
			char	UsCertPath[64]="\0";
			char	UsKeyPath[64]="\0";

			if(sockSt.state == PP_CLOSED)
			{
				#if 0
				getPP_rmtCfg_tspAddrPort(sockSt.BDLLinkAddr,&sockSt.BDLPort);
				if((sockSt.BDLLinkAddr[0] == 0) || (sockSt.BDLPort == 0))
				{
					log_e(LOG_SOCK_PROXY,"invalid addr %s or port %d +++++++\n",\
										sockSt.BDLLinkAddr,sockSt.BDLPort);
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}
				#endif
				/*port ipaddr*/
				if(sockSt.apnType != 1)
				{
					#ifdef HOZON_UAT
					iRet = HzPortAddrCft(21000, 1,"47.102.130.222",NULL);
					#endif
				}
				else
				{
					iRet = HzPortAddrCft(21000, 1,TSP_URL_IP,NULL);
				}
				if(iRet != 1010)
				{
					log_e(LOG_SOCK_PROXY,"HzPortAddrCft error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				/*create random string*/
				sprintf(OnePath, "%s",PP_CERTDL_HOZONCACER_PATH);
				sprintf(ScdPath, "%s",PP_CERTDL_TSPCACER_PATH);
				sprintf(UsCertPath, "%s",PP_CERTDL_CERTPATH);//申请的证书，要跟userAuth.key匹配使用
				sprintf(UsKeyPath, "%s",PP_CERTDL_TWOCERTKEYPATH);
				iRet = HzTboxCertchainCfg(OnePath, ScdPath, UsCertPath, UsKeyPath);
				if(iRet != 2030)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxCertchainCfg error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				/*init SSL*/
				if(access(PP_CERTDL_TBOXCRL,F_OK) != 0)//文件不存在
				{
					int fd = file_create(PP_CERTDL_TBOXCRL, 0644);
					if(fd < 0)
					{
						log_e(LOG_SOCK_PROXY,"creat file /usrdata/pem/tbox.crl fail\n");
						sleep(1);
						return -1;
					}

					close(fd);
				}

				char vin[18] = {0};
				gb32960_getvin(vin);
				iRet = HzTboxSetVin(vin);
				if(0 != iRet)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxSetVin error+++++++++++++++iRet[%d]\n",iRet);
					sleep(1);
					return -1;
				}

				iRet = HzTboxInit(PP_CERTDL_TBOXCRL);
				if(iRet != 1151)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxInit error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				/*Initiate a connection server request*/
				HzTboxSocketFd(&SP_sockFd);
				if(sockSt.apnType == 1)
				{
					struct ifreq nif;
					memset(&nif,0x00,sizeof(nif));
    				char *inface = "rmnet_data0";
					strcpy(nif.ifr_name, inface);
					/* 绑定接口 */
					if (setsockopt(SP_sockFd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&nif, sizeof(nif)) < 0)
					{
						printf("bind interface fail, errno: %d \r\n", errno);
						sleep(1);
						return -1;		
					}
					else
					{
						printf("setsockopt interface success \r\n");
					}
				}
				iRet = HzTboxConnect(SP_sockFd);
				if(iRet != 1230)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxConnect error+++++++++++++++iRet[%d] \n", iRet);
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
					sockSt.waittime = tm_get_time();
					sleep(1);
					return -1;
				}

				log_o(LOG_HOZON, "set up BDLlink success\n");
				sockSt.state = PP_OPENED;
				sockSt.tsplinkstatus = 1;
				setPrvtProt_sendHeartbeat();
				//sockSt.waittime = tm_get_time();
			}
			else
			{
				if(sockSt.state == PP_OPENED)
				{
					return 0;
				}

				if(1 == sockSt.asynCloseFlg)
				{
					sockSt.asynCloseFlg = 0;
					setPrvtProt_sendHeartbeat();
					log_i(LOG_SOCK_PROXY, "sockSt.asynCloseFlg == 1 ,start to close socket\n");
					sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_CLOSE;
				}
			}
		}
		break;
		case SOCKPROXY_BDLLINK_CLOSE:
		{
			if((0 == sockSt.rcvflag) && (pthread_mutex_trylock(&sendmtx) == 0))//
			{
				if(sockSt.state != PP_CLOSED)
				{
					log_o(LOG_SOCK_PROXY, "bdl socket closed\n");
					(void)HzTboxClose();
					sockSt.state = PP_CLOSED;
				}

				sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
				pthread_mutex_unlock(&sendmtx);

				return -2;
			}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������sockproxy_do_receive
*��  �Σ�void
*����ֵ��void
*��  ������������
*��  ע��
******************************************************/
static int sockproxy_do_receive(sockproxy_stat_t *state)
{
    int ret = 0, rlen = 0;
    char rbuf[1456] = {0};

    if(state->state == PP_OPENED)
	{
    	sockSt.rcvflag = 1;
		if(!sockSt.pkiEnFlag)
		{
			if(pthread_mutex_lock(&rcvmtx) == 0)
			{
				if ((rlen = sock_recv(state->socket, (uint8_t*)rbuf, sizeof(rbuf))) < 0)
				{
					log_e(LOG_SOCK_PROXY, "socket recv error: %s", strerror(errno));
					log_e(LOG_SOCK_PROXY, "socket recv error, reset protocol");
					sockproxy_socketclose((int)(PP_SP_COLSE_SP + 3));
					pthread_mutex_unlock(&rcvmtx);
					return -1;
				}
				pthread_mutex_unlock(&rcvmtx);
			}
		}
		else
		{
			if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
			{
				ret = SgHzTboxDataRecv(rbuf, 1456, &rlen);
				if(ret != 1275)
				{
					log_e(LOG_SOCK_PROXY,"SgHzTboxDataRecv error+++++++++++++++iRet[%d] [%d]\n", ret, rlen);
					sockproxy_socketclose((int)(PP_SP_COLSE_SP + 8));
					sockSt.rcvflag = 0;
					return -1;
				}
			}
			else
			{
				ret = HzTboxDataRecv(rbuf, 1456, &rlen);
				if(ret != 1275)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxDataRecv error+++++++++++++++iRet[%d] [%d]\n", ret, rlen);
					sockproxy_socketclose((int)(PP_SP_COLSE_SP + 9));
					sockSt.rcvflag = 0;	
					return -1;
				}
			}
		}

		protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_RCV", (uint8_t*)rbuf, rlen, 0);

#if SOCKPROXY_SHELL_PROTOCOL
		while (ret == 0 && rlen > 0)
		{
			int uselen, type, ack, dlen;

			if (gb_makeup_pack(state, input, rlen, &uselen) != 0)
			{
				break;
			}

			rlen  -= uselen;
			input += uselen;
		}
#else

		switch(sockSt.rcvType)
		{
			case PP_RCV_UNRCV:
			{
				sockSt.rcvstep = PP_RCV_IDLE;
				sockSt.rcvlen = 0;
				if((0x23 == rbuf[0]) && (0x23 == rbuf[1]))
				{
					sockSt.rcvType = PP_RCV_GB;
					sockproxy_gbMakeupMsg((uint8_t*)rbuf,rlen);

				}
				else if((0x2A == rbuf[0]) && (0x2A == rbuf[1]))
				{
					sockSt.rcvType = PP_RCV_PRIV;
					sockproxy_privMakeupMsg((uint8_t*)rbuf,rlen);
				}
				else
				{
					if(rlen > 0)
					{
						log_e(LOG_SOCK_PROXY, "sockproxy_do_receive unknow package");
					}
				}
			}
			break;
			case PP_RCV_GB:
			{
				sockproxy_gbMakeupMsg((uint8_t*)rbuf,rlen);
			}
			break;
			case PP_RCV_PRIV:
			{
				sockproxy_privMakeupMsg((uint8_t*)rbuf,rlen);
			}
			break;
			default:
			break;
		}

#endif
		sockSt.rcvflag = 0;
	}

    return ret;
}


/******************************************************
*��������sockproxy_do_send
*��  �Σ�void
*����ֵ��void
*��  ������������
*��  ע��
******************************************************/
static int sockproxy_do_send(sockproxy_stat_t *state)
{
	SP_Send_t *rpt;
	char pakgtype;
	int res = 0;
	if ((rpt = SP_data_get_pack()) != NULL)
	{
		if(rpt->Inform_cb_para != NULL)
		{
			PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)(rpt->Inform_cb_para);
			pakgtype = TxInform_ptr->pakgtype;
			log_i(LOG_HOZON, "send %s to tsp\n",TxInform_ptr->description);
			switch(pakgtype)
			{
				case PP_TXPAKG_SIGTIME://����ʱЧ��
				{
					if((tm_get_time() - TxInform_ptr->eventtime) < SOCK_TXPAKG_OUTOFTIME)//����δ����
					{
						res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
						if (res < 0)
						{
							log_e(LOG_HOZON, "socket send error, reset protocol");
							SP_data_put_send(rpt);
							if(rpt->SendInform_cb != NULL)
							{
								TxInform_ptr->successflg = PP_TXPAKG_FAIL;
								TxInform_ptr->failresion = PP_TXPAKG_TXFAIL;
								TxInform_ptr->txfailtime = tm_get_time();
								rpt->SendInform_cb(rpt->Inform_cb_para);
							}
							sockproxy_socketclose((int)(PP_SP_COLSE_SP + 4));//by liujian 20190510
							sleep(1);
						}
						else if(res == 0)
						{
							//log_e(LOG_HOZON, "send wait, send is canceled");
							SP_data_put_back(rpt);
						}
						else
						{//���ͳɹ�
							SP_data_put_send(rpt);
							if(rpt->SendInform_cb != NULL)
							{
								TxInform_ptr->successflg = PP_TXPAKG_SUCCESS;
								rpt->SendInform_cb(rpt->Inform_cb_para);
							}
						}
					}
					else//���Ĺ���
					{
						log_e(LOG_HOZON, "package past due\n");
						SP_data_put_send(rpt);
						TxInform_ptr->successflg = PP_TXPAKG_FAIL;
						TxInform_ptr->failresion = PP_TXPAKG_OUTOFDATE;
						TxInform_ptr->txfailtime = tm_get_time();
						rpt->SendInform_cb(rpt->Inform_cb_para);
					}
				}
				break;
				case PP_TXPAKG_SIGTRIG:
				{
					res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
					if (res < 0)
					{
						log_e(LOG_HOZON, "socket send error, reset protocol\n");
						SP_data_put_send(rpt);
						if(rpt->SendInform_cb != NULL)
						{
							TxInform_ptr->successflg = PP_TXPAKG_FAIL;
							TxInform_ptr->failresion = PP_TXPAKG_TXFAIL;
							TxInform_ptr->txfailtime = tm_get_time();
							rpt->SendInform_cb(rpt->Inform_cb_para);
						}
						sockproxy_socketclose((int)(PP_SP_COLSE_SP + 5));//by liujian 20190510
						sleep(1);
					}
					else if(res == 0)
					{
						//log_e(LOG_HOZON, "send is canceled\n");
						SP_data_put_send(rpt);
						if(rpt->SendInform_cb != NULL)
						{
							TxInform_ptr->successflg = PP_TXPAKG_FAIL;
							TxInform_ptr->failresion = PP_TXPAKG_TXFAIL;
							TxInform_ptr->txfailtime = tm_get_time();
							rpt->SendInform_cb(rpt->Inform_cb_para);
						}
					}
					else
					{//���ͳɹ�
						SP_data_put_send(rpt);
						if(rpt->SendInform_cb != NULL)
						{
							TxInform_ptr->successflg = PP_TXPAKG_SUCCESS;
							rpt->SendInform_cb(rpt->Inform_cb_para);
						}
					}
				}
				break;
				case PP_TXPAKG_CONTINUE:
				{
					res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
					if (res < 0)
					{
						log_e(LOG_HOZON, "socket send error, reset protocol");
						if(rpt->SendInform_cb != NULL)
						{
							TxInform_ptr->successflg = PP_TXPAKG_FAIL;
							TxInform_ptr->failresion = PP_TXPAKG_TXFAIL;
							TxInform_ptr->txfailtime = tm_get_time();
							rpt->SendInform_cb(rpt->Inform_cb_para);
						}
						SP_data_put_back(rpt);
						sockproxy_socketclose((int)(PP_SP_COLSE_SP + 6));//by liujian 20190510
						sleep(1);
					}
					else if(res == 0)
					{
						//log_e(LOG_HOZON, "send wait, send is canceled");
						SP_data_put_back(rpt);
					}
					else
					{//���ͳɹ�
						SP_data_put_send(rpt);
						if(rpt->SendInform_cb != NULL)
						{
							TxInform_ptr->successflg = PP_TXPAKG_SUCCESS;
							rpt->SendInform_cb(rpt->Inform_cb_para);
						}
					}
				}
				break;
				default:
				break;
			}
		}
		else
		{
			res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
			if (res < 0)
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				SP_data_put_send(rpt);
				if(rpt->SendInform_cb != NULL)
				{
					rpt->SendInform_cb(rpt->Inform_cb_para);
				}
				sockproxy_socketclose((int)(PP_SP_COLSE_SP + 7));//by liujian 20190510
				sleep(1);
			}
			else if(res == 0)
			{
				//log_e(LOG_HOZON, "send wait, send is canceled");
				SP_data_put_back(rpt);
			}
			else
			{
				SP_data_put_send(rpt);
				if(rpt->SendInform_cb != NULL)
				{
					rpt->SendInform_cb(rpt->Inform_cb_para);
				}
			}
		}
	}

	return 0;
}

/******************************************************
*��������sockproxy_MsgSend
*��  �Σ�
*����ֵ��
*��  �������ݷ���
*��  ע��
******************************************************/
int sockproxy_MsgSend(uint8_t* msg,int len,void (*sync)(void))
{
	int res = 0;

	if(pthread_mutex_lock(&sendmtx) == 0)
	{
		if((sockSt.state == PP_OPENED) || (sockSt.state == PP_CLOSE_WAIT))
		{
			protocol_dump(LOG_SOCK_PROXY, "sending data", msg, len, 1);
			if(!sockSt.pkiEnFlag)
			{
				res = sock_send(sockSt.socket, msg, len, sync);
				if((res > 0) && (res != len))
				{
					res = 0;
				}
			}
			else
			{
				SP_data_ack_pack();
				if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
				{
					log_i(LOG_SOCK_PROXY, "<<<<< SgHzTboxDataSend <<<<<");
					res = SgHzTboxDataSend((char*)msg,len);
					log_i(LOG_SOCK_PROXY, ">>>>> SgHzTboxDataSend >>>>>");
					if(res != 1260)
					{
						log_e(LOG_SOCK_PROXY,"SgHzTboxDataSend error+++++++++++++++iRet[%d] \n", res);
						pthread_mutex_unlock(&sendmtx);//解锁
						return -1;
					}
				}
				else
				{
					log_i(LOG_SOCK_PROXY, "<<<<< HzTboxDataSend <<<<<");
					res = HzTboxDataSend((char*)msg,len);
					log_i(LOG_SOCK_PROXY, ">>>>> HzTboxDataSend >>>>>");
					if(res != 1260)
					{
						log_e(LOG_SOCK_PROXY,"HzTboxDataSend error+++++++++++++++iRet[%d] \n", res);
						pthread_mutex_unlock(&sendmtx);//解锁
						return -1;
					}
				}
			}
		}
		else
		{
			//log_e(LOG_SOCK_PROXY, "socket is not open");
		}
		
		pthread_mutex_unlock(&sendmtx);//解锁
	}

	return res;
}

/******************************************************
*��������sockproxy_socketState
*��  �Σ�
*����ֵ��
*��  ����双向链路socket open/colse state
*��  ע��
******************************************************/
int sockproxy_socketState(void)
{
	if(!sockSt.pkiEnFlag)
	{
		if((sockSt.state == PP_OPENED) && \
			(sock_status(sockSt.socket) == SOCK_STAT_OPENED))
		{
			return 1;
		}
	}
	else
	{
		if(((sockSt.state == PP_OPENED) || (sockSt.state == PP_CLOSE_WAIT))&& \
			(sockSt.linkSt == SOCKPROXY_SETUP_BDLLINK))
		{
			return 1;
		}
	}

	return 0;
}

/******************************************************
*��������sockproxy_sgsocketState
*��  �Σ�
*����ֵ��
*��  ����单向链路socket open/colse state
*��  ע��
******************************************************/
int sockproxy_sgsocketState(void)
{
	if(!sockSt.pkiEnFlag)
	{
		return 0;
	}
	else
	{
		if(((sockSt.state == PP_OPENED) || (sockSt.state == PP_CLOSE_WAIT)) && \
				(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK))
		{
			return 1;
		}
	}

	return 0;
}

/******************************************************
*������:Setsocketproxy_Awaken

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void Setsocketproxy_Awaken(void)
{
	sockSt.sleepFlag = 0;
}

/******************************************************
*��������sockproxy_Sleep
*��  �Σ�
*����ֵ��
*��  ����
*��  ע��
******************************************************/
char sockproxy_Sleep(void)
{

	return sockSt.sleepFlag;
}

/******************************************************
*��������sockproxy_gbMakeupMsg
*��  �Σ�
*����ֵ��
*��  ����gbЭ���������
*��  ע��
******************************************************/
static void sockproxy_gbMakeupMsg(uint8_t *data,int len)
{
	int rlen = 0;
	while(len--)
	{
		switch(sockSt.rcvstep)
		{
			case PP_GB_RCV_IDLE:
			{
				sockSt.rcvstep =PP_GB_RCV_SIGN;
				sockSt.rcvlen = 0;
			}
			break;
			case PP_GB_RCV_SIGN:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				sockSt.rcvstep = PP_GB_RCV_CTRL;
			}
			break;
			case PP_GB_RCV_CTRL:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if(22 == sockSt.rcvlen)
				{
					sockSt.rcvstep = PP_GB_RCV_DATALEN;
				}
			}
			break;
			case PP_GB_RCV_DATALEN:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if(24 == sockSt.rcvlen)
				{
					sockSt.datalen = sockSt.rcvbuf[22]*256 + sockSt.rcvbuf[23];
					if((sockSt.datalen + 24) < SOCK_PROXY_RCVLEN)
					{
						sockSt.rcvstep = PP_GB_RCV_DATA;
					}
					else
					{
						sockSt.rcvstep = PP_RCV_IDLE;
						//sockSt.rcvlen = 0;
						sockSt.rcvType = PP_RCV_UNRCV;
						return;
					}
				}
			}
			break;
			case PP_GB_RCV_DATA:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if((24 + sockSt.datalen) == sockSt.rcvlen)
				{
					sockSt.rcvstep = PP_GB_RCV_CHECKCODE;
				}
			}
			break;
			case PP_GB_RCV_CHECKCODE:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if(WrSockproxyData_Queue(SP_GB,sockSt.rcvbuf,sockSt.rcvlen) < 0)
				{
					 log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_GB,rcvbuf,rlen) error");
				}
				sockSt.rcvstep = PP_RCV_IDLE;
				sockSt.rcvType = PP_RCV_UNRCV;
				protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_GB_RCV", sockSt.rcvbuf, sockSt.rcvlen, 0);//��ӡgb���յ�����
			}
			break;
			default:
			{
				sockSt.rcvstep = PP_RCV_IDLE;
				sockSt.rcvlen = 0;
				sockSt.rcvType = PP_RCV_UNRCV;
			}
			break;
		}
	}
}

/******************************************************
*��������sockproxy_privMakeupMsg
*��  �Σ�
*����ֵ��
*��  ������ҵ˽��Э���������
*��  ע��
******************************************************/
static void sockproxy_privMakeupMsg(uint8_t *data,int len)
{
	int rlen = 0;
	while(len--)
	{
		switch(sockSt.rcvstep)
		{
			case PP_PRIV_RCV_IDLE:
			{
				sockSt.rcvstep = PP_PRIV_RCV_SIGN;
				sockSt.rcvlen = 0;
			}
			break;
			case PP_PRIV_RCV_SIGN:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				sockSt.rcvstep = PP_PRIV_RCV_CTRL;
			}
			break;
			case PP_PRIV_RCV_CTRL:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if(18 == sockSt.rcvlen)
				{
					sockSt.datalen = (((long)sockSt.rcvbuf[10]) << 24) + (((long)sockSt.rcvbuf[11]) << 16) + \
									 (((long)sockSt.rcvbuf[12]) << 8) + ((long)sockSt.rcvbuf[13]);
					if(sockSt.datalen == 18)//message data length
					{
						if(WrSockproxyData_Queue(SP_PRIV,sockSt.rcvbuf,sockSt.rcvlen) < 0)
						{
							log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
						}
						sockSt.rcvstep = PP_RCV_IDLE;
						sockSt.rcvType = PP_RCV_UNRCV;
						//protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_PRIV_RCV",sockSt.rcvbuf, sockSt.rcvlen, 0);//��ӡ˽��Э����յ�����

					}
					else if(sockSt.datalen > SOCK_PROXY_RCVLEN)
					{
						sockSt.rcvstep = PP_RCV_IDLE;
						//sockSt.rcvlen = 0;
						sockSt.rcvType = PP_RCV_UNRCV;
						return;
					}
					else
					{
						sockSt.rcvstep = PP_PRIV_RCV_DATA;
					}
				}
			}
			break;
			case PP_PRIV_RCV_DATA:
			{
				sockSt.rcvbuf[sockSt.rcvlen++] = data[rlen++];
				if((sockSt.datalen) == sockSt.rcvlen)
				{
					if(PP_OPERATETYPE_CERTDL == sockSt.rcvbuf[9])//收到证书下载包
					{
						WrPP_BigBuf_queue(PP_BUGBUF_CERT_DL,sockSt.rcvbuf,sockSt.rcvlen);
					}
					else
					{
						if(WrSockproxyData_Queue(SP_PRIV,sockSt.rcvbuf,sockSt.rcvlen) < 0)
						{
							log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
						}
					}
					sockSt.rcvstep = PP_RCV_IDLE;
					sockSt.rcvType = PP_RCV_UNRCV;
				}
			}
			break;
			default:
			{
				sockSt.rcvstep = PP_RCV_IDLE;
				sockSt.rcvlen = 0;
				sockSt.rcvType = PP_RCV_UNRCV;
			}
			break;
		}
	}
}

/*
*	重新拨号
*/
static void sockproxy_nm_dial_recall(void)
{
	static char IGNnewSt,IGNoldSt = 0;

	IGNnewSt = dev_get_KL15_signal();
	if(IGNoldSt != IGNnewSt)
	{
		IGNoldSt = IGNnewSt;
		if(1 == IGNnewSt)//IGN ON
		{
			sockSt.recalltimer = tm_get_time();
		}
	}

	if(1 == IGNnewSt)//IGN ON
	{
		if(sockproxy_SkipSockCheck())
		{
			if((tm_get_time() - sockSt.recalltimer) >= 30000)
			{
				nm_dial_restart();
				sockSt.recalltimer = tm_get_time();
			}
		}
		else
		{
			sockSt.recalltimer = tm_get_time();
		}
	}
}

/*
*	显示参数
*/
void sockproxy_showParameters(void)
{
	log_o(LOG_SOCK_PROXY, "sockSt.asynCloseFlg = %d\n",sockSt.asynCloseFlg);
	log_o(LOG_SOCK_PROXY, "sockSt.asynCloseType = %d\n",sockSt.asynCloseType);
	log_o(LOG_SOCK_PROXY, "sockSt.rcvflag = %d\n",sockSt.rcvflag);
	log_o(LOG_SOCK_PROXY, "sockSt.sleepFlag = %d\n",sockSt.sleepFlag);
	log_o(LOG_SOCK_PROXY, "sockSt.state = %d\n",sockSt.state);
	log_o(LOG_SOCK_PROXY, "sockSt.BDLlinkSt = %d\n",sockSt.BDLlinkSt);
	log_o(LOG_SOCK_PROXY, "sockSt.sglinkSt = %d\n",sockSt.sglinkSt);
	log_o(LOG_SOCK_PROXY, "sockSt.BDLLinkAddr = %s\n",sockSt.BDLLinkAddr);
	log_o(LOG_SOCK_PROXY, "sockSt.BDLPort = %d\n",sockSt.BDLPort);
	log_o(LOG_SOCK_PROXY, "sockSt.sgLinkAddr = %s\n",sockSt.sgLinkAddr);
	log_o(LOG_SOCK_PROXY, "sockSt.sgPort = %d\n",sockSt.sgPort);
	log_o(LOG_SOCK_PROXY, "sockSt.pkiEnFlag = %d\n",sockSt.pkiEnFlag);
}

#ifdef SOCKPROXY_TEST
/*
*	3G/4G制式切换测试
*/
static void sockproxy_testTask(void)
{
	int ret;
	static uint32_t switchtimes = 0;
	static char if_4GFlag = 1;
	if(sockSt.state == PP_OPENED)
	{
		if((tm_get_time() - socketopentimer) >= 15000)
		{
			if(if_4GFlag)
			{
				if_4GFlag = 0;
				ret = at_network_switch(2);    
				if (ret != 0)
				{
					shellprintf(" set wan apn failed,ret:0x%08x\r\n", ret);
					return;
				}
				switchtimes++;
    			shellprintf(" set net ok\r\n");
				socketopentimer = tm_get_time();
				log_o(LOG_SOCK_PROXY, "switch to 3G,switch times %d\n",switchtimes);
			}
			else//切换到4G
			{
				if_4GFlag = 1;
				ret = at_network_switch(3);    
				if (ret != 0)
				{
					shellprintf(" set wan apn failed,ret:0x%08x\r\n", ret);
					return;
				}
				switchtimes++;
				shellprintf(" set net ok\r\n");
				socketopentimer = tm_get_time();
				log_o(LOG_SOCK_PROXY, "switch to 4G,switch times %d\n",switchtimes);
			}
		}
	}
	else
	{
		socketopentimer = tm_get_time();
	}
}
#endif

/*
* 设置双向连接域名地址
*/
void setsockproxy_bdlAddrPort(char* addr,char* port)
{
	memset(sockSt.BDLLinkAddr, 0 , 33);
	memcpy(sockSt.BDLLinkAddr,addr,strlen((const char*)addr));
	sockSt.BDLPort = atoi((const char*)port);
	log_o(LOG_SOCK_PROXY, "sockSt.BDLLinkAddr: %s\n",sockSt.BDLLinkAddr);
	log_o(LOG_SOCK_PROXY, "port = %s\n",port);
	log_o(LOG_SOCK_PROXY, "sockSt.BDLPort = %d\n",sockSt.BDLPort);
	sockproxy_socketclose((int)(PP_SP_COLSE_SP + 10));//by liujian 20191015
}

/*
* 设置单向连接域名地址
*/
void setsockproxy_sgAddrPort(char* addr,char* port)
{
	memset(sockSt.sgLinkAddr, 0 , 33);
	memcpy(sockSt.sgLinkAddr,addr,strlen((const char*)addr));
	sockSt.sgPort = atoi((const char*)port);
	log_o(LOG_SOCK_PROXY, "sockSt.sgLinkAddr: %s\n",sockSt.sgLinkAddr);
	log_o(LOG_SOCK_PROXY, "port = %s\n",port);
	log_o(LOG_SOCK_PROXY, "sockSt.sgPort = %d\n",sockSt.sgPort);
	sockproxy_socketclose((int)(PP_SP_COLSE_SP + 11));//by liujian 20191015
}

/*
* 设置pki使能状态
*/
static int PP_shell_setPKIenable(int argc, const char **argv)
{  
	unsigned int pkien;

    if (argc != 1)
    {
        shellprintf(" usage:set pki en error\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &pkien);
	cfg_set_para(CFG_ITEM_EN_PKI, (unsigned char *)&pkien, 1);
	shellprintf(" set pki ok\r\n");
	//system("reboot");

    return 0;
}

/*
* 获取pki使能状态
*/
uint8_t getsockproxy_pkiEnStatus(void)
{
	return sockSt.pkiEnFlag;
}

/* 
获取链路状态
*/
static int sockproxy_get_link_status(void)
{
	log_i(LOG_SOCK_PROXY, "tsp link status: %s\n",sockSt.tsplinkstatus?"normal":"fault");
	return sockSt.tsplinkstatus;
}

/*检查底层socket链路状态*/
int sockproxy_check_link_status(void)
{
	int tcpst,res=0;

	if(!sockSt.pkiEnFlag)
	{
		return 0;
	}

	if(1 == sockproxy_socketState())
	{
		HzTcpLinkStateSg(SP_sockFd,&tcpst);
		if(1 != tcpst)
		{
			res = -1;
		}
	}

	return res;
}

/*检查底层socket链路状态*/
int sockproxy_nm_apncardtype(void)
{
	return sockSt.apnType;
}

/*
* 获取网络状态
*/
static int sockproxy_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
{
	if(!sockSt.apnType)//单apn
	{
		if (NM_PUBLIC_NET != type)
		{
			return 0;
		}
	}
	else
	{
		if (NM_PRIVATE_NET != type)
		{
			return 0;
		}
	}

    switch (nmmsg)
    {
        case NM_REG_MSG_CONNECTED:
		{
            sockSt.network = 1;
		}
        break;
        case NM_REG_MSG_DISCONNECTED:
		{
            sockSt.network = 0;
		}
        break;
        default:
            return -1;
    }

	return 0;
}