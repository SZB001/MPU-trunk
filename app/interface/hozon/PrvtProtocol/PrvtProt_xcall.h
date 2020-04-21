/******************************************************
ÎÄ¼₫Ăû£º	PrvtProt_xcall.h

ĂèÊö£º	Æó̉µË½ÓĐĐ­̉é£¨Ơă½­ºÏÖÚ£©	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_XCALL_H
#define		_PRVTPROT_XCALL_H
/*******************************************************
description£º include the header file
*******************************************************/

/*******************************************************
description£º macro definitions
*******************************************************/
/**********ºê¿ª¹Ø¶¨̉å*********/

/**********ºê³£Á¿¶¨̉å*********/
#define PP_XCALL_ACK_WAIT 		0x01//Ó¦´đ³É¹¦
#define PP_XCALL_ACK_SUCCESS 	0x02//Ó¦´đ³É¹¦


#define	PP_ECALL_TYPE 	2//ecall
#define PP_BCALL_TYPE	1//bcall
#define	PP_ICALL_TYPE	3//icall
/***********ºêº¯Êư***********/

/*******************************************************
description£º struct definitions
*******************************************************/

/*******************************************************
description£º typedef definitions
*******************************************************/
/******enum definitions******/

typedef enum
{
	PP_BCALL = 0,//
    PP_ECALL,//
	PP_ICALL,
	PP_detection,
	PP_XCALL_MAX
} PP_Xcall_INDEX;

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* ÇëÇó:box to tsp */
	uint8_t resp;/* ḮÓ¦:box to tsp */
	uint8_t retrans;/* retransmission */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_xcallSt_t; /*xcall½á¹¹̀å*/

/* application data struct */
/***********************************
			Xcall
***********************************/
typedef struct
{
	int  gpsSt;//gps×´̀¬ 0-Î̃Đ§£»1-ÓĐĐ§
	long gpsTimestamp;//gpsÊ±¼ä´Á
	long latitude;//Î³¶È x 1000000,µ±GPSĐÅºÅÎ̃Đ§Ê±£¬ÖµÎª0
	long longitude;//¾­¶È x 1000000,µ±GPSĐÅºÅÎ̃Đ§Ê±£¬ÖµÎª0
	long altitude;//¸ß¶È£¨m£©
	long heading;//³µÍ··½Ị̈½Ç¶È£¬0ÎªƠư±±·½Ị̈
	long gpsSpeed;//ËÙ¶È x 10£¬µ¥Î»km/h
	long hdop;//Ë®Æ½¾«¶Ẹ̀̉×Ó x 10
}PrvtProt_Rvsposition_t;

typedef struct
{
	long xcallType;//ÀàĐÍ  1-µÀÂ·¾ÈÔ®   2-½ô¼±¾ÈÔ®£¨ecall£©  3-400µç»°½øÏß
	long engineSt;//Æô¶¯×´̀¬£»1-Ï¨»đ£»2-Æô¶¯
	long totalOdoMr;//Àï³̀ÓĐĐ§·¶Î§£º0 - 1000000£¨km£©
	long srsSt;//°²È«ÆøÄ̉×´̀¬ 1- Ơư³££»2 - µ¯³ö
	long updataTime;//Êư¾ƯÊ±¼ä´Á
	long battSOCEx;//³µÁ¾µç³ØÊ£ÓàµçÁ¿£º0-10000£¨0%-100%£©
	PrvtProt_Rvsposition_t gpsPos;//³µÁ¾¾ÈÔ®Î»ÖĂ
}PrvtProt_App_Xcall_t;
/******union definitions*****/

/*******************************************************
description£º variable External declaration
*******************************************************/

/*******************************************************
description£º function External declaration
*******************************************************/
extern void PP_xcall_init(void);
extern int PP_xcall_mainfunction(void *task);
extern void PP_xcall_SetXcallReq(unsigned char req);
#endif 
