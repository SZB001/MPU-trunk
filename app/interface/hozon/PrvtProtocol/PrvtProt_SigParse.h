/******************************************************
�ļ�����	PrvtProt_SigParse.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_SIGPARSE_H
#define		_PRVTPROT_SIGPARSE_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_RMTCTRL_CANSIGN  0x01//


// ��������
#define PP_CANSIGN_DOORLOCK       		0x00//
#define PP_CANSIGN_DOORUNLOCK       	0x01//
#define PP_CANSIGN_FINDCAR       		0x02//
#define PP_CANSIGN_REARDOOROPEN     	0x03//
#define PP_CANSIGN_REARDOORCLOSE    	0x04//
#define PP_CANSIGN_SUNROOFOPEN       	0x05//
#define PP_CANSIGN_SUNROOFCLOSE       	0x06//
#define PP_CANSIGN_SUNROOFUPWARP       	0x07//
#define PP_CANSIGN_HIGHVOIELEC     		0x08//
#define PP_CANSIGN_HIGHPRESSELEC     	0x09//
#define PP_CANSIGN_ACON    				0x0A//
#define PP_CANSIGN_LHTEMP       		0x0B//
#define PP_CANSIGN_ACOFF       			0x0C//
#define PP_CANSIGN_DRIVHEATING       	0x0D//
#define PP_CANSIGN_PASSHEATING       	0x0E//
#define PP_CANSIGN_CHARGEON     		0x0F//
#define PP_CANSIGN_CHARGEOFF    		0x10//
#define PP_CANSIGN_ENGIFORBID       	0x11//
#define PP_CANSIGN_CANCELENGIFORBID     0x12//
#define PP_CANSIGN_AUTHEST     			0x13//
#define PP_CANSIGN_AUTHEFAILRESION    	0x14//
#define PP_CANSIGN_CHARGEAPPOINTEN    	0x15//
#define PP_CANSIGN_READYLIGHTST    		0x16//
#define PP_CANSIGN_OTAMODEFAILSTS    	0x17//
#define PP_CANSIGN_PM25VALID    		0x18//空气净化器状态/pm2.5有效性
#define PP_CANSIGN_CO2DENSITYSTS    	0x19//CO2浓度报警状态
#define PP_CANSIGN_BLUETOOTHSTARTST    	0x1A//蓝牙一键启动状态
#define PP_CANSIGN_CHRGGUNCNCTLIST    	0x1B//充电枪连接状态
#define PP_CANSIGN_ODOMETERUPDATE    	0x1C//总计里程同步状态
#define PP_CANSIGN_ACAUTOST    	        0x1D//空调auto状态
#define PP_CANSIGN_HVREADY    	        0x1F//HV Ready状态
#define PP_CANSIGN_BTMCHARGECMD    	    0x20//BTM充电请求
#define PP_CANSIGN_VCU5SYSFAUTST    	0x21//VCU5 System Faults status
#define PP_CANSIGN_MCU2FAULTLVL     	0x22//mcu2 fault level
#define PP_CANSIGN_OVERCURRENTST     	0x23//总电流过流
#define PP_CANSIGN_DIAGST            	0x24//动力电池系统故障
#define PP_CANSIGN_AVHLAMPREQ           0x25//AVH Lamp Req
#define PP_CANSIGN_EGSMERRST            0x26//EGSM故障状态
#define PP_CANSIGN_ESCBRAKEWARNMAMP     0x27//ESC Brake Warning Lamp
#define PP_CANSIGN_BMSDISCHGFLT         0x28//当前最高放电故障等级
#define PP_CANSIGN_VCUSYSLIGHTST        0x29//系统故障灯-限功率
#define PP_CANSIGN_VCUPWRTRILVL         0x2A//动力系统故障等级
#define PP_CANSIGN_EHBFAIST             0x2B//EHBS故障及有效性
#define PP_MAX_RMTCTRL_CANSIGN_INFO   (PP_CANSIGN_EHBFAIST + 1)
/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/
typedef struct
{
    int info[PP_MAX_RMTCTRL_CANSIGN_INFO];
}PP_rmtCtrl_canSign_t;

typedef struct
{
	PP_rmtCtrl_canSign_t rmtCtrlSign;
}PP_canSign_t;


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
extern void InitPrvtProt_SignParse_Parameter(void);
extern int PrvtProt_data_parse_surfix(int sigid, const char *sfx);
extern unsigned char PrvtProt_SignParse_findcarSt(void);
extern unsigned char PrvtProt_SignParse_sunroofSt(void);
extern unsigned char PrvtProt_SignParse_RmtStartSt(void);
extern unsigned char PrvtProt_SignParse_DrivHeatingSt(void);
extern unsigned char PrvtProt_SignParse_PassHeatingSt(void);
extern unsigned char PrvtProt_SignParse_cancelEngiSt(void);
extern unsigned char PrvtProt_SignParse_autheSt(void);
extern unsigned char PrvtProt_SignParse_authefailresion(void);
extern unsigned char PrvtProt_SignParse_chrgAptEnSt(void);
extern unsigned char PrvtProt_SignParse_chrgOnOffSt(void);
extern unsigned char PrvtProt_SignParse_readyLightSt(void);
extern unsigned char PrvtProt_SignParse_OtaFailSts(void);
extern unsigned char PrvtProt_SignParse_pm25valid(void);
extern unsigned char PrvtProt_SignParse_CO2DensitySt(void);
extern unsigned char PrvtProt_SignParse_BleStartSt(void);
extern unsigned char PrvtProt_SignParse_chrgGunCnctSt(void);
extern unsigned char PrvtProt_SignParse_OdomtrUpdtSt(void);
extern unsigned char PrvtProt_SignParse_ACAutoSt(void);
extern unsigned char PrvtProt_SignParse_HVReadySt(void);
extern unsigned char PrvtProt_SignParse_BTMChrgCmd(void);
extern unsigned char PrvtProt_SignParse_SysFaultSt(void);
extern unsigned char PrvtProt_SignParse_Mcu2FltLvl(void);
extern unsigned char PrvtProt_SignParse_overCurrSt(void);
extern unsigned char PrvtProt_SignParse_DiagSts(void);
extern unsigned char PrvtProt_SignParse_AVHLampSts(void);
extern unsigned char PrvtProt_SignParse_EGSMErrSt(void);
extern unsigned char PrvtProt_SignParse_BkWarnLampSt(void);
extern unsigned char PrvtProt_SignParse_BMSDisChgFlt(void);
extern unsigned char PrvtProt_SignParse_VCUSysLightSt(void);
extern unsigned char PrvtProt_SignParse_VCUPwrTriLvl(void);
extern unsigned char PrvtProt_SignParse_EHBFaiSt(void);
#endif 
