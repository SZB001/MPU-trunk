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
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/prctl.h>
#include <netdb.h>
#include "timer.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "sock_api.h"
#include "gb32960_api.h"
#include "nm_api.h"
#include "../../support/protocol.h"
#include "hozon_PP_api.h"
#include "hozon_SP_api.h"
#include "sockproxy_rxdata.h"
#include "sockproxy_txdata.h"
#include "../PrvtProtocol/PrvtProt.h"
#include "tboxsock.h"
#include "sockproxy.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
static sockproxy_stat_t sockSt;
static pthread_mutex_t sendmtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
static pthread_mutex_t closemtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
#if !SOCKPROXY_SAFETY_EN
static pthread_mutex_t rcvmtx = 	PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
#endif
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
    //uint32_t reginf = 0, cfglen;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
		{
			sockSt.socket = 0;
			sockSt.state = PP_CLOSED;
			sockSt.asynCloseFlg = 0;
			sockSt.sock_addr.port = 0;
			sockSt.sock_addr.url[0] = 0;
			sockSt.rcvType = PP_RCV_UNRCV;
			sockSt.rcvstep = PP_RCV_IDLE;//���տ���
			sockSt.rcvlen = 0;//��������֡�ܳ���
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
			SP_data_init();
			sockSt.linkSt = 0;
		}
        break;
    }

    return ret;
}


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

    pthread_t sockttid;
    pthread_attr_t socktta;
    pthread_attr_init(&socktta);
    pthread_attr_setdetachstate(&socktta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&sockttid, &socktta, (void *)sockproxy_socketmain, NULL);

    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create socketmain failed, error: %s", strerror(errno));
        return ret;
    }

    pthread_t rcvtid;
    pthread_attr_t rcvta;
    pthread_attr_init(&rcvta);
    pthread_attr_setdetachstate(&rcvta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&rcvtid, &rcvta, (void *)sockproxy_rcvmain, NULL);

    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create rcvmain failed, error: %s", strerror(errno));
        return ret;
    }

    pthread_t sendtid;
    pthread_attr_t sendta;
    pthread_attr_init(&sendta);
    pthread_attr_setdetachstate(&sendta, PTHREAD_CREATE_DETACHED);

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

	log_set_level(LOG_SOCK_PROXY, LOG_DEBUG);

