/******************************************************
文件名：	
描述：	合众tsp对接socket链路的建立、断开、收/发数据处理
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__SOCK_PROXY_DATA_H
#define		__SOCK_PROXY_DATA_H
/*******************************************************
description： include the header file
*******************************************************/


/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/


/**********宏常量定义*********/
#define SP_DATA_LNG  1456U/*数据队列中数据长*/
#define SP_QUEUE_LNG  3U/*数据队列长*/

/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	SP_GB = 0,//
    SP_PRIV,
	SP_MAX,
} SP_RX_OBJ;

/*****struct definitions*****/
typedef struct
{
	unsigned char  NonEmptyFlg;	/*数据非空标志*/
	int	  len;/*数据长*/
	unsigned char  data[SP_DATA_LNG];/*数据*/
}sockProxyCache_t;/*数据队列结构体*/

typedef struct
{
	unsigned char  HeadLabel;/*头标签*/
	unsigned char  TialLabel;/*尾标签*/
	sockProxyCache_t SPCache[SP_QUEUE_LNG];
}sockProxyObj_t;/*接收对象结构体*/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void SockproxyData_Init(void);
extern int WrSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len);
extern int RdSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len);

#endif
