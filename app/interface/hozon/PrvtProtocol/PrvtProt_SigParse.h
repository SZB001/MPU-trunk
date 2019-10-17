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
#define PP_MAX_RMTCTRL_CANSIGN_INFO   (PP_CANSIGN_BLUETOOTHSTARTST + 1)
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
#endif 
