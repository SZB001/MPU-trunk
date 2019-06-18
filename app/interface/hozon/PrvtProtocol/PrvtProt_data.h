/******************************************************
文件名：	PrvtProt_data.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_DATA_H
#define		_PRVTPROT_DATA_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_MAX_SENDQUEUE		30

#define PP_SENDBUFLNG			1456
/***********宏函数***********/
typedef void (*PP_sendInform_cb)(void* x);//发送通知回调
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
	uint8_t 				msgdata[PP_SENDBUFLNG];
	int						msglen;
	void 					*Inform_cb_para;
	PP_sendInform_cb		SendInform_cb;//
	uint8_t					type;
    list_t 					*list;
    list_t  				link;
}PrvtProt_Send_t; /*结构体*/

/******union definitions*****/


/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PrvtProt_data_init(void);
extern void PrvtProt_data_write(uint8_t *data,int len,PP_sendInform_cb sendInform_cb,void * cb_para);
extern PrvtProt_Send_t *PrvtProt_data_get_pack(void);
extern void PrvtProt_data_put_back(PrvtProt_Send_t *pack);
#endif 
