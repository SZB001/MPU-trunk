/******************************************************
文件名：	PrvtProt_xcall.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_XCALL_H
#define		_PRVTPROT_XCALL_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_XCALL_ACK_WAIT 		0x01//应答成功
#define PP_XCALL_ACK_SUCCESS 	0x02//应答成功


#define	PP_ECALL_TYPE 	2//ecall
#define PP_BCALL_TYPE	1//bcall
#define	PP_ICALL_TYPE	3//icall
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
	PP_BCALL = 0,//
    PP_ECALL,//
	PP_ICALL,
	PP_detection,
	PP_XCALL_MAX
} PP_Xcall_INDEX;

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* 请求:box to tsp */
	uint8_t resp;/* 响应:box to tsp */
	uint8_t retrans;/* retransmission */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_xcallSt_t; /*xcall结构体*/

/* application data struct */
/***********************************
			Xcall
***********************************/
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
/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_xcall_init(void);
extern int PP_xcall_mainfunction(void *task);
extern void PP_xcall_SetXcallReq(unsigned char req);
//extern void PP_rmtCtrl_SetEcallResp(unsigned char resp);

#endif 
