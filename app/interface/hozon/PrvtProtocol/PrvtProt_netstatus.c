/******************************************************
�ļ�����	PrvtProt_netstatus.c

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
#include <stdlib.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <pthread.h>
#include <sys/prctl.h>
#include "init.h"
#include "timer.h"
#include "log.h"
#include "dev_api.h"
#include "PrvtProt_netstatus.h"

/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/


/*Static function declaration*/
static PP_net_status_t PP_net_st;
static pthread_mutex_t dtmtx = 	PTHREAD_MUTEX_INITIALIZER;

/*******************************************************
description�� global variable definitions
*******************************************************/


/*******************************************************
description�� static variable definitions
*******************************************************/
static int PP_netstatus_pubilc_status(char* dstaddr,int cnt);
static void *TskPP_netstatus_main(void);

/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPP_netstatus_Parameter

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void InitPP_netstatus_Parameter(void)
{
    memset(&PP_net_st,0,sizeof(PP_net_status_t));
}

void PP_netstatus_run(void)
{
    int ret;
    pthread_t netsttid;
    pthread_attr_t netstta;

    pthread_attr_init(&netstta);
    pthread_attr_setdetachstate(&netstta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&netsttid, &netstta, (void *)TskPP_netstatus_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "check net status pthread create failed, error: %s", strerror(errno));
    }
}

/******************************************************
*TskPP_VehiInfo_MainFunction

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
static void *TskPP_netstatus_main(void)
{
    log_o(LOG_HOZON, "check net status thread running");
    prctl(PR_SET_NAME, "NET_STATUS");
    while(1)
    {
        sleep(60);
        if(dev_get_KL15_signal())
        {
            PP_net_st.newSt = PP_netstatus_pubilc_status("www.baidu.com",PP_NETST_CNT);
            if(PP_net_st.newSt != PP_net_st.oldSt)
            {
                PP_net_st.oldSt = PP_net_st.newSt;
                if(0 == PP_net_st.newSt)
                {
                    pthread_mutex_lock(&dtmtx);
                    PP_net_st.faultflag = 0;
                    pthread_mutex_unlock(&dtmtx);
                }
                else
                {
                    pthread_mutex_lock(&dtmtx);
                    PP_net_st.faultflag = 1;
                    PP_net_st.timestamp = tm_get_time();
                    pthread_mutex_unlock(&dtmtx);
                }
            }
        }
    }

    return NULL;
}

/*
	检查公网网络状态
*/
static int PP_netstatus_pubilc_status(char* dstaddr,int cnt)
{
    FILE *stream;
    char recvBuf[16] = {0};
    char cmdBuf[256] = {0};

    if((NULL == dstaddr) || (cnt <= 0))
	{
		return -1;
	}

    sprintf(cmdBuf, "ping %s -c %d -i 0.2 | grep time= | wc -l",dstaddr,cnt);
    stream = popen(cmdBuf,"r");
    fread(recvBuf, sizeof(char), sizeof(recvBuf)-1, stream);
    pclose(stream);

    if(atoi(recvBuf) > 0)
	{
		return 0;
	}

    return -1;
}


/*
    网络故障状态
*/
uint8_t PP_netstatus_pubilcfaultsts(uint64_t *timestamp)
{
    uint8_t faultst;

    pthread_mutex_lock(&dtmtx);
    if(timestamp != NULL)
    {
        *timestamp = PP_net_st.timestamp;
    }
    faultst = PP_net_st.faultflag;
    pthread_mutex_unlock(&dtmtx);

    return faultst;
}