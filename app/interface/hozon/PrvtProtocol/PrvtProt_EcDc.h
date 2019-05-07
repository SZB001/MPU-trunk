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
#define PP_ECDC_DATA_LEN 	512//����

#define PP_ENCODE_DISBODY 	0x01//����dispatcher header
#define PP_ENCODE_APPDATA 	0x02//����app data

//AID����
#define PP_AID_ECALL 	170//Xcall

//MID����
#define PP_MID_ECALL_REQ 	1//Xcall request
#define PP_MID_ECALL_RESP 	2//Xcall response

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
	PP_APP_HEARTBEAT = 0,//����
	PP_ECALL_REQ,//ecall request
    PP_ECALL_RESP,//ecall response
	PP_APP_MAX
} PP_APP_TYPE;//Ӧ������

/*****struct definitions*****/
/* Dispatcher Body struct */
typedef struct 
{
	uint8_t	 	aID[3];
	uint8_t	 	mID;
	long	eventTime;
	long	eventId	/* OPTIONAL */;
	long	ulMsgCnt	/* OPTIONAL */;
	long	dlMsgCnt	/* OPTIONAL */;
	long	msgCntAcked	/* OPTIONAL */;
	int		ackReq	/* OPTIONAL */;
	long	appDataLen	/* OPTIONAL */;
	long	appDataEncode	/* OPTIONAL */;
	long	appDataProVer	/* OPTIONAL */;
	long	testFlag	/* OPTIONAL */;
	long	result	/* OPTIONAL */;
}PrvtProt_DisptrBody_t;

/* application data struct */
typedef struct 
{
	int  gpsSt;//gps״̬ 0-��Ч��1-��Ч
	long gpsTimestamp;//gpsʱ���
	long latitude;//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long longitude;//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long altitude;//�߶ȣ�m��
	long heading;//��ͷ����Ƕȣ�0Ϊ��������
	long gpsSpeed;//�ٶ� x 10����λkm/h
	long hdop;//ˮƽ�������� x 10
}PrvtProt_Rvsposition_t;

/* application data struct */
typedef struct 
{
	long xcallType;//����  1-��·��Ԯ   2-������Ԯ��ecall��  3-400�绰����
	long engineSt;//����״̬��1-Ϩ��2-����
	long totalOdoMr;//�����Ч��Χ��0 - 1000000��km��
	PrvtProt_Rvsposition_t gpsPos;//������Ԯλ��
	long srsSt;//��ȫ����״̬ 1- ������2 - ����
	long updataTime;//����ʱ���
	long battSOCEx;//�������ʣ�������0-10000��0%-100%��
}PrvtProt_EcDc_Xcall_t;

typedef struct
{
	PrvtProt_EcDc_Xcall_t Xcall;//xcall
}PrvtProt_appData_t;

/* message data struct */
typedef struct 
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t appData;
}PrvtProt_msgData_t;

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern int PrvtPro_msgPackageEncoding(uint8_t type,uint8_t *msgData,long *msgDataLen, \
							  PrvtProt_DisptrBody_t *DisptrBody, PrvtProt_appData_t *Appchoice);
extern void PrvtPro_decodeMsgData(uint8_t *LeMessageData,int LeMessageDataLen, \
								  PrvtProt_msgData_t *msgData);
#endif 
