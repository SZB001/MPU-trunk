/******************************************************
文件名：	PrvtProt.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_H
#define		_PRVTPROT_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_ACK_WAIT 	0x01//等待成功
#define PP_ACK_SUCCESS 	0x02//应答成功

#define PP_MSG_DATA_LEN 	1024//message data 长度
#define PP_TBOXSN_LEN 		19//tboxsn 长度

#define	PP_NATIONALSTANDARD_TYPE	0//操作类型：国标类型
#define	PP_HEARTBEAT_TYPE			1//心跳类型
#define	PP_NGTP_TYPE				2//NGTP类型

/* xcall */
//AID
#define PP_AID_XCALL 	170//Xcall
//MID
#define PP_MID_XCALL_REQ 	1//Xcall request
#define PP_MID_XCALL_RESP 	2//Xcall response

/* remote config */
//AID
#define PP_AID_RMTCFG	100//remote config
//MID
#define PP_MID_CHECK_CFG_REQ 	1//check config req
#define PP_MID_CHECK_CFG_RESP 	2//check config response
#define PP_MID_GET_CFG_REQ 		3//get config req
#define PP_MID_GET_CFG_RESP 	4//get config response
#define PP_MID_READ_CFG_REQ 	5//read config req
#define PP_MID_READ_CFG_RESP 	6//read config response
#define PP_MID_CONN_CFG_REQ 	7//conn config req
#define PP_MID_CONN_CFG_RESP 	8//conn config response
#define PP_MID_CFG_END 	9//end config req

/* remote ctrl */
//AID
#define PP_AID_RMTCTRL	 	110//
//MID
#define PP_MID_RMTCTRL_REQ 			1//remote ctrl request
#define PP_MID_RMTCTRL_RESP 		2//remote ctrl response
#define PP_MID_RMTCTRL_BOOKINGRESP 	3//remote ctrl booking response
#define PP_MID_RMTCTRL_HUBOOKINGRESP 	4//remote ctrl HU booking response

#define PP_AID_VS	 		130//车辆状态
//MID
#define PP_MID_VS_REQ 	1//VS request
#define PP_MID_VS_RESP 	2//VS response

#define PP_AID_DIAG	 		140//远程诊断
//MID
#define PP_MID_DIAG_REQ 	1//request
#define PP_MID_DIAG_RESP 	2//response
#define PP_MID_DIAG_STATUS 			3
#define PP_MID_DIAG_IMAGEACQREQ  	4
#define PP_MID_DIAG_LOGACQRESP 		5
//#define PP_MID_DIAG_IMAGEACQRESP  	6
//#define PP_MID_DIAG_LOGACQRES		7

#define PP_TXPAKG_FAIL 	(-1)//报文发送失败
#define PP_TXPAKG_SUCCESS 	  1//报文发送成功

/***********宏函数***********/
typedef void (*PrvtProt_InitObj)(void);//初始化
typedef int (*PrvtProt_mainFuncObj)(void* x);//

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_RMTFUNC_XCALL = 0,//XCALL
	PP_RMTFUNC_CC,//呼叫中心
	PP_RMTFUNC_CFG,//远程配置
	PP_RMTFUNC_CTRL,//远程控制
	PP_RMTFUNC_VS,//车辆状态
	PP_RMTFUNC_DIAG,//远程诊断
	PP_RMTFUNC_MAX
} PP_RMTFUNC_INDEX;

typedef enum
{
	PP_TXPAKG_UNKOWNTYPE = 0,//未知类型
	PP_TXPAKG_CONTINUE,//周期型
	PP_TXPAKG_SIGTRIG,//单次触发型
	PP_TXPAKG_SIGTIME//单次时效型
} PP_TXPAKG_TYPE;

typedef enum
{
	PP_TXPAKG_UNKOWNFAIL = 0,//未知类型
	PP_TXPAKG_TXFAIL,
	PP_TXPAKG_OUTOFDATE//报文过期
} PP_TXPAKG_FAILTYPE;

typedef enum
{
	PP_IDLE = 0,//
    PP_HEARTBEAT,//等待心跳响应状态
} PP_WAIT_STATE;

