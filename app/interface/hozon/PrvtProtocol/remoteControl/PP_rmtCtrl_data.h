/******************************************************
�ļ�����	PrvtProt_rmtCtrl_data.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMT_CTRL_DATA_H
#define		_PRVTPROT_RMT_CTRL_DATA_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_RMTCTRL_MAX_SENDQUEUE		30
/***********�꺯��***********/
typedef void (*PP_rmtCtrlsendInform_cb)(void* x);//����֪ͨ�ص�
/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/

/******struct definitions******/

/******union definitions*****/


/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_rmtCtrl_data_init(void);

#endif 
