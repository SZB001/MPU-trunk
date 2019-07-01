/******************************************************
文件名：	PP_rmtDiag_cfg.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PP_RMTDIAG_CFG_H
#define		_PP_RMTDIAG_CFG_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/


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
	PP_DIAG_ALL = 0,//
	PP_DIAG_VCU,//
    PP_DIAG_BMS,//
	PP_DIAG_MCUp,
	PP_DIAG_OBCp,
	PP_DIAG_FLR,
	PP_DIAG_FLC,
	PP_DIAG_APA,
	PP_DIAG_ESCPluse,
	PP_DIAG_EPS,
	PP_DIAG_EHB,
	PP_DIAG_BDCM,
	PP_DIAG_GW,
	PP_DIAG_LSA,
	PP_DIAG_CLM,
	PP_DIAG_PTC,
	PP_DIAG_EACP,
	PP_DIAG_EGSM,
	PP_DIAG_ALM,
	PP_DIAG_WPC,
	PP_DIAG_IHU,
	PP_DIAG_ICU,
	PP_DIAG_IRS,
	PP_DIAG_DVR,
	PP_DIAG_TAP,
	PP_DIAG_MFCP,
	PP_DIAG_TBOX,
	PP_DIAG_ACU,
	PP_DIAG_PLG,
	PP_DIAG_MAXECU,
} PP_RMTDIAG_CFG_TYPE;
/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/

#endif 
