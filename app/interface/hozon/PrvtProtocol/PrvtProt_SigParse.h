/******************************************************
文件名：	PrvtProt_SigParse.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_SIGPARSE_H
#define		_PRVTPROT_SIGPARSE_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_RMTCTRL_CANSIGN  0x01//


// 其他故障
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
#define PP_MAX_RMTCTRL_CANSIGN_INFO   (PP_CANSIGN_CANCELENGIFORBID + 1)
/***********宏函数***********/


/*******************************************************
description： struct definitions
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
extern int PrvtProt_data_parse_surfix(int sigid, const char *sfx);
extern uint8_t PrvtProt_SignParse_findcarSt(void);
extern uint8_t PrvtProt_SignParse_sunroofSt(void);
extern uint8_t PrvtProt_SignParse_RmtStartSt(void);
extern uint8_t PrvtProt_SignParse_DrivHeatingSt(void);
extern uint8_t PrvtProt_SignParse_PassHeatingSt(void);
extern uint8_t PrvtProt_SignParse_cancelEngiSt(void);
#endif 
