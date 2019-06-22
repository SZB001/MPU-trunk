/******************************************************
文件名：	PrvtProt_rmtDiag.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMTDIAG_H
#define		_PRVTPROT_RMTDIAG_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/


/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_ALL,//all
	PP_TBOX,//
    PP_HU,//
	PP_ICU,
	PP_UNKNOW
} PP_RMTDIAG_TYPE;


/*****struct definitions*****/

typedef struct
{
	uint8_t diagReq;
	uint8_t diagType;
	long	diageventId;
	uint8_t waitSt;
	uint64_t waittime;
}PrvtProt_rmtDiagSt_t; /*结构体*/

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
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_rmtDiag_init(void);
extern int PP_rmtDiag_mainfunction(void *task);

#endif 
