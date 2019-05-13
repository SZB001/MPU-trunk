/******************************************************
�ļ�����	PrvtProt.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_H
#define		_PRVTPROT_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_ACK_WAIT 	0x01//�ȴ��ɹ�
#define PP_ACK_SUCCESS 	0x02//Ӧ��ɹ�

#define PP_MSG_DATA_LEN 	1024//message data ����

#define	PP_NATIONALSTANDARD_TYPE	0//�������ͣ���������
#define	PP_HEARTBEAT_TYPE			1//��������
#define	PP_NGTP_TYPE				2//NGTP����

/* xcall */
//AID
#define PP_AID_XCALL 	170//Xcall
//MID
#define PP_MID_XCALL_REQ 	1//Xcall request
#define PP_MID_XCALL_RESP 	2//Xcall response

/* remote config */
//AID
#define PP_AID_CFG_CHECK_REQ 	100//remote config
//MID

/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
/*typedef enum
{
	PP_APP_HB = 0,//����
	PP_APP_XCALL,//
	PP_APP_MAX
} PP_APP_INDEX;
*/

typedef enum
{
	PP_IDLE = 0,//
    PP_HEARTBEAT,//�ȴ�������Ӧ״̬
} PP_WAIT_STATE;

/*****struct definitions*****/
typedef struct 
{		
	unsigned char sign[2U];/* ��ʼ����־λ��ȡֵ0x2A��0x2A*/
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char mnr : 4;/* С�汾(��TSPƽ̨����)*/
			unsigned char mjr : 4;/* ��汾(��TSPƽ̨����)*/
		}bits; /**/
	}ver;
	unsigned long int nonce;/* TCP�ỰID ��TSPƽ̨���� */
	
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char encode : 4;/* ���뷽ʽ0��none��1NGTP��2��GZIP��3��JSON*/	
			unsigned char mode : 1;/* ����0��normal��1��debug*/
			unsigned char connt  : 1;/* ���ӷ�ʽ0�������ӣ�1��������*/
			unsigned char asyn : 1;/* ͨѶ��ʽ0��ͬ����1���첽*/
			unsigned char dir  : 1;/* ���ķ���0��To Tbox��1��To TSP*/
		}bits; /**/
	}commtype;
	
	union
	{
		unsigned char Byte;/* */
		struct 
		{		
			unsigned char encrypt   : 4;/* ���ܷ�ʽ0��none��1��AES128��2��AES256��3RSA2048*/
			unsigned char signature : 4;/* ǩ����ʽ:0 -- none ;1 -- SHA1;2 -- SHA256*/	
		}bits; /**/
	}safetype;
	
	unsigned char opera;/* ��������:0 -- national standard ;1 -- heartbeat;2 -- ngtp ;3 -- OTA */
	unsigned long int msglen;/* ���ĳ��� */
	unsigned long int tboxid;/* ƽ̨ͨ��tboxID��tboxSNӳ�� */
}__attribute__((packed)) PrvtProt_pack_Header_t; /*����ͷ�ṹ��*/ 

typedef struct 
{		
	PrvtProt_pack_Header_t Header;/* */
	unsigned char msgdata[PP_MSG_DATA_LEN];/* ��Ϣ�� */
	unsigned char msgtype;/* ��Ϣ���� */
}__attribute__((packed)) PrvtProt_pack_t; /*���Ľṹ��*/

typedef struct 
{		
	uint64_t  timer;/* ������ʱ�� */
	//uint8_t ackFlag;/* Ӧ���־:2-�ȴ�Ӧ��1-�ɹ�Ӧ�� */
	uint8_t state;/* ����״̬ 1- ���� */
	uint8_t period;/* ��������uints����*/
	PP_WAIT_STATE waitSt;/* �ȴ���Ӧ��״̬ */
	uint64_t waittime;/* �ȴ���Ӧ��ʱ�� */
}__attribute__((packed))  PrvtProt_heartbeat_t; /*�����ṹ��*/

typedef struct 
{	
	//PrvtProt_heartbeat_t heartbeat;
	//PrvtProt_xcall_t xcall[PP_XCALL_MAX];
	//PP_WAIT_STATE waitSt[PP_APP_MID_MAX];/* �ȴ���Ӧ��״̬ */
	//uint64_t waittime[PP_APP_MID_MAX];/* �ȴ���Ӧ��ʱ�� */
	char suspend;/* ��ͣ */
	uint32_t nonce;/* TCP�ỰID ��TSPƽ̨���� */
	unsigned char version;/* ��/С�汾(��TSPƽ̨����)*/
	uint32_t tboxid;/* ƽ̨ͨ��tboxID��tboxSNӳ�� */
}__attribute__((packed))  PrvtProt_task_t; /* ��������ṹ��*/

/* Dispatcher Body struct */
typedef struct
{
	uint8_t	 	aID[3];
	uint8_t	 	mID;
	long	eventTime;
	long	expTime	/* OPTIONAL */;
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
/**************************** Xcall ******************************/
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

typedef struct
{
	long xcallType;//����  1-��·��Ԯ   2-������Ԯ��ecall��  3-400�绰����
	long engineSt;//����״̬��1-Ϩ��2-����
	long totalOdoMr;//�����Ч��Χ��0 - 1000000��km��
	PrvtProt_Rvsposition_t gpsPos;//������Ԯλ��
	long srsSt;//��ȫ����״̬ 1- ������2 - ����
	long updataTime;//����ʱ���
	long battSOCEx;//�������ʣ�������0-10000��0%-100%��
}PrvtProt_App_Xcall_t;
/******************************************************************/

typedef struct
{
	PrvtProt_App_Xcall_t Xcall;//xcall
}PrvtProt_appData_t;

/* message data struct */
typedef struct
{
	PrvtProt_DisptrBody_t	DisBody;
	PrvtProt_appData_t 		appData;
}PrvtProt_msgData_t;
/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/
//extern PrvtProt_task_t 	pp_task;
//extern PrvtProt_pack_Header_t 	PP_PackHeader[PP_APP_MID_MAX];
//extern PrvtProt_pack_t 			PP_Pack[PP_APP_MID_MAX];
//extern PrvtProt_DisptrBody_t	PP_DisptrBody[PP_APP_MID_MAX];
extern PrvtProt_appData_t 		PP_appData;//���õĽṹ�壨app data���ݴ��ʱʹ�ã�
/*******************************************************
description�� function External declaration
*******************************************************/
extern long PrvtPro_BSEndianReverse(long value);
extern long PrvtPro_getTimestamp(void);
#endif 
