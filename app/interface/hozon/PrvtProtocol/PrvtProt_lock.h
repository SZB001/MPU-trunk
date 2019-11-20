/******************************************************
�ļ�����	PrvtProt_lock.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/10/21		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_LOCK_H
#define		_PRVTPROT_LOCK_H

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/


/**********�곣������*********/


/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/



/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
	unsigned char	obj;
	unsigned char	flag;
}PrvtProt_lock_t;


/******enum definitions******/
typedef enum
{
	PP_DIAGLOCK_INIT = 0,//
	PP_DIAGLOCK_RMTDIAG,//
	PP_DIAGLOCK_OTA,//
    PP_DIAGLOCK_RMTCTRL//
} PP_LOCK_DIAG_OBJ;

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern unsigned char setPP_lock_otadiagmtxlock(unsigned char obj);
extern void clearPP_lock_otadiagmtxlock(unsigned char obj);
extern void InitPP_lock_parameter(void);
extern void showPP_lock_mutexlockstatus(void);
extern unsigned char setPP_lock_diagrmtctrlotamtxlock(unsigned char obj);
extern void clrPP_lock_diagrmtctrlotamtxlock(unsigned char obj);
#endif 
