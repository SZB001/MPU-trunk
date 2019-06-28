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
	PP_RMTDIAG_MAX_RESP
} PP_RMTDIAG_RESPTYPE;


typedef enum
{
	PP_DIAG_ALL = 0,//
	PP_DIAG_VCU,//
    PP_DIAG_BMS,//
	PP_DIAG_MCUp,
	PP_DIAG_OBCp,
	PP_DIAG_FLR,
	PP_DIAG_FLC,
	PP_DIAG_APA,
	PP_DIAG_ESCPluse,
	PP_DIAG_EPS,
	PP_DIAG_EHB,
	PP_DIAG_BDCM,
	PP_DIAG_GW,
	PP_DIAG_LSA,
	PP_DIAG_CLM,
	PP_DIAG_PTC,
	PP_DIAG_EACP,
	PP_DIAG_EGSM,
	PP_DIAG_ALM,
	PP_DIAG_WPC,
	PP_DIAG_IHU,
	PP_DIAG_ICU,
	PP_DIAG_IRS,
	PP_DIAG_DVR,
	PP_DIAG_TAP,
	PP_DIAG_MFCP,
	PP_DIAG_TBOX,
	PP_DIAG_ACU,
	PP_DIAG_PLG
} PP_RMTDIAG_TYPE;

typedef enum
{
	PP_DIAGRESP_IDLE = 0,//
	PP_DIAGRESP_PENDING,//
	PP_DIAGRESP_END
} PP_RMTDIAG_DIAGRESP_ST;

typedef enum
{
	PP_IMAGEACQRESP_IDLE = 0,//
	PP_IMAGEACQRESP_INFORM_HU,//֪ͨHU
	PP_IMAGEACQRESP_WAITHURESP,//�ȴ�HU��Ӧ
	PP_IMAGEACQRESP_END
} PP_RMTDIAG_IMAGEACQRESP_ST;

/*****struct definitions*****/

typedef struct
{
	uint8_t  diagReq;
	uint8_t  diagType;
	long	 diageventId;
	uint8_t  ImageAcquisitionReq;
	uint8_t  dataType;
	uint8_t  cameraName;
	uint32_t effectiveTime;
	uint32_t sizeLimit;
	uint8_t  result;//�ɼ�����֪ͨ״̬
	uint8_t  failureType;//�ɼ�����ʧ������
	char     fileName[255];//�ɼ������ļ���
	uint8_t  diagrespSt;
	uint8_t  ImageAcqRespSt;
	uint8_t  waitSt;
	uint64_t waittime;
}PrvtProt_rmtDiagSt_t; /*�ṹ��*/

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
	uint8_t faultnum;
}PP_DiagnosticStatus_t;

typedef struct
{
	long dataType;
	long cameraName;
	long effectiveTime;
	long sizeLimit;
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
	PP_DiagnosticReq_t  		DiagnosticReq;
	PP_DiagnosticResp_t 		DiagnosticResp;
	PP_DiagnosticStatus_t 		DiagnosticSt;
	PP_ImageAcquisitionReq_t 	ImageAcquisitionReq;
	PP_ImageAcquisitionResp_t 	ImageAcquisitionResp;
	PP_LogAcquisitionResp_t		LogAcquisitionResp;
	PP_LogAcquisitionRes_t		LogAcquisitionRes;
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
extern void PP_diag_SetdiagReq(unsigned char diagType);

#endif 
