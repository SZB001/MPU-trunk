/******************************************************
�ļ�����	PrvtProt_rmtDiag.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMTDIAG_H
#define		_PRVTPROT_RMTDIAG_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_DIAG_WAITTIME    2500//�ȴ�HU��Ӧʱ��
#define PP_DIAG_MAX_REPORT  80//һ������ϱ��Ĺ�������

#define PP_DIAGPWRON_WAITTIME    	5000//5s
#define PP_DIAGQUERY_WAITTIME    	5000//5s
#define PP_FAULTCODECLEAN_WAITTIME    	5000//5s
#define PP_DIAGQUERYALL_WAITTIME	90000//90s
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
	PP_RMTDIAG_RESP_REQ = 0,//
	PP_RMTDIAG_RESP_IMAGEACQREQ,//
	PP_RMTDIAG_STATUS,//
	PP_RMTDIAG_MAX_RESP
} PP_RMTDIAG_RESPTYPE;

typedef enum
{
	PP_DIAGRESP_IDLE = 0,//
	PP_DIAGRESP_QUERYFAILREQ,//�����ѯ����
	PP_DIAGRESP_QUERYWAIT,//�ȴ���ѯ���Ӧ��
	PP_DIAGRESP_QUERYUPLOAD,//��ѯ�ϱ�
	PP_DIAGRESP_END
} PP_RMTDIAG_DIAGRESP_ST;

typedef enum
{
	PP_IMAGEACQRESP_IDLE = 0,//
	PP_IMAGEACQRESP_INFORM_HU,//֪ͨHU
	PP_IMAGEACQRESP_WAITHURESP,//�ȴ�HU��Ӧ
	PP_IMAGEACQRESP_END
} PP_RMTDIAG_IMAGEACQRESP_ST;

typedef enum
{
	PP_LOGACQRESP_IDLE = 0,//
	PP_LOGACQRESP_INFORM_UPLOADLOG//֪ͨ
} PP_RMTDIAG_LOGACQRESP_ST;

typedef enum
{
	PP_LOG_TBOX = 1,//
	PP_LOG_HU = 2//
} PP_RMTDIAG_LOG_TYPE;

typedef enum
{
	PP_ACTIVEDIAG_IDLE = 0,//�ϵ�
	PP_ACTIVEDIAG_CHECKREPORTST,//����ϱ����
	PP_ACTIVEDIAG_CHECKOTACOND,
	PP_ACTIVEDIAG_CHECKVEHICOND,//��鳵��
	PP_ACTIVEDIAG_QUREYWAIT,//
	PP_ACTIVEDIAG_QUERYUPLOAD,//
	PP_ACTIVEDIAG_END
} PP_RMTDIAG_ACTIVEDIAG_ST;

typedef enum
{
	PP_RMTDIAG_ERROR_NONE = 0,//
	PP_RMTDIAG_ERROR_VEHISPEED,//vehicle speed > 5km/h
	PP_RMTDIAG_ERROR_TIMEOUT,//time out
	PP_RMTDIAG_ERROR_FOTAING,
	PP_RMTDIAG_ERROR_FOTAECUDIAG,
	PP_RMTDIAG_ERROR_DIAGEVTCONFLICT
}PP_RMTDIAG_QUERYWRONGTYPE;

typedef enum
{
	PP_FAULTCODECLEAN_IDLE = 0,//
	PP_FAULTCODECLEAN_REQ,//
	PP_FAULTCODECLEAN_WAIT,//
	PP_FAULTCODECLEAN_END
} PP_FAULTCODECLEAN_ST;

/*****struct definitions*****/
typedef struct
{
	uint8_t  diagReq;
	uint8_t  diagType;
	long	 diageventId;
	long	 diagexpTime;
	uint8_t  ImageAcquisitionReq;
	long	 imagereqeventId;
	uint8_t  dataType;
	uint8_t  cameraName;
	uint16_t ImagedurationTime;
	//uint32_t effectiveTime;
	//uint32_t sizeLimit;
	uint8_t  result;//�ɼ�����֪ͨ״̬
	uint8_t  failureType;//�ɼ�����ʧ������
	char     fileName[255];//�ɼ������ļ���
	uint8_t  diagrespSt;
	uint8_t  ImageAcqRespSt;
	uint8_t  waitSt;
	uint64_t waittime;

	uint8_t  activeDiagSt;
	uint8_t  activeDiagFlag;
	uint64_t activeDiagdelaytime;
	uint8_t	 activeDiagWeek;

	uint8_t  LogAcqRespSt;
	uint8_t  LogAcquisitionReq;
	long	 logeventId;
	uint8_t  logType;
	uint8_t  logLevel;
	uint32_t startTime;
	uint16_t durationTime;

	uint8_t	 faultquerySt;

	uint8_t  cleanfaultSt;
	uint8_t  cleanfaultReq;
	uint8_t  cleanfaultType;
	long	 cleanfaulteventId;
	long	 cleanfaultexpTime;
	uint8_t	 faultCleanFinish;
	uint64_t faultcleanwaittime;
	uint8_t	 faultCleanResult;
	uint8_t	 faultCleanfailureType;
}PrvtProt_rmtDiagSt_t; /*�ṹ��*/

