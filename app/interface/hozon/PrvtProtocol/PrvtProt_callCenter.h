/******************************************************
文件名：	PrvtProt_callCenter.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CALL_CENTER_H
#define		_PRVTPROT_CALL_CENTER_H
/*******************************************************
description： include the header file
*******************************************************/


/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/

/***********宏函数***********/
#define PrvtProt_CC_callreq() 0//call request

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/


/*****struct definitions*****/
typedef struct
{
	char callreq;/* 暂停 */
}__attribute__((packed))  PrvtProt_CC_task_t; /* 任务参数结构体*/
/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PrvtProt_CC_init(void);
extern int PrvtProt_CC_mainfunction(void *task);
extern void PrvtPro_SetcallCCReq(unsigned char req);
#endif 
