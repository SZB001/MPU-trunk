/******************************************************
文件名：	PrvtProt_xcall.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_XCALL_H
#define		_PRVTPROT_XCALL_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_XCALL_ACK_WAIT 		0x01//应答成功
#define PP_XCALL_ACK_SUCCESS 	0x02//应答成功


#define	PP_ECALL_TYPE 	2//ecall
#define PP_BCALL_TYPE	1//bcall
#define	PP_ICALL_TYPE	3//icall
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
	PP_BCALL = 0,//
    PP_ECALL,//
	PP_ICALL,
	PP_XCALL_MAX
} PP_Xcall_INDEX;

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* 请求:box to tsp */
	uint8_t resp;/* 响应:box to tsp */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_xcallSt_t; /*xcall结构体*/


/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_xcall_init(void);
extern int PP_xcall_mainfunction(void *task);
extern void PP_xcall_SetEcallReq(unsigned char req);
extern void PP_xcall_SetEcallResp(unsigned char resp);

#endif 