typedef struct
{
	uint32_t datetime;
	uint8_t  diagflag;//bit 1-7 ��ʾ ����1~7
}PP_rmtDiag_datetime_t; /*�ṹ��*/

typedef struct
{
	uint8_t  week;
	uint8_t  mask;
}PP_rmtDiag_weekmask_t; /*�ṹ��*/

/* application data struct */
/***********************************

***********************************/
/* remote Diagnostic*/
typedef struct
{
	long	diagType;
}PP_DiagnosticReq_t;

typedef struct
{
	uint8_t diagCode[5];
	uint8_t diagCodelen;
	uint8_t faultCodeType;
	uint8_t lowByte;
	long 	diagTime;
}PP_DiagCode_t;

typedef struct
{
	long	diagType;
	int		result;
	long	failureType;
	PP_DiagCode_t		diagCode[255];
	uint8_t diagcodenum;
}PP_DiagnosticResp_t;

typedef struct
{
	PP_DiagnosticResp_t		diagStatus[255];
	uint8_t diagobjnum;
}PP_DiagnosticStatus_t;

typedef struct
{
	long dataType;
	long durationTime;
	long cameraName;
	//long effectiveTime;
	//long sizeLimit;
}PP_ImageAcquisitionReq_t;

typedef struct
{
	int result;
	long failureType ;
	uint8_t fileName[255];
	uint8_t fileNamelen;
}PP_ImageAcquisitionResp_t;

typedef struct
{
	long logType;
	long logLevel;
	long startTime;
	long durationTime;
}PP_LogAcquisitionResp_t;

typedef struct
{
	long logType;
	int  result;
	long failureType;
	uint8_t fileName[255];
	uint8_t fileNamelen;
}PP_LogAcquisitionRes_t;

typedef struct
{
	long	diagType;
}PP_FaultCodeClearanceReq_t;

typedef struct
{
	long	diagType;
	int		result;
	long	failureType;
}PP_FaultCodeClearanceResp_t;

typedef struct
{
	long	durationTime;
}PP_CanBusMessageCollectReq_t;
typedef struct
{
	PP_DiagnosticReq_t  		DiagnosticReq;
	PP_DiagnosticResp_t 		DiagnosticResp;
	PP_DiagnosticStatus_t 		DiagnosticSt;
	PP_ImageAcquisitionReq_t 	ImageAcquisitionReq;
	PP_ImageAcquisitionResp_t 	ImageAcquisitionResp;
	PP_LogAcquisitionResp_t		LogAcquisitionResp;
	PP_LogAcquisitionRes_t		LogAcquisitionRes;
	PP_FaultCodeClearanceReq_t	FaultCodeClearanceReq;
	PP_FaultCodeClearanceResp_t	FaultCodeClearanceResp;
	PP_CanBusMessageCollectReq_t CanBusMessageCollectReq;
}PP_App_rmtDiag_t;



/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_rmtDiag_init(void);
extern int PP_rmtDiag_mainfunction(void *task);
extern void PP_diag_SetdiagReq(unsigned char diagType,unsigned char reqtype);
extern void PP_diag_rmtdiagtest(unsigned char diagType,unsigned char sueecss,unsigned char faultNum);
#endif 
