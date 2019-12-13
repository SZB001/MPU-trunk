/******************************************************
�ļ�����	sockproxy_txdata.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_TX_DATA_H
#define		_PRVTPROT_TX_DATA_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define SP_MAX_SENDQUEUE		256

#define SP_SENDBUFLNG			1456
/***********�꺯��***********/
typedef void (*SP_sendInform_cb)(void* x);//����֪ͨ�ص�
/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
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
}SP_Send_t; /*�ṹ��*/

/******union definitions*****/


/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void SP_data_init(void);
extern void SP_data_write(uint8_t *data,int len,SP_sendInform_cb sendInform_cb,void * cb_para);
extern SP_Send_t *SP_data_get_pack(void);
extern void SP_data_put_back(SP_Send_t *pack);
extern void SP_data_put_send(SP_Send_t *pack);
extern void SP_data_ack_pack(void);
#endif 
