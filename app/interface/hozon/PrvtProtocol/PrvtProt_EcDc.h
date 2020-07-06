/******************************************************
�ļ�����	PrvtProt_EcDc.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/4/29		V1.0			liujian
*******************************************************/
#ifndef		_PRVTPROT_ECDC_H
#define		_PRVTPROT_ECDC_H
/*******************************************************
description�� include the header file
*******************************************************/


/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_ECDC_DATA_LEN 	1024//����

#define PP_ENCODE_DISBODY 	0x01//����dispatcher header
#define PP_ENCODE_APPDATA 	0x02//����app data


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
	/*XCALL*/
	ECDC_XCALL_REQ = 0,//xcall request
    ECDC_XCALL_RESP,//xcall response

	/*remote config*/
	ECDC_RMTCFG_CHECK_REQ,//check remote config req
	ECDC_RMTCFG_GET_REQ,//get remote config req
	ECDC_RMTCFG_END_REQ,//remote config req end
	ECDC_RMTCFG_READ_REQ,//remote config read req
	ECDC_RMTCFG_CONN_RESP,//remote config conn resp
	ECDC_RMTCFG_READ_RESP,//remote config read resp
	ECDC_RMTCTRL_RESP,//remote control resp
	ECDC_RMTCTRL_BOOKINGRESP,//remote control booking resp
	ECDC_RMTCTRL_HUBOOKINGRESP,//remote control HU booking resp
	ECDC_RMTVS_RESP,//remote check vehi status response

	/*remote diag*/
	ECDC_RMTDIAG_RESP,//remote diag status response
	ECDC_RMTDIAG_STATUS,//remote diag status
	ECDC_RMTDIAG_IMAGEACQRESP,//remote diag image acq response
	ECDC_RMTDIAG_CLEANFAULTRESP,//remote clean fault code response
	ECDC_RMTDIAG_LOGACQRESP,//remote log acq response
	ECDC_RMTDIAG_STOPLOGACQRESP,//remote stop log acq response
	ECDC_RMTDIAG_CANACQRESP,//remote can acq response

	/*FOTA INFO PUSH*/
	ECDC_FIP_REQ,//request
    ECDC_FIP_RESP,//response
	ECDC_FIP_TASKREQ,//request
    ECDC_FIP_TASKRESP,//response
	ECDC_APP_MID_MAX
} ECDC_APP_MID_TYPE;//Ӧ������
/*****struct definitions*****/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,int *msgDataLen, \
							  void *disptrBody, void *appchoice);
extern int PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
										void *DisBody,void *appData);
#endif 
