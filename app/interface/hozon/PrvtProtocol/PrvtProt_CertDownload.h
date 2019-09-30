/******************************************************
�ļ�����	PrvtProt_CertDoenload.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CERTDOWNLOAD_H
#define		_PRVTPROT_CERTDOWNLOAD_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********宏开关*********/

/**********宏常量*********/
#define PP_CHECK_CERT_IDLE			0
#define PP_CHECK_CERT_AVAILABILITY	1
#define PP_CHECK_CERT_DL			2
#define PP_CHECK_CERT_RENEWCERT		3
#define PP_CHECK_CERT_VERIFYCERT	4
#define PP_CHECK_CERT_END			5

//请求下载证书
#define PP_CERTDL_IDLE					0
#define PP_CERTDL_CHECK_CIPHER_CSR		1
#define PP_CERTDL_DLREQ					2
#define PP_CERTDL_DLREQWAIT				3
#define PP_CERTDL_END					4

#define PP_CHECK_REVO_IDLE				0
#define PP_CHECK_REVO_REQ				1
#define PP_CHECK_REVO_WAIT				2
#define PP_CHECK_REVO_END				3

//启用证书
#define PP_CERTEN_IDLE				0
#define PP_CERTEN_REQ				1
#define PP_CERTEN_WAIT				2
#define PP_CERTEN_CHECKREVOLIST		3
#define PP_CERTEN_CHECKREVOWAIT		4
#define PP_CERTEN_END				5

//检查更新证书状态
#define PP_CERTUPDATA_IDLE			0
#define PP_CERTUPDATA_CKREQ			1
#define PP_CERTUPDATA_CKREQWAIT		2
#define PP_CERTUPDATA_UDREQ			3
#define PP_CERTUPDATA_UDWAIT		4
#define PP_CERTUPDATA_END			5

#define PP_CERTDL_DLTIMEOUT			60000

#define PP_CERT_DL_TXINFORMNODE 	30

#define PP_CERTDL_MID_REQ			1
#define PP_CERTDL_MID_RESP			2
#define PP_CERTDL_MID_UDREQ			3
#define PP_CERTDL_MID_UDREQRESP		4
#define PP_CERTDL_MID_CERT_STATUS	5
#define PP_CERTDL_MID_REVOLISTREQ	6
#define PP_CERTDL_MID_REVOLISTRESP	7

//证书下载mid= 2起始字节
#define PP_CERTDL_RESP_MID			0
#define PP_CERTDL_RESP_EVTID		1
#define PP_CERTDL_RESP_RESULT		5
#define PP_CERTDL_RESP_FAILTYPE		6
#define PP_CERTDL_RESP_CERTTYPE		6
#define PP_CERTDL_RESP_CERTLEN		7
#define PP_CERTDL_RESP_CERTCONTENT	11

#define PP_CERTDL_INITVAL			0
#define PP_CERTDL_SUCCESS			1
#define PP_CERTDL_FAIL				2
/***********宏函数********/

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
	uint8_t 	waitSt;
	uint64_t 	waittime;
	uint8_t		dlSt;//
	uint8_t		dlsuccess;//1 - success
	uint8_t		CertValid;//证书有效性
	uint8_t		CertEnflag;//证书启用状态
	uint8_t		certDLTestflag;//证书下载测试标志
	uint8_t		cipherexist;

	uint8_t		checkSt;
	uint8_t		verifyFlag;
	uint8_t		certAvailableFlag;
}__attribute__((packed))  PP_CertDownloadSt_t;

typedef struct
{
	uint32_t eventid;
	unsigned int tboxid;
}__attribute__((packed))  PP_CertDownloadPara_t;

/* application data struct */
/***********************************
			CertDownload
***********************************/
typedef struct
{
	uint8_t 	mid;//消息ID
	uint32_t	eventid;//事件id
	uint8_t		cerType;
	uint16_t	infoListLength;
	uint8_t		infoList[4096];
}PP_CertificateDownloadReq_t;

typedef struct
{
	uint8_t 	mid;//消息ID
	uint32_t	eventid;//事件id
	uint8_t		result;
	uint8_t		failureType;
	uint8_t		certType;
	uint32_t	certLength;
	uint8_t 	certContent[2048];
}PP_CertificateDownloadResp_t;

typedef struct
{
	uint8_t 	mid;//消息ID
	uint32_t	eventid;//事件id
	uint8_t		certType;
	uint8_t		failureType;
	uint8_t		certSnLength;
	char 		certSn[256];
}PP_CertificateStatus_t;

typedef struct
{
	uint8_t 	mid;//消息ID
	uint32_t	eventid;//事件id
	uint8_t		certType;
	uint8_t		failureType;
	uint16_t	crlLength;
}PP_CertRevoListReq_t;

typedef struct
{
	uint8_t 	mid;//消息ID
	uint32_t	eventid;//事件id
	uint8_t		certType;
	uint8_t		certSnLength;
	uint8_t		certSn[255];
	uint8_t		certSnSignLength;
	uint8_t 	certSnSign[255];
}PP_CertUpdataReq_t;

typedef struct
{
	PP_CertificateDownloadReq_t		CertDLReq;
	PP_CertificateDownloadResp_t	CertDLResp;
}PP_CertificateDownload_t;

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_CertDownload_init(void);
extern int PP_CertDownload_mainfunction(void *task);
extern void PP_CertDL_SetCertDLReq(unsigned char req);
extern void PP_CertDL_SetCertDLUpdata(unsigned char req);

#endif 
