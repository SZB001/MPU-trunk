/******************************************************
�ļ�����	PrvtProt_cfg.c

������	��ҵ˽��Э�飨�㽭���ڣ�
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "init.h"

#include "../PrvtProt_SigParse.h"


#include "gb32960_api.h"
#include "PPrmtCtrl_cfg.h"



/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/

/******************************************************
*��������PP_rmtCtrl_cfg_AuthStatus
*��  �Σ�
*����ֵ��int
*��  ����   ������֤״̬
*��  ע��
******************************************************/
unsigned char PP_rmtCtrl_cfg_AuthStatus(void)
{
	return PrvtProt_SignParse_autheSt();
}
/*
 	 读取车辆状态
*/

unsigned char PP_rmtCtrl_cfg_vehicleState(void)
{
	//return gb_data_vehicleState();
	return 0;
}

/*
 	 读取车门锁状态
*/
unsigned char PP_rmtCtrl_cfg_doorlockSt(void)
{
	return gb_data_doorlockSt();
}


/*
 	 读取尾门状态
*/
unsigned char PP_rmtCtrl_cfg_reardoorSt(void)
{
	return gb_data_reardoorSt();
}

/*
 	 读取电量状态
*/
unsigned char PP_rmtCtrl_cfg_vehicleSOC(void)
{
	//return gb_data_vehicleSOC();
	return 20;
}
/*
	寻车状态
*/
unsigned char PP_rmtCtrl_cfg_findcarSt(void)
{

	return PrvtProt_SignParse_findcarSt();
}

/*
	天窗状态
*/
unsigned char PP_rmtCtrl_cfg_sunroofSt(void)
{
	unsigned char st;
	if(PrvtProt_SignParse_sunroofSt() == 2)
	{
		st = 0;//关闭
	}
	else
	{
		st = 1;//开启
	}

	 return st;
}
/*
	远程启动状态
*/
unsigned char PP_rmtCtrl_cfg_RmtStartSt(void)
{
	return PrvtProt_SignParse_RmtStartSt();
}

/*
	空调启动状态
*/

unsigned char PP_rmtCtrl_cfg_ACOnOffSt(void)
{
	return gb_data_ACOnOffSt();

}

/*
座椅加热状态
*/
unsigned char PP_rmtCtrl_cfg_HeatingSt(uint8_t dt)
{
	if(dt ==0)
	{
		return PrvtProt_SignParse_DrivHeatingSt();
	}
	else
	{
		return PrvtProt_SignParse_PassHeatingSt();
	}
}

/*
 	 读取充电开关状态:1--开启；2--关闭
*/
unsigned char PP_rmtCtrl_cfg_chargeOnOffSt(void)
{
	uint8_t OnOffSt = 0;
	if((gb_data_chargeOnOffSt() == 1) && (PrvtProt_SignParse_chrgAptEnSt() == 0))
	{
		OnOffSt = 1;
	}
	return OnOffSt;
}

/*
 	 读取充电状态:0-未充电；1 -充电中；2-充电完成；3-充电失败
 PP_RMTCTRL_CFG_NOTCHARGE			0//未充电
 PP_RMTCTRL_CFG_CHARGEING			1//充电中
 PP_RMTCTRL_CFG_CHARGEFINISH			2//充电完成
 PP_RMTCTRL_CFG_CHARGEFAIL
*/
unsigned char PP_rmtCtrl_cfg_chargeSt(void)
{
	uint8_t chargeSt = PP_RMTCTRL_CFG_NOTCHARGE;
	uint8_t tmp = 0;

	tmp = gb_data_chargeSt();
	if(tmp == 0)
	{
		chargeSt = PP_RMTCTRL_CFG_NOTCHARGE;
	}
	else if(tmp == 3)
	{
		chargeSt = PP_RMTCTRL_CFG_CHARGEFINISH;
	}
	else if(tmp == 4)
	{
		chargeSt = PP_RMTCTRL_CFG_CHARGEFAIL;
	}
	else if((tmp == 1) || (tmp == 2) || (tmp == 6))
	{
		chargeSt = PP_RMTCTRL_CFG_CHARGEING;
	}
	else
	{}

	return chargeSt;
}

/*
 	 充电枪连接状态:1--连接；0-未连接
*/
unsigned char PP_rmtCtrl_cfg_chargeGunCnctSt(void)
{
	uint8_t chargeGunCnctSt = 0;

	chargeGunCnctSt = gb_data_chargeGunCnctSt();
	if((1 == chargeGunCnctSt) || (2 == chargeGunCnctSt))
	{
		return 1;
	}

	return 0;
}

/*
 	 运动就绪状态:
*/
unsigned char PP_rmtCtrl_cfg_readyLightSt(void)
{
	return PrvtProt_SignParse_readyLightSt();
}

/*
	绂佹鍚姩鐘舵€?
*/

unsigned char PP_rmtCtrl_cfg_cancelEngiSt(void)
{
	 return PrvtProt_SignParse_cancelEngiSt();

}

/*
	鑾峰彇绌鸿皟娓╁害
*/

unsigned char PP_rmtCtrl_cfg_LHTemp(void)
{

	return gb_data_LHTemp();
}

