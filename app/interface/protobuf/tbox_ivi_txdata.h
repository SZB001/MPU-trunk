/******************************************************
文件名：	sockproxy_txdata.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_TX_DATA_H
#define		_PRVTPROT_TX_DATA_H
#include "list.h"
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define HU_MAX_SENDQUEUE		120

#define HU_SENDBUFLNG			2048
/***********宏函数***********/
typedef void (*SP_sendInform_cb)(void* x);//发送通知回调
/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/

/******struct definitions******/
typedef struct
{
	uint8_t 				msgdata[HU_SENDBUFLNG];
	int						msglen;
	void 					*Inform_cb_para;
	SP_sendInform_cb		SendInform_cb;//
	uint8_t					type;
    list_t 					*list;
    list_t  				link;
}HU_Send_t; /*结构体*/

/******union definitions*****/


/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void HU_data_init(void);
extern void HU_data_write(uint8_t *data,int len,SP_sendInform_cb sendInform_cb,void * cb_para);
extern HU_Send_t *HU_data_get_pack(void);
extern void HU_data_put_back(HU_Send_t *pack);
extern void HU_data_put_send(HU_Send_t *pack);
extern void HU_data_ack_pack(void);
#endif 
