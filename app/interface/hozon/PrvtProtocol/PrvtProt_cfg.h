/******************************************************
文件名：	PrvtProt_cfg.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CFG_H
#define		_PRVTPROT_CFG_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/
#define PP_THREAD   1//定义是否单独创建线程 1-是 0-不是
#define PP_SOCKPROXY   1//定义是否使用socket代理(是否由其他模块创建socket链路) 1-是 0-不是
/**********宏常量定义*********/
#define PP_HEART_BEAT_TIME (10)//心跳周期

#define PP_HB_WAIT_TIMEOUT 	(5*1000)//心跳等待超时时间
#define PP_XCALL_WAIT_TIMEOUT 	(5*1000)//等待超时时间
#define PP_RMTCFG_WAIT_TIMEOUT 	(5*1000)//等待超时时间
#define	PP_INIT_EVENTID			0x0

#define	PP_RETRANSMIT_TIMES		5

/***********宏函数***********/


/*******************************************************
description： struct definitions
*******************************************************/
typedef struct
{
    unsigned int date;
    double time;
    double longitude;
    unsigned int is_east;
    double latitude;
    unsigned int is_north;
    double direction;
    double knots;       // 1kn = 1 mile/h = 1.852 km/h
    double kms;         // 1km/h = 0.5399kn
    double height;
    double hdop;
    double vdop;
}PrvtProtcfg_gpsData_t;
/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern int PrvtProtCfg_rcvMsg(unsigned char* buf,int buflen);
extern int PrvtProtCfg_ecallTriggerEvent(void);
extern int PrvtProtCfg_gpsStatus(void);
extern long PrvtProtCfg_engineSt(void);
extern long PrvtProtCfg_totalOdoMr(void);
extern long PrvtProtCfg_vehicleSOC(void);
extern void PrvtProtCfg_gpsData(PrvtProtcfg_gpsData_t *gpsDt);
#endif 