#if !SOCKPROXY_SAFETY_EN
	if ((sockSt.socket = sock_create("sockproxy", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_SOCK_PROXY, "create socket failed, thread exit");
        return NULL;
    }
#else

#endif

    while (1)
    {
        res = sockproxy_do_checksock(&sockSt);
    }
	(void)res;
#if !SOCKPROXY_SAFETY_EN
	sock_delete(sockSt.socket);
#else
	if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
	{
		(void)SgHzTboxClose();
	}
	else
	{
		(void)HzTboxClose();
	}
#endif
    return NULL;
}

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

    while (1)
    {
        res = sockproxy_do_receive(&sockSt);
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
void sockproxy_socketclose(void)
{
	if(pthread_mutex_trylock(&closemtx) == 0)
	{
		if(sockSt.state == PP_OPENED)
		{
			log_i(LOG_SOCK_PROXY, "close socket");
			sockSt.state = PP_CLOSE_WAIT;//ر�̬
			sockSt.asynCloseFlg = 1;
			sockSt.closewaittime = tm_get_time();;
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
#if !SOCKPROXY_SAFETY_EN
	static uint64_t time = 0;
	if((1 == sockSt.asynCloseFlg) && (pthread_mutex_lock(&rcvmtx) == 0))
	{
		if(pthread_mutex_trylock(&sendmtx) == 0)//(������������)��ȡ�������ɹ���˵����ǰ���Ϳ��У�ͬʱ��ס����
		{
			if(sockSt.state != PP_CLOSED)
			{
				log_i(LOG_SOCK_PROXY, "socket closed");
				sock_close(sockSt.socket);
				sockSt.state = PP_CLOSED;
				time = tm_get_time();
				pthread_mutex_unlock(&sendmtx);
			}
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
				if (sock_error(state->socket) || sock_sync(state->socket))
				{
					log_e(LOG_SOCK_PROXY, "socket error, reset protocol");
					sockproxy_socketclose();
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
					sockproxy_socketclose();
				}
				else
				{
					return 0;
				}
			}
		}
		break;
	}

    return -1;
#else

	int iRet = 0;
	char	OnePath[128]="\0";
	char	ScdPath[128]="\0";
	char 	destIP[128];
	struct 	hostent * he;
	char 	**phe = NULL;

	if(sockproxy_SkipSockCheck())
	{
		//log_i(LOG_HOZON, "network is not ok\n");
		return -1;
	}

	switch(sockSt.linkSt)
	{
		case SOCKPROXY_CHECK_CSR:
		{
			if(GetPP_CertDL_allowBDLink() == 0)//
			{//建立单向连接
				log_i(LOG_HOZON, "Cert inValid,set up sglink\n");
				sockSt.waittime = tm_get_time();
				sockSt.sglinkSt =  SOCKPROXY_SGLINK_INIT;
				sockSt.linkSt = SOCKPROXY_SETUP_SGLINK;
			}
			else
			{//建立双向连接
				log_i(LOG_HOZON, "Cert Valid,set up BDLlink\n");
				sockSt.waittime = tm_get_time();
				sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
				sockSt.linkSt = SOCKPROXY_SETUP_BDLLINK;
			}
		}
		break;
		case SOCKPROXY_SETUP_SGLINK:
		{
			switch(sockSt.sglinkSt)
			{
				case SOCKPROXY_SGLINK_INIT:
				{
					if((tm_get_time() - sockSt.waittime) >= 1000)
					{
						sockSt.sglinkSt = SOCKPROXY_SGLINK_CREAT;
						sockSt.waittime = tm_get_time();
					}
				}
				break;
				case SOCKPROXY_SGLINK_CREAT:
				{
					if(sockSt.state == PP_CLOSED)
					{
						he=gethostbyname("tboxgw-qa.chehezhi.cn");
						if(he == NULL)
						{
							log_e(LOG_SOCK_PROXY,"gethostbyname error\n");
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						for( phe=he->h_addr_list ; NULL != *phe ; ++phe)
						{
							 inet_ntop(he->h_addrtype,*phe,destIP,sizeof(destIP));
							 log_i(LOG_SOCK_PROXY,"%s\n",destIP);
							 break;
						}
						/*port ipaddr*/
						iRet = HzPortAddrCft(22000, 1,destIP,NULL);//TBOX端口地址配置初始化
						if(iRet != SOCKPROXY_SG_ADDR_INIT_SUCCESS)
						{
							log_e(LOG_SOCK_PROXY,"HzPortAddrCft error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						/*create random string*/
						sprintf(OnePath, "%s","/usrdata/pem/HozonCA.cer");
						sprintf(ScdPath, "%s","/usrdata/pem/TspCA.cer");

						iRet = SgHzTboxCertchainCfg(OnePath, ScdPath);
						if(iRet != SOCKPROXY_SG_CCIC_SUCCESS)
						{
							log_e(LOG_SOCK_PROXY,"SgHzTboxCertchainCfg +++++++++++++++iRet[%d] \n", iRet);
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						/*init SSL*/
						iRet = SgHzTboxInit("/usrdata/pem/userAuth.crl");
						if(iRet != SOCKPROXY_SG_INIT_SUCCESS)
						{
							log_e(LOG_SOCK_PROXY,"HzTboxInit error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						/*Initiate a connection server request*/
						iRet = SgHzTboxConnect();
						if(iRet != SOCKPROXY_SG_CONN_SUCCESS)
						{
							log_e(LOG_SOCK_PROXY,"SgHzTboxConnect error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						log_i(LOG_HOZON, "set up sglink success\n");
						sockSt.state = PP_OPENED;
					}
					else
					{
						if(sockSt.state == PP_OPENED)
						{
							return 0;
						}

						if((sockSt.asynCloseFlg == 1) && (0 == sockSt.rcvflag))
						{
							log_i(LOG_SOCK_PROXY, "sockSt.asynCloseFlg == 1 ,start to close sg socket\n");
							sockSt.sglinkSt = SOCKPROXY_SGLINK_CLOSE;
						}
					}
				}
				break;
				case SOCKPROXY_SGLINK_CLOSE:
				{
					/*release all resources and close all connections*/
					if(pthread_mutex_trylock(&sendmtx) == 0)//
					{
						sockSt.asynCloseFlg = 0;
						sockSt.waittime = tm_get_time();
						sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
						sockSt.linkSt = SOCKPROXY_CHECK_CSR;
						if(sockSt.state != PP_CLOSED)
						{
							log_i(LOG_SOCK_PROXY, "close sg socket\n");
							sockSt.state = PP_CLOSED;
							SgHzTboxClose();
						}
						pthread_mutex_unlock(&sendmtx);
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
		}
		break;
		case SOCKPROXY_SETUP_BDLLINK:
		{
			switch(sockSt.BDLlinkSt)
			{
				case SOCKPROXY_BDLLINK_INIT:
				{
					if((tm_get_time() - sockSt.waittime) >= 1000)
					{
						sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_CREAT;
						sockSt.waittime = tm_get_time();
					}
				}
				break;
				case SOCKPROXY_BDLLINK_CREAT:
				{
					char	UsCertPath[128]="\0";
					char	UsKeyPath[128]="\0";

					if(sockSt.state == PP_CLOSED)
					{
						he=gethostbyname("tboxgw-qa.chehezhi.cn");
						if(he == NULL)
						{
							log_e(LOG_SOCK_PROXY,"gethostbyname error\n");
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						for( phe=he->h_addr_list ; NULL != *phe ; ++phe)
						{
							 inet_ntop(he->h_addrtype,*phe,destIP,sizeof(destIP));
							 log_i(LOG_SOCK_PROXY,"%s\n",destIP);
							 break;
						}
						/*port ipaddr*/
						iRet = HzPortAddrCft(21000, 1,destIP,NULL);
						if(iRet != 1010)
						{
							log_e(LOG_SOCK_PROXY,"HzPortAddrCft error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						/*create random string*/
						sprintf(OnePath, "%s","/usrdata/pem/HozonCA.cer");
						sprintf(ScdPath, "%s","/usrdata/pem/TspCA.cer");
						sprintf(UsCertPath, "%s",PP_CERTDL_CERTPATH);//申请的证书，要跟two_certreqmain.key匹配使用
						sprintf(UsKeyPath, "%s",PP_CERTDL_TWOCERTKEYPATH);
						iRet = HzTboxCertchainCfg(OnePath, ScdPath, UsCertPath, UsKeyPath);
						if(iRet != 2030)
						{
							log_e(LOG_SOCK_PROXY,"HzTboxCertchainCfg error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						/*init SSL*/
						iRet = HzTboxInit("/usrdata/pem/tbox.crl");
						if(iRet != 1151)
						{
							log_e(LOG_SOCK_PROXY,"HzTboxInit error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
							sockSt.waittime = tm_get_time();
							return -1;
						}

						log_e(LOG_SOCK_PROXY,"\n", iRet);
						/*Initiate a connection server request*/
						iRet = HzTboxConnect();
						if(iRet != 1230)
						{
							log_e(LOG_SOCK_PROXY,"HzTboxConnect error+++++++++++++++iRet[%d] \n", iRet);
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
							sockSt.sglinkSt = SOCKPROXY_SGLINK_INIT;
							sockSt.waittime = tm_get_time();
							PP_CertDL_CertDLReset();
							sockSt.linkSt = SOCKPROXY_CHECK_CSR;
							return -1;
						}
						log_i(LOG_HOZON, "set up BDLlink success\n");
						sockSt.state = PP_OPENED;
					}
					else
					{
						if(sockSt.state == PP_OPENED)
						{
							return 0;
						}

						if((1 == sockSt.asynCloseFlg) && (0 == sockSt.rcvflag))
						{
							log_i(LOG_SOCK_PROXY, "sockSt.asynCloseFlg == 1 ,start to close socket\n");
							sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_CLOSE;
						}
					}
				}
				break;
				case SOCKPROXY_BDLLINK_CLOSE:
				{
					if(pthread_mutex_trylock(&sendmtx) == 0)//
					{
						if(sockSt.state != PP_CLOSED)
						{
							log_i(LOG_SOCK_PROXY, "socket closed\n");
							(void)HzTboxClose();
							sockSt.state = PP_CLOSED;
						}

						sockSt.asynCloseFlg = 0;
						sockSt.linkSt = SOCKPROXY_CHECK_CSR;
						sockSt.BDLlinkSt = SOCKPROXY_BDLLINK_INIT;
						pthread_mutex_unlock(&sendmtx);
					}
				}
				break;
				default:
				break;
			}
		}
		break;
		default:
		break;
	}
	return -1;
#endif
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
    int ret = 0, rlen;
    char rbuf[1456] = {0};

    if(state->state == PP_OPENED)
	{
    	sockSt.rcvflag = 1;
#if !SOCKPROXY_SAFETY_EN
    	if(pthread_mutex_lock(&rcvmtx) == 0)
    	{
			if ((rlen = sock_recv(state->socket, (uint8_t*)rbuf, sizeof(rbuf))) < 0)
			{
				log_e(LOG_SOCK_PROXY, "socket recv error: %s", strerror(errno));
				log_e(LOG_SOCK_PROXY, "socket recv error, reset protocol");
				sockproxy_socketclose();
				pthread_mutex_unlock(&rcvmtx);
				return -1;
			}
			pthread_mutex_unlock(&rcvmtx);
    	}
#else
		//log_i(LOG_SOCK_PROXY,"recv data<<<<<<<<<<<<<<<<<<<<<<\n");
		if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
		{
			ret = SgHzTboxDataRecv(rbuf, 1456, &rlen);
			if(ret != 1275)
			{
				log_e(LOG_SOCK_PROXY,"SgHzTboxDataRecv error+++++++++++++++iRet[%d] [%d]\n", ret, rlen);
				return -1;
			}
		}
		else
		{
			ret = HzTboxDataRecv(rbuf, 1456, &rlen);
			if(ret != 1275)
			{
				log_e(LOG_SOCK_PROXY,"HzTboxDataRecv error+++++++++++++++iRet[%d] [%d]\n", ret, rlen);
				return -1;
			}
		}

#endif

		protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_RCV", (uint8_t*)rbuf, rlen, 0);//��ӡ���յ�����
		//log_i(LOG_SOCK_PROXY,"recv data>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
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
				sockSt.rcvstep = PP_RCV_IDLE;//���տ���
				sockSt.rcvlen = 0;//��������֡�ܳ���
				if((0x23 == rbuf[0]) && (0x23 == rbuf[1]))//��������
				{
					sockSt.rcvType = PP_RCV_GB;
					sockproxy_gbMakeupMsg((uint8_t*)rbuf,rlen);

				}
				else if((0x2A == rbuf[0]) && (0x2A == rbuf[1]))//HOZON ��ҵ˽��Э������
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
		log_i(LOG_HOZON, "start to send report to server");
		if(rpt->Inform_cb_para != NULL)
		{
			PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)(rpt->Inform_cb_para);
			pakgtype = TxInform_ptr->pakgtype;
			switch(pakgtype)
			{
				case PP_TXPAKG_SIGTIME://����ʱЧ��
				{
					if((tm_get_time() - TxInform_ptr->eventtime) < SOCK_TXPAKG_OUTOFTIME)//����δ����
					{
						res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
						//protocol_dump(LOG_HOZON, "send data to tsp", rpt->msgdata, rpt->msglen, 1);
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
							sockproxy_socketclose();//by liujian 20190510
						}
						else if(res == 0)
						{
							log_e(LOG_HOZON, "send wait, send is canceled");
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
					//protocol_dump(LOG_HOZON, "send data to tsp", rpt->msgdata, rpt->msglen, 1);
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
						sockproxy_socketclose();//by liujian 20190510
					}
					else if(res == 0)
					{
						log_e(LOG_HOZON, "send wait, send is canceled");
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
				case PP_TXPAKG_CONTINUE:
				{
					res = sockproxy_MsgSend(rpt->msgdata, rpt->msglen, SP_data_ack_pack);
					//protocol_dump(LOG_HOZON, "send data to tsp", rpt->msgdata, rpt->msglen, 1);
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
						sockproxy_socketclose();//by liujian 20190510
					}
					else if(res == 0)
					{
						log_e(LOG_HOZON, "send wait, send is canceled");
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
			//protocol_dump(LOG_HOZON, "send data to tsp", rpt->msgdata, rpt->msglen, 1);
			if (res < 0)
			{
				log_e(LOG_HOZON, "socket send error, reset protocol");
				SP_data_put_send(rpt);
				if(rpt->SendInform_cb != NULL)
				{
					rpt->SendInform_cb(rpt->Inform_cb_para);
				}
				sockproxy_socketclose();//by liujian 20190510
			}
			else if(res == 0)
			{
				log_e(LOG_HOZON, "send wait, send is canceled");
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
	log_i(LOG_SOCK_PROXY, "<<<<<sockproxy_MsgSend <<<<<");
	if(pthread_mutex_trylock(&sendmtx) == 0)//(������������)��ȡ������
	{
		if((sockSt.state == PP_OPENED) || (sockSt.state == PP_CLOSE_WAIT))
		{
			protocol_dump(LOG_HOZON, "send data to tsp", msg, len, 1);
#if !SOCKPROXY_SAFETY_EN
			res = sock_send(sockSt.socket, msg, len, sync);
			if((res > 0) && (res != len))//ʵ�ʷ��ͳ�ȥ�����ݸ���Ҫ���͵����ݲ�һ��
			{
				res = 0;
			}
#else
			log_i(LOG_SOCK_PROXY, "<<<<<SP_data_ack_pack <<<<<");
			SP_data_ack_pack();
			log_i(LOG_SOCK_PROXY, ">>>>>SP_data_ack_pack >>>>>");
			if(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK)
			{
				log_i(LOG_SOCK_PROXY, "<<<<<SgHzTboxDataSend <<<<<");
				res = SgHzTboxDataSend((char*)msg,len);
				log_i(LOG_SOCK_PROXY, ">>>>>SgHzTboxDataSend >>>>>");
				if(res != 1260)
				{
					log_e(LOG_SOCK_PROXY,"SgHzTboxDataSend error+++++++++++++++iRet[%d] \n", res);
					//SP_data_ack_pack();
					return -1;
				}
			}
			else
			{
				log_i(LOG_SOCK_PROXY, "<<<<<HzTboxDataSend <<<<<");
				res = HzTboxDataSend((char*)msg,len);
				log_i(LOG_SOCK_PROXY, ">>>>>HzTboxDataSend >>>>>");
				if(res != 1260)
				{
					log_e(LOG_SOCK_PROXY,"HzTboxDataSend error+++++++++++++++iRet[%d] \n", res);
					//SP_data_ack_pack();
					return -1;
				}
			}

			//SP_data_ack_pack();
			//protocol_dump(LOG_HOZON, "send data to tsp", msg, len, 1);
#endif
		}
		else
		{
			log_e(LOG_SOCK_PROXY, "socket is not open");
		}
		
		pthread_mutex_unlock(&sendmtx);//����������
	}
	else
	{
		log_e(LOG_SOCK_PROXY, "send busy");
	}
	log_i(LOG_SOCK_PROXY, ">>>>>sockproxy_MsgSend >>>>>");
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
#if !SOCKPROXY_SAFETY_EN
	if((sockSt.state == PP_OPENED) && \
			(sock_status(sockSt.socket) == SOCK_STAT_OPENED))
	{
		return 1;
	}

#else
	if((sockSt.state == PP_OPENED) && \
			(sockSt.linkSt == SOCKPROXY_SETUP_BDLLINK))
	{
		return 1;
	}
#endif
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
	if(((sockSt.state == PP_OPENED) || (sockSt.state == PP_CLOSE_WAIT)) && \
			(sockSt.linkSt == SOCKPROXY_SETUP_SGLINK))
	{
		return 1;
	}

	return 0;
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
					else//���ݳ������
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
					if(sockSt.datalen == 18)//˵��û��message data
					{
						if(WrSockproxyData_Queue(SP_PRIV,sockSt.rcvbuf,sockSt.rcvlen) < 0)
						{
							log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
						}
						sockSt.rcvstep = PP_RCV_IDLE;
						sockSt.rcvType = PP_RCV_UNRCV;
						protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_PRIV_RCV",sockSt.rcvbuf, sockSt.rcvlen, 0);//��ӡ˽��Э����յ�����

					}
					else if(sockSt.datalen > SOCK_PROXY_RCVLEN)//���ݳ������
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
					if(WrSockproxyData_Queue(SP_PRIV,sockSt.rcvbuf,sockSt.rcvlen) < 0)
					{
						log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
					}
					sockSt.rcvstep = PP_RCV_IDLE;
					sockSt.rcvType = PP_RCV_UNRCV;
					protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_PRIV_RCV",sockSt.rcvbuf, sockSt.rcvlen, 0);//��ӡ˽��Э����յ�����
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
