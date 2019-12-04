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
#define SOCKPROXY_SHELL_PROTOCOL  	0//
//#define SOCKPROXY_TEST
/**********�곣������*********/
#define SOCK_SERVR_TIMEOUT    	(1000 * 5)
#define SOCK_TXPAKG_OUTOFTIME    (1000 * 2)

#define SOCK_CHECKCLOSEDTIMEOUT    	(2500)


#define SOCKPROXY_CHECK_CERT				0//检查证书
#define SOCKPROXY_SETUP_SGLINK			1//建立单向链路
#define SOCKPROXY_SETUP_BDLLINK			2//建立双向链路

#define SOCKPROXY_SGLINK_INIT			0//单向链路初始化
#define SOCKPROXY_SGLINK_CREAT			1//单向链路建立连接
#define SOCKPROXY_SGLINK_CLOSE			2//单向链路断开连接

//单向连接初始化返回状态值
#define SOCKPROXY_SG_INIT_SUCCESS			1151//TBOX 单向初始化成功
#define SOCKPROXY_SG_INIT_FAIL				1050//单向认证初始化失败
#define SOCKPROXY_SG_SSL_FAIL				1060//创建ssl链路ssl_ctx_new失败
#define SOCKPROXY_SG_SETCIPHERSUITE_FAIL 	1070//设置客户端状态加密套件失败
#define SOCKPROXY_SG_ROOT_FAIL				1080//TBOX root 证书打开失败
#define SOCKPROXY_SG_ROOTX509_FAIL			1090//TBOX root 证书x509格转失败
#define SOCKPROXY_SG_X509_STORE				1100//TBOX x509_store root证书库构建失败
//#define SOCKPROXY_SG_						1110//TBOX 中间证书打开失败
//1120-TBOX 中间证书证书x509格转失败
//1130-TBOX x509_store 中间证书库构建失败
//1140-证书库初始化失败
//1150-TBOX 客户端证书验证失败
//1160-TBOX 客户端证书验证回调结束

#define SOCKPROXY_SG_ADDR_INIT_SUCCESS		1010//TBOX端口地址配置初始化成功
#define SOCKPROXY_SG_ADDR_INIT_FAIL			1020//TBOX端口地址配置初始化失败

#define SOCKPROXY_SG_CCIC_SUCCESS			1030//TBOX client Certificate information configured successfully
#define SOCKPROXY_SG_CCIC_FAIL				1040//TBOX client Certificate information configured  fail .

#define SOCKPROXY_SG_CONN_SUCCESS			1230//tbox 客户端单向认证连接成功
//1170-ctx 上线文环境创建失败
//1180-Socket  tcp建立失败
//1190-初始化创建服务器地址信息失败
//1200-conn 连接服务器失败
//1210-ssl 证书验证失败
//1220-ssl 建立握手失败

#define SOCKPROXY_BDLLINK_INIT			0//双向链路初始化
#define SOCKPROXY_BDLLINK_CREAT			1//双向链路建立连接
#define SOCKPROXY_BDLLINK_CLOSE			2//双向链路断开连接

/***********�꺯��***********/
#define sockproxy_getURL(x)			gb32960_getURL(x)
#define sockproxy_SkipSockCheck() 	(!gb32960_networkSt())
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
	char asynCloseFlg;//�첽�ر�socket��
	int  asynCloseType;
	uint64_t closewaittime;
	svr_addr_t sock_addr;
	/* rcv */
	char rcvType;//��������
	uint8_t rcvstep;//���տ���
	int rcvlen;//��������֡�ܳ���
	uint8_t rcvbuf[SOCK_PROXY_RCVLEN];//��������֡buf
	long datalen;

	char linkSt;
	char sglinkSt;
	uint64_t waittime;
	char BDLlinkSt;
	char rcvflag;
	char sleepFlag;
	uint64_t sleepwaittime;
	char sleepwaittimeoutcnt;
	uint64_t recalltimer;
	char	 cancelRcvphreadFlag;
	char	sgLinkAddr[33];
	int		sgPort;
	char	BDLLinkAddr[33];
	int		BDLPort;
	uint8_t	pkiEnFlag;//pki使能标志
	uint8_t	dnserrcnt;//
}__attribute__ ((packed)) sockproxy_stat_t;



/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/


#endif 
