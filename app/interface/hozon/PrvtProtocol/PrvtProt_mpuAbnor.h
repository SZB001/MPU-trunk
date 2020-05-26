/******************************************************
�ļ�����	PrvtProt_mpuAbnor.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_MPUABNOR_H
#define		_PRVTPROT_MPUABNOR_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_ICCIDCHECK_TIMEOUT 	35000
#define PP_MPUREBOOT_TIMES 		5




/***********�꺯��***********/

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
	//char IGNoldst;
	char IGNnewst;
	char icciderrflag;
	uint64_t iccidchktimer;

	uint32_t datetime;
	uint8_t  reboottimes;
}__attribute__((packed))  PP_mpuAbnor_t;

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void InitPrvtPro_mpuAbnor(void);
extern void PrvtPro_mpuAbnorHandle(void);
extern void PrvtPro_mpureboottest(void);
#endif 
