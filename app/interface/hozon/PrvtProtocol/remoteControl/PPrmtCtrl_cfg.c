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
 	 读取充电状态:开启/关闭:1--开启；2--关闭
*/
unsigned char PP_rmtCtrl_cfg_chargeSt(void)
{
	return 1;
}
