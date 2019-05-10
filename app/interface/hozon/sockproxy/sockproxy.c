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
static void sockproxy_gbMakeupMsg(uint8_t *data,int len);
static void sockproxy_privMakeupMsg(uint8_t *data,int len);

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
			sockSt.asynCloseFlg = 0;
			sockSt.sock_addr.port = 0;
			sockSt.sock_addr.url[0] = 0;
			sockSt.rcvType = PP_RCV_UNRCV;
			sockSt.rcvstep = PP_RCV_IDLE;//接收空闲
			sockSt.rcvlen = 0;//接收数据帧总长度
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
	(void)res;
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
		if(sockSt.state == PP_OPENED)
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
	static uint64_t time = 0;
	if(1 == sockSt.asynCloseFlg) 
	{
		if(pthread_mutex_trylock(&sendmtx) == 0)//(非阻塞互斥锁)获取互斥锁成功，说明当前发送空闲，同时锁住发送
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
		return -1;	
	}
	
	sockproxy_getURL(&state->sock_addr);
    if(sockproxy_SkipSockCheck() || !state->sock_addr.port || !state->sock_addr.url[0])
    {
    	log_e(LOG_SOCK_PROXY, "state.network = %d",sockproxy_SkipSockCheck());
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
					else
					{
						state->state = PP_OPEN_WAIT;
					}

					time = tm_get_time();
				}
			}
			log_e(LOG_SOCK_PROXY, "socket status : %d\r\n",sock_status(state->socket));
		}
		break;
		case PP_OPEN_WAIT:
		{
			if((tm_get_time() - time) < 2000)
			{
				if(sock_status(state->socket) == SOCK_STAT_OPENED)
				{
					log_i(LOG_SOCK_PROXY, "socket is open success");
					state->state = PP_OPENED;
				}
			}
			else
			{
				log_e(LOG_SOCK_PROXY, "PP_OPEN_WAIT timeout,socket close");
				sock_close(sockSt.socket);
				sockSt.state = PP_CLOSED;
			}
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
    uint8_t rbuf[1456] = {0};

    if ((rlen = sock_recv(state->socket, rbuf, sizeof(rbuf))) < 0)
    {
        log_e(LOG_SOCK_PROXY, "socket recv error: %s", strerror(errno));
        log_e(LOG_SOCK_PROXY, "socket recv error, reset protocol");
        sockproxy_socketclose();
        return -1;
    }
	
	protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_RCV", rbuf, rlen, 0);//打印接收的数据
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
    		sockSt.rcvstep = PP_RCV_IDLE;//接收空闲
    		sockSt.rcvlen = 0;//接收数据帧总长度
    		if((0x23 == rbuf[0]) && (0x23 == rbuf[1]))//国标数据
			{
    			sockSt.rcvType = PP_RCV_GB;
				sockproxy_gbMakeupMsg(rbuf,rlen);

			}
			else if((0x2A == rbuf[0]) && (0x2A == rbuf[1]))//HOZON 企业私有协议数据
			{
				sockSt.rcvType = PP_RCV_PRIV;
				sockproxy_privMakeupMsg(rbuf,rlen);
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
			sockproxy_gbMakeupMsg(rbuf,rlen);
		}
		break;
    	case PP_RCV_PRIV:
		{
			sockproxy_privMakeupMsg(rbuf,rlen);
		}
		break;
    	default:
    	break;
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
	int res = 0;
	log_i(LOG_SOCK_PROXY, "sockproxy_MsgSend <<<<<");
	if(pthread_mutex_trylock(&sendmtx) == 0)//(非阻塞互斥锁)获取互斥锁
	{
		if(sockSt.state == PP_OPENED)
		{
			res = sock_send(sockSt.socket, msg, len, sync);
			if((res > 0) && (res != len))//实际发送出去的数据跟需要发送的数据不一致
			{
				res = 0;
			}	
		}
		else
		{
			log_e(LOG_SOCK_PROXY, "socket is not open");
		}
		
		pthread_mutex_unlock(&sendmtx);//解锁互斥锁
	}
	else
	{
		log_e(LOG_SOCK_PROXY, "send busy");
	}
	log_i(LOG_SOCK_PROXY, "sockproxy_MsgSend >>>>>");
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
	if((sockSt.state == PP_OPENED) && \
			(sock_status(sockSt.socket) == SOCK_STAT_OPENED))
	{
		return 1;
	}
	return 0;
}

/******************************************************
*函数名：sockproxy_gbMakeupMsg
*形  参：
*返回值：
*描  述：gb协议数据组包
*备  注：
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
					else//数据长度溢出
					{
						sockSt.rcvstep = PP_RCV_IDLE;
						sockSt.rcvlen = 0;
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
				protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_GB_RCV", sockSt.rcvbuf, sockSt.rcvlen, 0);//打印gb接收的数据
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
*函数名：sockproxy_privMakeupMsg
*形  参：
*返回值：
*描  述：企业私有协议数据组包
*备  注：
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
					if(sockSt.datalen == 18)//说明没有message data
					{
						if(WrSockproxyData_Queue(SP_PRIV,sockSt.rcvbuf,sockSt.rcvlen) < 0)
						{
							log_e(LOG_SOCK_PROXY, "WrSockproxyData_Queue(SP_PRIV,rcvbuf,rlen) error");
						}
						sockSt.rcvstep = PP_RCV_IDLE;
						sockSt.rcvType = PP_RCV_UNRCV;
						protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_PRIV_RCV",sockSt.rcvbuf, sockSt.rcvlen, 0);//打印私有协议接收的数据

					}
					else if(sockSt.datalen > SOCK_PROXY_RCVLEN)//数据长度溢出
					{
						sockSt.rcvstep = PP_RCV_IDLE;
						sockSt.rcvlen = 0;
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
					protocol_dump(LOG_SOCK_PROXY, "SOCK_PROXY_PRIV_RCV",sockSt.rcvbuf, sockSt.rcvlen, 0);//打印私有协议接收的数据
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
