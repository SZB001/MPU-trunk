/******************************************************
�ļ�����	
������
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__PRVT_PROT_QUEUE_H
#define		__PRVT_PROT_QUEUE_H
/*******************************************************
description�� include the header file
*******************************************************/


/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/


/**********�곣������*********/
#define PP_DATA_LNG  1456U/*���ݶ��������ݳ�*/
#define PP_QUEUE_LNG  10U/*���ݶ��г�*/

/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_XCALL = 0,//
	PP_REMOTE_CFG,
	PP_REMOTE_CTRL,
	PP_REMOTE_VS,
	PP_REMOTE_DIAG,
	PP_CERT_DL,
	PP_OTA_INFOPUSH,
	PP_MAX
}PP_RX_OBJ;

/*****struct definitions*****/
typedef struct
{
	unsigned char  NonEmptyFlg;	/*���ݷǿձ�־*/
	int	  len;/*���ݳ�*/
	unsigned char  data[PP_DATA_LNG];/*����*/
}PPCache_t;/*���ݶ��нṹ��*/

typedef struct
{
	unsigned char  HeadLabel;/*ͷ��ǩ*/
	unsigned char  TialLabel;/*β��ǩ*/
	PPCache_t PPCache[PP_QUEUE_LNG];
}PPObj_t;/*���ն���ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_queue_Init(void);
extern int WrPP_queue(unsigned char  obj,unsigned char* data,int len);
extern int RdPP_queue(unsigned char  obj,unsigned char* data,int len);

#endif
