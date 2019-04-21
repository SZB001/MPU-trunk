/******************************************************
文件名：	sockproxy.h
描述：	合众tsp对接socket链路的建立、断开、收/发数据处理
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__SOCK_PROXY_H
#define		__SOCK_PROXY_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/
#define SOCKPROXY_SHELL_PROTOCOL  0//该宏说明是否添加外壳协议:1-是 ; 0-否

/**********宏常量定义*********/
#define SOCK_SERVR_TIMEOUT    (1000 * 5)


/***********宏函数***********/
#define sockproxy_getURL(x)			gb32960_getURL(x)
#define sockproxy_SkipSockCheck() 	(!gb32960_getNetworkSt() || gb32960_getAllowSleepSt())
#define sockproxy_getsuspendSt() 	gb32960_getsuspendSt()

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_CLOSED = 0,//
    PP_OPEN,
} PP_SOCK_STATE;
/*****struct definitions*****/
typedef struct
{
    /* protocol status */
    int socket;
    char state;//
	char sendbusy;//发送忙状态
	char asynCloseFlg;//异步关闭socket标志
	svr_addr_t sock_addr;
}__attribute__ ((packed)) sockproxy_stat_t;


/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/


#endif 
