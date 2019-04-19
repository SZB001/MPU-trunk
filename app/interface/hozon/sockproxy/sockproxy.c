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
//#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "sock_api.h"
#include "sockproxy.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void *sockproxy_main(void);

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
    uint32_t reginf = 0, cfglen;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:

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
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)sockproxy_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_SOCK_PROXY, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
}


/******************************************************
*��������sockproxy_main
*��  �Σ�void
*����ֵ��void
*��  ������������
*��  ע��
******************************************************/
static void *sockproxy_main(void)
{
	sockproxy_stat_t state;
	
	log_o(LOG_SOCK_PROXY, "socket proxy  of hozon thread running");
    prctl(PR_SET_NAME, "SOCK_PROXY");
	memset(&state, 0, sizeof(hz_stat_t));
	if ((state.socket = sock_create("sockproxy", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_SOCK_PROXY, "create socket failed, thread exit");
        return NULL;
    }
	
    while (1)
    {
        res = gb_do_checksock(&state) ||	//���socket����
              gb_do_receive(&state);		//socket���ݽ���

    }

    return NULL;
}


