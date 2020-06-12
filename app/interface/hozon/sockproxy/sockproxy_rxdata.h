/******************************************************
�ļ�����	
������	����tsp�Խ�socket��·�Ľ������Ͽ�����/�����ݴ���
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__SOCK_PROXY_RX_DATA_H
#define		__SOCK_PROXY_RX_DATA_H
/*******************************************************
description�� include the header file
*******************************************************/


/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/


/**********�곣������*********/
#define SP_DATA_LNG  1456U/*���ݶ��������ݳ�*/
#define SP_QUEUE_LNG  20U/*���ݶ��г�*/

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
	SP_GB = 0,//
    SP_PRIV,
	SP_MAX,
} SP_RX_OBJ;

/*****struct definitions*****/
typedef struct
{
	unsigned char  NonEmptyFlg;	/*���ݷǿձ�־*/
	int	  len;/*���ݳ�*/
	unsigned char  data[SP_DATA_LNG];/*����*/
}sockProxyCache_t;/*���ݶ��нṹ��*/

typedef struct
{
	unsigned char  HeadLabel;/*ͷ��ǩ*/
	unsigned char  TialLabel;/*β��ǩ*/
	sockProxyCache_t SPCache[SP_QUEUE_LNG];
}sockProxyObj_t;/*���ն���ṹ��*/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void SockproxyData_Init(void);
extern int WrSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len);
extern int RdSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len);

#endif
