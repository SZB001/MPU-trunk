/******************************************************
�ļ�����	PPrmtCtrl_cfg.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PPRMTCTRL_CFG_H
#define		_PPRMTCTRL_CFG_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_RMTCTRL_CFG_CANSIGWAITTIME		200//can�ź�״̬��ʱ�о��ȴ�ʱ��


/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
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
