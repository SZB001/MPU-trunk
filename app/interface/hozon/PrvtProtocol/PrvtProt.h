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

#define	PP_TXINFORM_NUM		1000

//OperationType
#define	PP_OPERATETYPE_NS			0//NationalStandard
#define	PP_OPERATETYPE_HEARTBEAT	1//Heartbeat
#define	PP_OPERATETYPE_NGTP			2//NGTP
#define	PP_OPERATETYPE_OTA			3//ota
#define	PP_OPERATETYPE_RESULT		4//result
#define	PP_OPERATETYPE_CERTDL		5//CertDownload
#define	PP_OPERATETYPE_KEYEXC		6//KeyExchange
#define	PP_OPERATETYPE_FOTA			7//FOTA
#define	PP_OPERATETYPE_FILEUPLOAD	8//FileUpload

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
#define PP_MID_RMTCTRL_HUBOOKBACKRESP 	5//remote ctrl HU booking back response

#define PP_AID_VS	 		130//����״̬
//MID
#define PP_MID_VS_REQ 	1//VS request
#define PP_MID_VS_RESP 	2//VS response

#define PP_AID_DIAG	 		140//Զ�����
//MID
#define PP_MID_DIAG_REQ 			1//request
#define PP_MID_DIAG_RESP 			2//response
#define PP_MID_DIAG_STATUS 			3
#define PP_MID_DIAG_IMAGEACQREQ  	4
//#define PP_MID_DIAG_LOGACQRESP 	5
#define PP_MID_DIAG_FAULTCODECLEAN  6
#define PP_MID_DIAG_FAULTCODECLEANRESP		7
#define PP_MID_DIAG_CANBUSMSGCOLLREQ		8
#define PP_MID_DIAG_CANACQRESP 		9
#define PP_MID_DIAG_LOGACQREQ 		10
#define PP_MID_DIAG_LOGACQRESP 		11
#define PP_MID_DIAG_STOPLOGREQ 		12
#define PP_MID_DIAG_STOPLOGRESP 	13

#define PP_AID_OTAINFOPUSH	 		180//
//MID
#define PP_MID_OTA_INFOPUSHREQ 		1//request
#define PP_MID_OTA_INFOPUSHRESP 	2//response
#define PP_MID_OTA_FOTATASKREQ 		3//request
#define PP_MID_OTA_FOTATASKRESP 	4//response


#define PP_TXPAKG_FAIL 	(-1)//���ķ���ʧ��
#define PP_TXPAKG_SUCCESS 	  1//���ķ��ͳɹ�

#define	PP_APPDATA_ENCODING_UPER	0
#define	PP_APPDATA_ENCODING_DER		1
#define	PP_APPDATA_ENCODING_BER		2

/***********�꺯��***********/
typedef void (*PrvtProt_InitObj)(void);//��ʼ��
typedef int (*PrvtProt_mainFuncObj)(void* x);//
extern long PP_UpMsgCnt;
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
	PP_RMTFUNC_SYNC,
	PP_RMTFUNC_OTAPUSH,
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
	//uint8_t ackFlag;/* Ӧ���־:2-�ȴ�Ӧ��1-�ɹ�Ӧ�� */
	//uint8_t state;/* ����״̬ 1- ���� */
	uint8_t period;/* ��������uints����*/
	uint8_t hbtype;
	PP_WAIT_STATE waitSt;/* �ȴ���Ӧ��״̬ */
	uint64_t waittime;/* �ȴ���Ӧ��ʱ�� */
	char timeoutCnt;
	char hbtasksleepflag;
	char hbtaskflag;
	char hbtimeoutflag;
	char IGNoldst;
	char IGNnewst;
	uint64_t resettimer;
	uint64_t  timer;/* ������ʱ�� */
}__attribute__((packed))  PrvtProt_heartbeat_t; /*�����ṹ��*/

typedef struct 
{	
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
	char pakgtype;//发送报文的类型：周期还是单次
	uint64_t eventtime;//发送事件的时间
	char successflg;//发送成功标志
	uint8_t failresion;
	uint64_t txfailtime;//发送失败时间
	char *description;
}PrvtProt_TxInform_t; /*�ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/
//extern PrvtProt_appData_t 		PP_appData;//���õĽṹ�壨app data���ݴ��ʱʹ�ã�
extern PrvtProt_TxInform_t 	PP_TxInform[PP_TXINFORM_NUM];
/*******************************************************
description�� function External declaration
*******************************************************/
extern long PrvtPro_BSEndianReverse(long value);
extern long PrvtPro_getTimestamp(void);
extern long  PP_rmtCtrl_usTimestamp(void);
extern uint8_t PrvtProt_tboxsnValidity(void);
extern void PrvtProt_gettboxsn(char *tboxsn);
extern int PP_getIdleNode(void);
#endif 
