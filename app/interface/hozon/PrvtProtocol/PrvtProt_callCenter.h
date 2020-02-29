/******************************************************
�ļ�����	PrvtProt_callCenter.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CALL_CENTER_H
#define		_PRVTPROT_CALL_CENTER_H
/*******************************************************
description�� include the header file
*******************************************************/


/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/

/***********�꺯��***********/
#define PrvtProt_CC_callreq() 0//call request

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/


/*****struct definitions*****/
typedef struct
{
	char callreq;/* ��ͣ */
}__attribute__((packed))  PrvtProt_CC_task_t; /* ��������ṹ��*/
/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PrvtProt_CC_init(void);
extern int PrvtProt_CC_mainfunction(void *task);
#endif 
