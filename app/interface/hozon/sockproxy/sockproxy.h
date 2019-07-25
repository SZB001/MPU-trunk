/******************************************************
�ļ�����	sockproxy.h
������	����tsp�Խ�socket��·�Ľ������Ͽ�����/�����ݴ���
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__SOCK_PROXY_H
#define		__SOCK_PROXY_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define SOCKPROXY_SHELL_PROTOCOL  0//�ú�˵���Ƿ�������Э��:1-�� ; 0-��

/**********�곣������*********/
#define SOCK_SERVR_TIMEOUT    	(1000 * 5)
#define SOCK_TXPAKG_OUTOFTIME    (1000 * 2)

/***********�꺯��***********/
#define sockproxy_getURL(x)			gb32960_getURL(x)
#define sockproxy_SkipSockCheck() 	(!gb32960_getNetworkSt() )//|| gb32960_getAllowSleepSt())
#define sockproxy_getsuspendSt() 	0//gb32960_getsuspendSt()

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_CLOSED = 0,//
	PP_CLOSE_WAIT,//
	PP_OPEN_WAIT,//
    PP_OPENED,
} PP_SOCK_STATE;

typedef enum
{
	PP_RCV_UNRCV = 0,//���տ���
	PP_RCV_GB,//���չ�������
	PP_RCV_PRIV//����˽��Э������
} PP_SOCK_RCV_TYPE;

#define PP_RCV_IDLE 0
typedef enum
{
	PP_GB_RCV_IDLE = 0,//���տ���
	PP_GB_RCV_SIGN,//������ʼ��
	PP_GB_RCV_CTRL,//������������ֶΣ������־..���ݵ�Ԫ���ܷ�ʽ��
	PP_GB_RCV_DATALEN,//�������ݵ�Ԫ����
	PP_GB_RCV_DATA,//��������
	PP_GB_RCV_CHECKCODE//����У����
} PP_SOCK_GB_RCV_STEP;

typedef enum
{
	PP_PRIV_RCV_IDLE = 0,//���տ���
	PP_PRIV_RCV_SIGN,//������ʼ��
	PP_PRIV_RCV_CTRL,//������������ֶΣ������־..���ݵ�Ԫ���ܷ�ʽ��
	PP_PRIV_RCV_DATA,//��������
} PP_SOCK_PRIV_RCV_STEP;
/*****struct definitions*****/
#define SOCK_PROXY_RCVLEN	1456
typedef struct
{
    /* protocol status */
    int socket;
    char state;//
	//char sendbusy;//����æ״̬
	char asynCloseFlg;//�첽�ر�socket��־
	svr_addr_t sock_addr;
	/* rcv */
	char rcvType;//��������
	uint8_t rcvstep;//���տ���
	int rcvlen;//��������֡�ܳ���
	uint8_t rcvbuf[SOCK_PROXY_RCVLEN];//��������֡buf
	long datalen;
}__attribute__ ((packed)) sockproxy_stat_t;



/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/


#endif 
