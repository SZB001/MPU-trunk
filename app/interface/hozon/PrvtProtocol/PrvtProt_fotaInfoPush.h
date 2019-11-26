/******************************************************
文件名：	PrvtProt_fotaInfoPush.h

描述：	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_FOTAINFOPUSH_H
#define		_PRVTPROT_FOTAINFOPUSH_H
/*******************************************************
description£º include the header file
*******************************************************/
#define PP_FIP_INFOPUSH_SUCCESS	1
#define PP_FIP_INFOPUSH_FAIL	2
/*******************************************************
description£º macro definitions
*******************************************************/
/**********ºê¿ª¹Ø¶¨̉å*********/

/**********ºê³£Á¿¶¨̉å*********/

/***********ºêº¯Êư***********/

/*******************************************************
description£º struct definitions
*******************************************************/

/*******************************************************
description£º typedef definitions
*******************************************************/
/******enum definitions******/

/*****struct definitions*****/
typedef struct
{
	long fotaNotice;
	long sid;
	long noticeStatus;
}PrvtProt_App_FIP_t;
/******union definitions*****/

/*******************************************************
description£º variable External declaration
*******************************************************/

/*******************************************************
description£º function External declaration
*******************************************************/
extern void PP_FotaInfoPush_init(void);
extern int PP_FotaInfoPush_mainfunction(void *task);
extern void PP_FIP_shellReq(void);
#endif 
