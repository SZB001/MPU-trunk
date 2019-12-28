/******************************************************
�ļ�����	PrvtProt_cfg.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CFG_H
#define		_PRVTPROT_CFG_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define PP_THREAD   1//�����Ƿ񵥶������߳� 1-�� 0-����
#define PP_SOCKPROXY   1//�����Ƿ�ʹ��socket����(�Ƿ�������ģ�鴴��socket��·) 1-�� 0-����
/**********�곣������*********/
#define PP_HEART_BEAT_TIME (10)//
#define PP_HEART_BEAT_TIME_SLEEP (180)//

#define PP_HB_WAIT_TIMEOUT 	(2*1000)//�����ȴ���ʱʱ��
#define PP_XCALL_WAIT_TIMEOUT 	(5*1000)//�ȴ���ʱʱ��
#define PP_RMTCFG_WAIT_TIMEOUT 	(5*1000)//�ȴ���ʱʱ��
#define	PP_INIT_EVENTID			0x0

#define	PP_RETRANSMIT_TIMES		1

/***********�꺯��***********/


/*******************************************************
description�� struct definitions
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
description�� typedef definitions
*******************************************************/
/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern int PrvtProtCfg_rcvMsg(unsigned char* buf,int buflen);
extern char PrvtProtCfg_ecallTriggerEvent(void);
extern char PrvtProtCfg_bcallTriggerEvent(void);
extern char PrvtProtCfg_detectionTriggerSt(void);
extern int PrvtProtCfg_gpsStatus(void);
extern long PrvtProtCfg_engineSt(void);
extern long PrvtProtCfg_totalOdoMr(void);
extern long PrvtProtCfg_vehicleSOC(void);
extern void PrvtProtCfg_gpsData(PrvtProtcfg_gpsData_t *gpsDt);
extern int PrvtProtCfg_get_iccid(char *iccid);
extern uint8_t PrvtProtCfg_CrashOutputSt(void);
extern uint8_t PrvtProtCfg_chargeSt(void);
extern void PrvtProtCfg_ecallSt(uint8_t st);
extern void PrvtProtCfg_bcallSt(uint8_t st);
extern uint8_t PrvtProtCfg_sunroofSt(void);
extern uint8_t PrvtProtCfg_reardoorSt(void);
extern long PrvtProtCfg_ResidualOdometer(void);
extern long PrvtProtCfg_ACChargeRemainTime(void);
extern long PrvtProtCfg_TyrePre(uint8_t obj);
extern long PrvtProtCfg_TyreTemp(uint8_t obj);
extern long PrvtProtCfg_TotalOdometer(void);
extern long PrvtProtCfg_TotalVoltage(void);
extern long PrvtProtCfg_TotalCurrent(void);
extern long PrvtProtCfg_trip(void);
#endif 
