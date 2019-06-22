/******************************************************
文件名：	sockproxy_txdata.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_TX_DATA_H
#define		_PRVTPROT_TX_DATA_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define SP_MAX_SENDQUEUE		50

#define SP_SENDBUFLNG			1456
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
	uint8_t 				msgdata[SP_SENDBUFLNG];
	int						msglen;
	void 					*Inform_cb_para;
	SP_sendInform_cb		SendInform_cb;//
	uint8_t					type;
    list_t 					*list;
    list_t  				link;
}SP_Send_t; /*结构体*/

/******union definitions*****/


/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void SP_data_init(void);
extern void SP_data_write(uint8_t *data,int len,SP_sendInform_cb sendInform_cb,void * cb_para);
extern SP_Send_t *SP_data_get_pack(void);
extern void SP_data_put_back(SP_Send_t *pack);
#endif 
