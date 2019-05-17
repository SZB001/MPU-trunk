/******************************************************
文件名：	PrvtProt_remoteConfig.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_REMOTE_CFG_H
#define		_PRVTPROT_REMOTE_CFG_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define RMTCFG_DELAY_TIME 		5500//
//#define PP_XCALL_ACK_WAIT 	0x01//应答成功
//#define PP_XCALL_ACK_SUCCESS 	0x02//应答成功

/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/

typedef enum
{
	PP_RMTCFG_CFG_IDLE = 0,//
	PP_CHECK_CFG_REQ,//检查远程配置
	PP_CHECK_CFG_RESP,//
	PP_GET_CFG_REQ,//
	PP_GET_CFG_RESP,//
	PP_RMTCFG_CFG_END,//
} PP_RMTCFG_STATE_MACHINE;//状态机

typedef enum
{
	PP_RMTCFG_WAIT_IDLE = 0,//
	PP_RMTCFG_CHECK_WAIT_RESP,//
	PP_RMTCFG_GET_WAIT_RESP,//
} PP_RMTCFG_WAIT_STATE;//等待状态

/*****struct definitions*****/
typedef struct
{
	uint8_t req;
	uint8_t CfgSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_rmtCfgSt_t; /*remote config结构体*/

/***********************************
			remote config
***********************************/
typedef struct
{
	uint8_t mcuSw[6];
	uint8_t mpuSw[6];
	uint8_t vehicleVin[18];
	uint8_t iccID[21];
	uint8_t btMacAddr[13];//蓝牙mac地址
	uint8_t configSw[6];
	uint8_t cfgVersion[33];
	uint8_t mcuSwlen;
	uint8_t mpuSwlen;
	uint8_t vehicleVinlen;
	uint8_t iccIDlen;
	uint8_t btMacAddrlen;
	uint8_t configSwlen;
	uint8_t cfgVersionlen;
	uint8_t *mcuSw_ptr;
	uint8_t *mpuSw_ptr;
	uint8_t *vehicleVin_ptr;
	uint8_t *iccID_ptr;
	uint8_t *btMacAddr_ptr;
	uint8_t *configSw_ptr;
	uint8_t *cfgVersion_ptr;
}App_rmtCfg_checkReq_t;
typedef struct
{
	int needUpdate;
	uint8_t cfgVersion[33];
	uint8_t cfgVersionlen;
	uint8_t *cfgVersion_ptr;
}App_rmtCfg_checkResp_t;
typedef struct
{
	uint8_t cfgVersion[33];
	uint8_t cfgVersionlen;
	uint8_t *cfgVersion_ptr;
}App_rmtCfg_getReq_t;
typedef struct
{
	int result;
	uint8_t token[33];
	uint8_t userID[33];
	uint8_t tspAddr[33];
	uint8_t tspUser[17];
	uint8_t tspPass[17];
	uint8_t tspIP[16];
	uint8_t tspSms[32];
	uint8_t tspPort[7];
	uint8_t apn2Address[33];
	uint8_t apn2User[17];
	uint8_t apn2Pass[7];
	int actived;
	int rcEnabled;
	int svtEnabled;
	int vsEnabled;
	int iCallEnabled;
	int bCallEnabled;
	int eCallEnabled;
	int dcEnabled;
	int dtcEnabled;
	int journeysEnabled;
	int onlineInfEnabled;
	int rChargeEnabled;
	int btKeyEntryEnabled;
	uint8_t ecallNO[17];
	uint8_t bcallNO[17];
	uint8_t ccNO[17];
	uint8_t tokenlen;
	uint8_t userIDlen;
	uint8_t tspAddrlen;
	uint8_t tspUserlen;
	uint8_t tspPasslen;
	uint8_t tspIPlen;
	uint8_t tspSmslen;
	uint8_t tspPortlen;
	uint8_t apn2Addresslen;
	uint8_t apn2Userlen;
	uint8_t apn2Passlen;
	uint8_t ecallNOlen;
	uint8_t bcallNOlen;
	uint8_t ccNOlen;
	uint8_t *token_ptr;
	uint8_t *userID_ptr;
	uint8_t *tspAddr_ptr;
	uint8_t *tspUser_ptr;
	uint8_t *tspPass_ptr;
	uint8_t *tspIP_ptr;
	uint8_t *tspSms_ptr;
	uint8_t *tspPort_ptr;
	uint8_t *apn2Address_ptr;
	uint8_t *apn2User_ptr;
	uint8_t *apn2Pass_ptr;
	uint8_t *ecallNO_ptr;
	uint8_t *bcallNO_ptr;
	uint8_t *ccNO_ptr;
	uint8_t ficmConfigValid;
	uint8_t apn1ConfigValid;
	uint8_t apn2ConfigValid;
	uint8_t commonConfigValid;
	uint8_t extendConfigValid;
}App_rmtCfg_getResp_t;
typedef struct
{
	int configSuccess;
	uint8_t mcuSw[5];
	uint8_t mpuSw[5];
	uint8_t configSw[5];
	uint8_t cfgVersion[32];
	uint8_t mcuSwlen;
	uint8_t mpuSwlen;
	uint8_t configSwlen;
	uint8_t cfgVersionlen;
	uint8_t *mcuSw_ptr;
	uint8_t *mpuSw_ptr;
	uint8_t *configSw_ptr;
	uint8_t *cfgVersion_ptr;
}App_rmtCfg_EndCfgReq_t;
typedef struct
{
	int configAccepted;
}App_rmtCfg_CfgconnResp_t;

typedef struct
{
	/* check config request */
	App_rmtCfg_checkReq_t checkCfgReq;

	/* check config response */
	App_rmtCfg_checkResp_t checkCfgResp;

	/* get config req */
	App_rmtCfg_getReq_t getCfgReq;

	/* get config response */
	App_rmtCfg_getResp_t getCfgResp;

	/* end config req */
	App_rmtCfg_EndCfgReq_t EndCfgReq;

	/* config  conn req */
	App_rmtCfg_CfgconnResp_t connCfgResp;
}PrvtProt_App_rmtCfg_t;
/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_rmtCfg_init(void);
extern int PP_rmtCfg_mainfunction(void *task);
extern void PP_rmtCfg_SetCfgReq(unsigned char req);


#endif 
