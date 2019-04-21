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
#define SOCK_SERVR_TIMEOUT    (1000 * 5)


/***********�꺯��***********/
#define sockproxy_getURL(x)			gb32960_getURL(x)
#define sockproxy_SkipSockCheck() 	(!gb32960_getNetworkSt() || gb32960_getAllowSleepSt())
#define sockproxy_getsuspendSt() 	gb32960_getsuspendSt()

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
    PP_OPEN,
} PP_SOCK_STATE;
/*****struct definitions*****/
typedef struct
{
    /* protocol status */
    int socket;
    char state;//
	char sendbusy;//����æ״̬
	char asynCloseFlg;//�첽�ر�socket��־
	svr_addr_t sock_addr;
}__attribute__ ((packed)) sockproxy_stat_t;


/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/


#endif 
