/******************************************************
文件名：	sockproxy.c
描述：	合众tsp对接socket链路的建立、断开、收/发数据处理	
Data			Vasion			author
2019/4/17		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/prctl.h>
#include "timer.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "sock_api.h"
#include "gb32960_api.h"
#include "nm_api.h"
#include "../../support/protocol.h"
#include "hozon_PP_api.h"
#include "sockproxy_data.h"
#include "sockproxy.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static sockproxy_stat_t sockSt;
static pthread_mutex_t sendmtx = PTHREAD_MUTEX_INITIALIZER;//初始化静态锁
static pthread_mutex_t closemtx = PTHREAD_MUTEX_INITIALIZER;//初始化静态锁
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void *sockproxy_main(void);
static int sockproxy_do_checksock(sockproxy_stat_t *state);
static int sockproxy_do_receive(sockproxy_stat_t *state);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：sockproxy_init
*形  参：void
*返回值：void
*描  述：初始化
*备  注：
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
			sockSt.sendbusy = 0;
			sockSt.asynCloseFlg = 0;
			sockSt.sock_addr.port = 0;
			sockSt.sock_addr.url[0] = 0;
			SockproxyData_Init();
		}
        break;
        case INIT_PHASE_RESTORE:
		{
			
		}
        break;
        case INIT_PHASE_OUTSIDE:
		{
			
		}
        break;
    }

    return ret;
}


/******************************************************
*函数名：sockproxy_run
*形  参：void
*返回值：void
*描  述：创建任务线程
*备  注：
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
*函数名：sockproxy_main
*形  参：void
*返回值：void
*描  述：主任务函数
*备  注：
******************************************************/
static void *sockproxy_main(void)
{
	int res = 0;
	log_o(LOG_SOCK_PROXY, "socket proxy  of hozon thread running");
    prctl(PR_SET_NAME, "SOCK_PROXY");

	if ((sockSt.socket = sock_create("sockproxy", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_SOCK_PROXY, "create socket failed, thread exit");
        return NULL;
    }
	
    while (1)
    {
        res = sockproxy_do_checksock(&sockSt) ||	//检查socket连接,正常返回0
             sockproxy_do_receive(&sockSt);		//socket数据接收
    }
	
	sock_delete(sockSt.socket);
    return NULL;
}

/******************************************************
*函数名：sockproxy_socketState
*形  参：
*返回值：
*描  述：socket open/colse state
*备  注：同步操作
******************************************************/
void sockproxy_socketclose(void)
{
	if(pthread_mutex_trylock(&closemtx) == 0)//(非阻塞互斥锁)获取互斥锁成功
	{//(非阻塞互斥锁)若获取互斥锁失败，说明此时有其他线程在执行关闭
		if(sockSt.state == PP_OPEN)
		{
			sockSt.state = PP_CLOSE_WAIT;//等待关闭状态
			sockSt.asynCloseFlg = 1;
		}
		pthread_mutex_unlock(&closemtx);
	}
}

/******************************************************
*函数名：sockproxy_do_checksock
*形  参：void
*返回值：void
*描  述：检查socket连接
*备  注：
******************************************************/
static int sockproxy_do_checksock(sockproxy_stat_t *state)
{
	if(1 == sockSt.asynCloseFlg) 
	{
		if(pthread_mutex_trylock(&sendmtx) == 0)//(非阻塞互斥锁)获取互斥锁成功，说明当前发送空闲，同时锁住发送
		{
			if(sockSt.state != PP_CLOSED)
			{
				sock_close(sockSt.socket);
				sockSt.state = PP_CLOSED;
				pthread_mutex_unlock(&sendmtx);
			}
			sockSt.asynCloseFlg = 0;
		}
		return -1;	
	}
	
	sockproxy_getURL(&state->sock_addr);
    if(sockproxy_SkipSockCheck() || !state->sock_addr.port || !state->sock_addr.url[0])
    {
        return -1;
    }

    if (sock_status(state->socket) == SOCK_STAT_CLOSED)
    {
        static uint64_t time = 0;

        if(sockproxy_getsuspendSt() &&	\
				(time == 0 || tm_get_time() - time > SOCK_SERVR_TIMEOUT))
        {
            log_i(LOG_SOCK_PROXY, "start to connect with server");

            if (sock_open(NM_PUBLIC_NET,state->socket, state->sock_addr.url, state->sock_addr.port) != 0)
            {
                log_i(LOG_SOCK_PROXY, "open socket failed, retry later");
            }

            time = tm_get_time();
        }
    }
    else if (sock_status(state->socket) == SOCK_STAT_OPENED)
    {
		state->state = PP_OPEN;
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
	else
	{}

    return -1;
}

/******************************************************
*函数名：sockproxy_do_receive
*形  参：void
*返回值：void
*描  述：接收数据
*备  注：
******************************************************/
static int sockproxy_do_receive(sockproxy_stat_t *state)
{
    int ret = 0, rlen;
    uint8_t rcvbuf[1456] = {0};

    if ((rlen = sock_recv(state->socket, rcvbuf, sizeof(rcvbuf))) < 0)
    {
        log_e(LOG_SOCK_PROXY, "socket recv error: %s", strerror(errno));
        log_e(LOG_SOCK_PROXY, "socket recv error, reset protocol");
        sockproxy_socketclose();
        return -1;
    }
	
	protocol_dump(LOG_SOCK_PROXY, "sockproxy", rcvbuf, rlen, 0);//打印接收的数据
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
	if((0x2A == rcvbuf[0]) && (0x2A == rcvbuf[1]))//国标数据
	{
		if(WrSockproxyData_Queue(SP_GB,rcvbuf,rlen) < 0)
		{
			 log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_GB,rcvbuf,rlen) error");
		}
	}
	else if((0x23 == rcvbuf[0]) && (0x23 == rcvbuf[1]))//HOZON 企业私有协议数据
	{
		if(WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) < 0)
		{
			log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
		}
	}
	else
	{
		log_i(LOG_SOCK_PROXY, "sockproxy_do_receive unknow package");
	}
#endif

    return ret;
}

/******************************************************
*函数名：sockproxy_MsgSend
*形  参：
*返回值：
*描  述：数据发送
*备  注：
******************************************************/
int sockproxy_MsgSend(uint8_t* msg,int len,void (*sync)(void))
{
	int res;
	
	if(pthread_mutex_trylock(&sendmtx) != 0) return 0;//(非阻塞互斥锁)获取互斥锁失败
	if((sockSt.sendbusy == 1U) || (sockSt.state != PP_OPEN))	
	{
		pthread_mutex_unlock(&sendmtx);//解锁互斥锁
		return 0;	
	}
	sockSt.sendbusy = 1U;
	res = sock_send(sockSt.socket, msg, len, sync);

	if(res != len)//实际需要发送出去的数据跟需要发送的数据不一致
	{
		res = 0;
	}
	sockSt.sendbusy = 0U;
	pthread_mutex_unlock(&sendmtx);//解锁互斥锁
	return res;
}

/******************************************************
*函数名：sockproxy_socketState
*形  参：
*返回值：
*描  述：socket open/colse state
*备  注：
******************************************************/
int sockproxy_socketState(void)
{
	return sockSt.state;
}
