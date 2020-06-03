/******************************************************
�ļ�����	PrvtProt_mpuAbnor.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_SBATTRECHRG_H
#define		_PRVTPROT_SBATTRECHRG_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define	 PP_SBRC_SLEEPTIME	60



/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_SBRC_DETEC = 0,//
	PP_SBRC_WAKE_VEHI,
	PP_SBRC_WAKE_WAIT,
	PP_SBRC_END
} PP_SBRC_ST;//״̬��
/*****struct definitions*****/
typedef struct 
{
	char IGNoldst;
	char IGNnewst;
	long sleeptimestamp;
	uint64_t tsktimer;
	uint64_t waittimer;
	char	sleepflag;
	char	TskSt;
}__attribute__((packed))  PP_SBattRechrg_t;

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void InitPrvtPro_SBattRechrg(void);
extern void PrvtPro_SBattRechrgHandle(void);
extern int  GetPrvtPro_SBattRechrgSleepSt(void);
extern void setPrvtPro_SBattRechrgWakeup(void);
#endif 
