/******************************************************
文件名：	PPrmtCtrl_cfg.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PPRMTCTRL_CFG_H
#define		_PPRMTCTRL_CFG_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
#define PP_RMTCTRL_CFG_CANSIGWAITTIME		200//can信号状态延时判决等待时间


/***********宏函数***********/


/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern unsigned char PP_rmtCtrl_cfg_AuthStatus(void);
extern unsigned char PP_rmtCtrl_cfg_vehicleState(void);
extern unsigned char PP_rmtCtrl_cfg_doorlockSt(void);
extern unsigned char PP_rmtCtrl_cfg_reardoorSt(void);
extern unsigned char PP_rmtCtrl_cfg_vehicleSOC(void);
extern unsigned char PP_rmtCtrl_cfg_findcarSt(void);
extern unsigned char PP_rmtCtrl_cfg_sunroofSt(void);
extern unsigned char PP_rmtCtrl_cfg_findcarSt(void);
extern unsigned char PP_rmtCtrl_cfg_RmtStartSt(void);
extern unsigned char  PP_rmtCtrl_cfg_ACOnOffSt(void);
extern unsigned char PP_rmtCtrl_cfg_HeatingSt(uint8_t dt);
#endif 
