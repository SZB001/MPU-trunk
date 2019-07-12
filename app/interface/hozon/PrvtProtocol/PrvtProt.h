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
#define PP_TBOXSN_LEN 		19//tboxsn ����

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
#define PP_AID_RMTCFG	100//remote config
//MID
#define PP_MID_CHECK_CFG_REQ 	1//check config req
#define PP_MID_CHECK_CFG_RESP 	2//check config response
#define PP_MID_GET_CFG_REQ 		3//get config req
#define PP_MID_GET_CFG_RESP 	4//get config response
#define PP_MID_READ_CFG_REQ 	5//read config req
#define PP_MID_READ_CFG_RESP 	6//read config response
#define PP_MID_CONN_CFG_REQ 	7//conn config req
#define PP_MID_CONN_CFG_RESP 	8//conn config response
#define PP_MID_CFG_END 	9//end config req

/* remote ctrl */
//AID
#define PP_AID_RMTCTRL	 	110//
//MID
#define PP_MID_RMTCTRL_REQ 			1//remote ctrl request
#define PP_MID_RMTCTRL_RESP 		2//remote ctrl response
#define PP_MID_RMTCTRL_BOOKINGRESP 	3//remote ctrl booking response
#define PP_MID_RMTCTRL_HUBOOKINGRESP 	4//remote ctrl HU booking response

#define PP_AID_VS	 		130//����״̬
//MID
#define PP_MID_VS_REQ 	1//VS request
#define PP_MID_VS_RESP 	2//VS response

#define PP_AID_DIAG	 		140//Զ�����
//MID
#define PP_MID_DIAG_REQ 	1//request
#define PP_MID_DIAG_RESP 	2//response
#define PP_MID_DIAG_STATUS 			3
#define PP_MID_DIAG_IMAGEACQREQ  	4
#define PP_MID_DIAG_LOGACQRESP 		5
//#define PP_MID_DIAG_IMAGEACQRESP  	6
//#define PP_MID_DIAG_LOGACQRES		7

#define PP_TXPAKG_FAIL 	(-1)//���ķ���ʧ��
#define PP_TXPAKG_SUCCESS 	  1//���ķ��ͳɹ�

/***********�꺯��***********/
typedef void (*PrvtProt_InitObj)(void);//��ʼ��
typedef int (*PrvtProt_mainFuncObj)(void* x);//

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_RMTFUNC_XCALL = 0,//XCALL
	PP_RMTFUNC_CC,//��������
	PP_RMTFUNC_CFG,//Զ������
	PP_RMTFUNC_CTRL,//Զ�̿���
	PP_RMTFUNC_VS,//����״̬
	PP_RMTFUNC_DIAG,//Զ�����
	PP_RMTFUNC_MAX
} PP_RMTFUNC_INDEX;

typedef enum
{
	PP_TXPAKG_UNKOWNTYPE = 0,//δ֪����
	PP_TXPAKG_CONTINUE,//������
	PP_TXPAKG_SIGTRIG,//���δ�����
	PP_TXPAKG_SIGTIME//����ʱЧ��
} PP_TXPAKG_TYPE;

typedef enum
{
	PP_TXPAKG_UNKOWNFAIL = 0,//δ֪����
	PP_TXPAKG_TXFAIL,
	PP_TXPAKG_OUTOFDATE//���Ĺ���
} PP_TXPAKG_FAILTYPE;

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
	int totallen;//�����ܳ���
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
	char suspend;/* ��ͣ */
	uint32_t nonce;/* TCP�ỰID ��TSPƽ̨���� */
	unsigned char version;/* ��/С�汾(��TSPƽ̨����)*/
	uint32_t tboxid;/* ƽ̨ͨ��tboxID��tboxSNӳ�� */
}__attribute__((packed))  PrvtProt_task_t; /* ��������ṹ��*/

/* Dispatcher Body struct */
typedef struct
{
	uint8_t	 	aID[4];
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


typedef struct
{
	char funcObj;
	PrvtProt_InitObj Init;//��ʼ��
	PrvtProt_mainFuncObj mainFunc;//
}PrvtProt_RmtFunc_t; /*�ṹ��*/

typedef struct
{
	uint8_t idleflag;
	int aid;
	int mid;
	char pakgtype;//��������:1-�����ͣ���������ֱ�����ͳɹ�����2-���δ����ͣ�����ʧ�ܶ�������3-����ʱЧ�ԣ�����ʧ�ܶ�����ͬʱ���ľ���ʱЧ�ԣ�
	uint64_t eventtime;//�¼�����ʱ��ʱ��
	char successflg;//���ķ�����ɱ�־��-1 - ʧ�ܣ�1 - �ɹ���Ĭ��0
	uint8_t failresion;
	uint64_t txfailtime;//����ʧ�ܵ�ʱ��
}PrvtProt_TxInform_t; /*�ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/
//extern PrvtProt_appData_t 		PP_appData;//���õĽṹ�壨app data���ݴ��ʱʹ�ã�

/*******************************************************
description�� function External declaration
*******************************************************/
extern long PrvtPro_BSEndianReverse(long value);
extern long PrvtPro_getTimestamp(void);
extern void PrvtProt_gettboxsn(char *tboxsn);
#endif 
