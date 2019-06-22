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
#define SOCK_SERVR_TIMEOUT    	(1000 * 5)
#define SOCK_TXPAKG_OUTOFTIME    (1000 * 5)

/***********宏函数***********/
#define sockproxy_getURL(x)			gb32960_getURL(x)
#define sockproxy_SkipSockCheck() 	(!gb32960_getNetworkSt() )//|| gb32960_getAllowSleepSt())
#define sockproxy_getsuspendSt() 	0//gb32960_getsuspendSt()

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
	PP_CLOSE_WAIT,//
	PP_OPEN_WAIT,//
    PP_OPENED,
} PP_SOCK_STATE;

typedef enum
{
	PP_RCV_UNRCV = 0,//接收空闲
	PP_RCV_GB,//接收国标数据
	PP_RCV_PRIV//接收私有协议数据
} PP_SOCK_RCV_TYPE;

#define PP_RCV_IDLE 0
typedef enum
{
	PP_GB_RCV_IDLE = 0,//接收空闲
	PP_GB_RCV_SIGN,//接收起始符
	PP_GB_RCV_CTRL,//接收命令控制字段：命令标志..数据单元加密方式等
	PP_GB_RCV_DATALEN,//接收数据单元长度
	PP_GB_RCV_DATA,//接收数据
	PP_GB_RCV_CHECKCODE//接收校验码
} PP_SOCK_GB_RCV_STEP;

typedef enum
{
	PP_PRIV_RCV_IDLE = 0,//接收空闲
	PP_PRIV_RCV_SIGN,//接收起始符
	PP_PRIV_RCV_CTRL,//接收命令控制字段：命令标志..数据单元加密方式等
	PP_PRIV_RCV_DATA,//接收数据
} PP_SOCK_PRIV_RCV_STEP;
/*****struct definitions*****/
#define SOCK_PROXY_RCVLEN	1456
typedef struct
{
    /* protocol status */
    int socket;
    char state;//
	//char sendbusy;//发送忙状态
	char asynCloseFlg;//异步关闭socket标志
	svr_addr_t sock_addr;
	/* rcv */
	char rcvType;//接收类型
	uint8_t rcvstep;//接收空闲
	int rcvlen;//接收数据帧总长度
	uint8_t rcvbuf[SOCK_PROXY_RCVLEN];//接收数据帧buf
	long datalen;
}__attribute__ ((packed)) sockproxy_stat_t;



/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/


#endif 
