/******************************************************
文件名：	PrvtProt_rmtCtrl_data.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMT_CTRL_DATA_H
#define		_PRVTPROT_RMT_CTRL_DATA_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_RMTCTRL_MAX_SENDQUEUE		30

#define PP_RMTCTRL_SENDBUFLNG			1456
/***********宏函数***********/
typedef void (*PP_rmtCtrlsendInform_cb)(void* x);//发送通知回调
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
	uint8_t 				msgdata[PP_RMTCTRL_SENDBUFLNG];
	int						msglen;
	void 					*Inform_cb_para;
	PP_rmtCtrlsendInform_cb	SendInform_cb;//
	uint8_t					type;
    list_t 					*list;
    list_t  				link;
}PrvtProt_RmtCtrlSend_t; /*结构体*/

/******union definitions*****/


/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_rmtCtrl_data_init(void);
extern void PP_rmtCtrl_data_write(uint8_t *data,int len,PP_rmtCtrlsendInform_cb sendInform_cb,void * cb_para);
extern PrvtProt_RmtCtrlSend_t *PP_rmtCtrl_data_get_pack(void);
extern void PP_rmtCtrl_data_put_back(PrvtProt_RmtCtrlSend_t *pack);
#endif 