/*****struct definitions*****/
typedef struct 
{		
	unsigned char sign[2U];/* 起始符标志位，取值0x2A，0x2A*/
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char mnr : 4;/* 小版本(由TSP平台定义)*/
			unsigned char mjr : 4;/* 大版本(由TSP平台定义)*/
		}bits; /**/
	}ver;
	unsigned long int nonce;/* TCP会话ID 由TSP平台产生 */
	
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char encode : 4;/* 编码方式0：none；1NGTP；2：GZIP；3：JSON*/	
			unsigned char mode : 1;/* 类型0：normal；1：debug*/
			unsigned char connt  : 1;/* 连接方式0：短连接；1：长连接*/
			unsigned char asyn : 1;/* 通讯方式0：同步；1：异步*/
			unsigned char dir  : 1;/* 报文方向0：To Tbox；1：To TSP*/
		}bits; /**/
	}commtype;
	
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char encrypt   : 4;/* 加密方式0：none；1：AES128；2：AES256；3RSA2048*/
			unsigned char signature : 4;/* 签名方式:0 -- none ;1 -- SHA1;2 -- SHA256*/	
		}bits; /**/
	}safetype;
	
	unsigned char opera;/* 操作类型:0 -- national standard ;1 -- heartbeat;2 -- ngtp ;3 -- OTA */
	unsigned long int msglen;/* 报文长度 */
	unsigned long int tboxid;/* 平台通过tboxID与tboxSN映射 */
}__attribute__((packed)) PrvtProt_pack_Header_t; /*报文头结构体*/ 

typedef struct 
{		
	PrvtProt_pack_Header_t Header;/* */
	unsigned char msgdata[PP_MSG_DATA_LEN];/* 消息体 */
	unsigned char msgtype;/* 消息类型 */
	int totallen;//数据总长度
}__attribute__((packed)) PrvtProt_pack_t; /*报文结构体*/

typedef struct 
{		
	uint64_t  timer;/* 心跳计时器 */
	//uint8_t ackFlag;/* 应答标志:2-等待应答；1-成功应答 */
	uint8_t state;/* 心跳状态 1- 正常 */
	uint8_t period;/* 心跳周期uints：秒*/
	PP_WAIT_STATE waitSt;/* 等待响应的状态 */
	uint64_t waittime;/* 等待响应的时间 */
}__attribute__((packed))  PrvtProt_heartbeat_t; /*心跳结构体*/

typedef struct 
{	
	char suspend;/* 暂停 */
	uint32_t nonce;/* TCP会话ID 由TSP平台产生 */
	unsigned char version;/* 大/小版本(由TSP平台定义)*/
	uint32_t tboxid;/* 平台通过tboxID与tboxSN映射 */
}__attribute__((packed))  PrvtProt_task_t; /* 任务参数结构体*/

/* Dispatcher Body struct */
typedef struct
{
	uint8_t	 	aID[4];
	uint8_t	 	mID;
	long	eventTime;
	long	expTime	/* OPTIONAL */;
	long	eventId	/* OPTIONAL */;
	long	ulMsgCnt	/* OPTIONAL */;
	long	dlMsgCnt	/* OPTIONAL */;
	long	msgCntAcked	/* OPTIONAL */;
	int		ackReq	/* OPTIONAL */;
	long	appDataLen	/* OPTIONAL */;
	long	appDataEncode	/* OPTIONAL */;
	long	appDataProVer	/* OPTIONAL */;
	long	testFlag	/* OPTIONAL */;
	long	result	/* OPTIONAL */;
}PrvtProt_DisptrBody_t;


typedef struct
{
	char funcObj;
	PrvtProt_InitObj Init;//初始化
	PrvtProt_mainFuncObj mainFunc;//
}PrvtProt_RmtFunc_t; /*结构体*/

typedef struct
{
	uint8_t idleflag;
	int aid;
	int mid;
	char pakgtype;//报文类型:1-持续型（持续发送直到发送成功）；2-单次触发型（发送失败丢弃）；3-单次时效性（发送失败丢弃，同时报文具有时效性）
	uint64_t eventtime;//事件产生时的时间
	char successflg;//报文发送完成标志：-1 - 失败；1 - 成功；默认0
	uint8_t failresion;
	uint64_t txfailtime;//发送失败的时间
}PrvtProt_TxInform_t; /*结构体*/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/
//extern PrvtProt_appData_t 		PP_appData;//共用的结构体（app data数据打包时使用）

/*******************************************************
description： function External declaration
*******************************************************/
extern long PrvtPro_BSEndianReverse(long value);
extern long PrvtPro_getTimestamp(void);
extern void PrvtProt_gettboxsn(char *tboxsn);
#endif 
