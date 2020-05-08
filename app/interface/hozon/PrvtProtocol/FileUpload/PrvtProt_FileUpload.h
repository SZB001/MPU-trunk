/******************************************************
�ļ�����	PrvtProt_FileUpload.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/10/21		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_FILEUPLOAD_H
#define		_PRVTPROT_FILEUPLOAD_H

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define PP_FILEUPLOAD_PACKNUM_MAX   255
#define PP_FILEUPLOAD_PACKNUM_DEF   60
#define PP_FILEUPLOAD_BUFNUM        2

#define PP_FILEUPLOAD_DATALEN       1350
#define PP_FILEUPLOAD_MAXPKG        120

#define PP_FILEUPLOAD_IGNONDLYTIME     6000

#define PP_FILEUPLOAD_PATH      "/media/sdcard/fileUL/"
#define PP_CANFILEUPLOAD_PATH   "/media/sdcard/CanMsgFileUL/"


#define PP_CANFILEUL_SIGN_VCU5SYSFLT		0
#define PP_CANFILEUL_SIGN_BMSDISCHGFLT		1
#define PP_CANFILEUL_SIGN_BMSINTERLOCKST	2
#define PP_CANFILEUL_SIGN_MCU2FLTLVL		3
#define PP_CANFILEUL_SIGN_TEMPRISEFAST		4
#define PP_CANFILEUL_SIGN_CELLOVERTEMP		5
#define PP_CANFILEUL_SIGN_UNDERVOLT			6
#define PP_CANFILEUL_SIGN_VCUSYSLGHT		7
#define PP_CANFILEUL_SIGN_ISOISUPER			8
#define PP_CANFILEUL_SIGN_TEMPDIFF			9
#define PP_CANFILEUL_SIGN_BMS7DIAGSTS		10
#define PP_CANFILEUL_SIGN_EGSMERR			11
#define PP_CANFILEUL_SIGN_VCUPWRTRFAILVL	12
#define PP_CANFILEUL_SIGN_DCDCWORKST		13
#define PP_CANFILEUL_SIGN_EHBFAIL			14
#define PP_CANFILEUL_SIGN_ESCWARNLAMP		15
#define PP_CANFILEUL_SIGN_RLTYREPRESS		16
#define PP_CANFILEUL_SIGN_RLTYRETEMP		17
#define PP_CANFILEUL_SIGN_RLTYREQUCIKLK		18
#define PP_CANFILEUL_SIGN_RRTYREPRESS		19
#define PP_CANFILEUL_SIGN_RRTYRETEMP		20
#define PP_CANFILEUL_SIGN_RRTYREQUCIKLK		21
#define PP_CANFILEUL_SIGN_FLTYREPRESS		22
#define PP_CANFILEUL_SIGN_FLTYRETEMP		23
#define PP_CANFILEUL_SIGN_FLTYREQUCIKLK		24
#define PP_CANFILEUL_SIGN_FRTYREPRESS		25
#define PP_CANFILEUL_SIGN_FRTYRETEMP		26
#define PP_CANFILEUL_SIGN_FRTYREQUCIKLK		27
#define PP_CANFILEUL_SIGN_CRASHOUTPUT		28
#define PP_CANFILEUL_SIGN_EPSELESTRFAIL		29
#define PP_CANFILEUL_SIGN_WARN_MAX			(PP_CANFILEUL_SIGN_EPSELESTRFAIL + 1)

/**********�곣������*********/


/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/



/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
	uint8_t oldSt;
	uint8_t newSt;
}__attribute__((packed))  PP_FileUpload_warnSign_t;

typedef struct
{
    int len;
	uint8_t data[2*PP_FILEUPLOAD_DATALEN];
}__attribute__((packed))  PP_FileUpload_Pack_t;


typedef struct
{
	uint8_t successflag;
    uint8_t cnt;
	PP_FileUpload_Pack_t pack[PP_FILEUPLOAD_PACKNUM_MAX];
}__attribute__((packed))  PP_FileUpload_Buf_t;

typedef struct
{
	PP_FileUpload_warnSign_t	warnSign[PP_CANFILEUL_SIGN_WARN_MAX];
	uint64_t tasktime;
    uint8_t index;
	PP_FileUpload_Buf_t buffer[PP_FILEUPLOAD_BUFNUM];
	uint8_t network;
	int pkgnum;
	uint8_t tspReqFlag;
 	int tspReqTime;
	uint8_t signTrigFlag;
	uint64_t ignonDlyTime;
}__attribute__((packed))  PP_FileUpload_t;

/******enum definitions******/

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void InitPP_FileUpload_Parameter(void);
//extern void PP_FileUpload_run(void);
#endif 
