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

#define	PP_NATIONALSTANDARD_TYPE	0//操作类型：国标类型
#define	PP_HEARTBEAT_TYPE			1//心跳类型
#define	PP_NGTP_TYPE				2//NGTP类型

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
	PP_APP_HB = 0,//心跳
	PP_APP_XCALL,//
	PP_APP_MAX
} PP_APP_INDEX;

typedef enum
{
	PP_APP_HEARTBEAT = 0,//心跳
	PP_XCALL_REQ,//xcall request
    PP_XCALL_RESP,//xcall response
	PP_APP_MID_MAX
} PP_APP_MID_TYPE;//应用类型

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
}__attribute__((packed)) PrvtProt_pack_t; /*报文结构体*/

typedef struct 
{		
	uint64_t  timer;/* 心跳计时器 */
	//uint8_t ackFlag;/* 应答标志:2-等待应答；1-成功应答 */
	uint8_t state;/* 心跳状态 1- 正常 */
	uint8_t period;/* 心跳周期uints：秒*/
}__attribute__((packed))  PrvtProt_heartbeat_t; /*心跳结构体*/

typedef struct 
{	
	PrvtProt_heartbeat_t heartbeat;
	//PrvtProt_xcall_t xcall[PP_XCALL_MAX];
	PP_WAIT_STATE waitSt[PP_APP_MAX];/* 等待响应的状态 */
	uint64_t waittime[PP_APP_MAX];/* 等待响应的时间 */
	char suspend;/* 暂停 */
	uint32_t nonce;/* TCP会话ID 由TSP平台产生 */
	unsigned char version;/* 大/小版本(由TSP平台定义)*/
	uint32_t tboxid;/* 平台通过tboxID与tboxSN映射 */
}__attribute__((packed))  PrvtProt_task_t; /* 任务参数结构体*/

/* Dispatcher Body struct */
typedef struct
{
	uint8_t	 	aID[3];
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

/* application data struct */
/**************************** Xcall ******************************/
typedef struct
{
	int  gpsSt;//gps状态 0-无效；1-有效
	long gpsTimestamp;//gps时间戳
	long latitude;//纬度 x 1000000,当GPS信号无效时，值为0
	long longitude;//经度 x 1000000,当GPS信号无效时，值为0
	long altitude;//高度（m）
	long heading;//车头方向角度，0为正北方向
	long gpsSpeed;//速度 x 10，单位km/h
	long hdop;//水平精度因子 x 10
}PrvtProt_Rvsposition_t;

typedef struct
{
	long xcallType;//类型  1-道路救援   2-紧急救援（ecall）  3-400电话进线
	long engineSt;//启动状态；1-熄火；2-启动
	long totalOdoMr;//里程有效范围：0 - 1000000（km）
	PrvtProt_Rvsposition_t gpsPos;//车辆救援位置
	long srsSt;//安全气囊状态 1- 正常；2 - 弹出
	long updataTime;//数据时间戳
	long battSOCEx;//车辆电池剩余电量：0-10000（0%-100%）
}PrvtProt_App_Xcall_t;
/******************************************************************/

typedef struct
{
	PrvtProt_App_Xcall_t Xcall;//xcall
}PrvtProt_appData_t;

/* message data struct */
typedef struct
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t 		appData;
}PrvtProt_msgData_t;
/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/
//extern PrvtProt_task_t 	pp_task;
extern PrvtProt_pack_Header_t 	PP_PackHeader[PP_APP_MID_MAX];
extern PrvtProt_pack_t 			PP_Pack[PP_APP_MID_MAX];
extern PrvtProt_DisptrBody_t	PP_DisptrBody[PP_APP_MID_MAX];
extern PrvtProt_appData_t 		PP_appData;
/*******************************************************
description： function External declaration
*******************************************************/
extern long PrvtPro_BSEndianReverse(long value);
extern long PrvtPro_getTimestamp(void);
#endif 
