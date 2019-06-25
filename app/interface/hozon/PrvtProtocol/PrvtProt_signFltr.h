/******************************************************
文件名：	PrvtProt_signFltr.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_SIGNFLTR_H
#define		_PRVTPROT_SIGNFLTR_H

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/


/**********宏常量定义*********/
#define PPSIGNFLTR_FLTRTIME   30U/*滤波时间门限*/


#define PPSIGNFLTR_ON		1
#define PPSIGNFLTR_OFF		1

/***********宏函数***********/


/*******************************************************
description： struct definitions
*******************************************************/



/*******************************************************
description： typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
	uint8_t e_u_St;/*开/关状态*/
	uint8_t e_u_OnFltrCnt;/*开/关开启状态滤波计时器*/
	uint8_t e_u_OffFltrCnt;/*开/关关闭状态滤波计时器*/
}PPsignFltrStruct;

/******enum definitions******/
typedef enum
{
	bdcmAuthSt,	  /*BDCM认证状态*/
	PPSIGNFLTR_NUM	  /*外设数量*/
}PPsignFltrEnum;

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void InitPPsignFltr_Parameter(void);
extern void TskPPsignFltr_MainFunction(void);
extern uint8_t GetPPsignFltr_u_PeriSt(uint8_t LePPsignFltr_u_Index);
#endif 
