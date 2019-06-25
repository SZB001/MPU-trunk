/******************************************************
文件名：	PrvtProt_signFltr.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include "init.h"
#include "log.h"
#include "PrvtProt_SigParse.h"
#include "PrvtProt_signFltr.h"
/*******************************************************

description： function declaration

*******************************************************/

/*Global function declaration*/



/*Static function declaration*/

static PPsignFltrStruct  SaPPsignFltr_h_Para[PPSIGNFLTR_NUM];/*按键参数结构体*/

/*******************************************************

description： global variable definitions

*******************************************************/
const uint8_t CaPPsignFltr_u_Active[PPSIGNFLTR_NUM] = /*开关有效配置参数，当采集数据跟配置一致则认为外设开启*/
{
	1U/*状态--开启*/
};

/*******************************************************

description： static variable definitions

*******************************************************/
static void PPsignFltr_FltrDeal(void);
static uint8_t GetPPsignFltr_u_RTSt(uint8_t LeFltr_u_Index);
/******************************************************

description： function code

******************************************************/
/******************************************************
*函数名：InitPPsignFltr_Parameter

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
void InitPPsignFltr_Parameter(void)
{
	uint8_t LePPsignFltr_u_Index;
	for(LePPsignFltr_u_Index = 0U;LePPsignFltr_u_Index < PPSIGNFLTR_NUM;LePPsignFltr_u_Index++)
	{
		SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_St = 0;
		SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OnFltrCnt = 0;
		SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OffFltrCnt = 0;
	}
}


/******************************************************
*函数名：TskPPsignFltr_MainFunction

*形  参：

*返回值：

*描  述：主任务函数

*备  注：调用周期5ms
******************************************************/
void TskPPsignFltr_MainFunction(void)
{
	PPsignFltr_FltrDeal();
}

/******************************************************
*函数名：PPsignFltr_FltrDeal

*形  参：

*返回值：

*描  述：

*备  注：调用周期5ms
******************************************************/
static void PPsignFltr_FltrDeal(void)
{
	uint8_t LePPsignFltr_u_Index;
	uint8_t LeFltr_u_St;

	for(LePPsignFltr_u_Index = 0U;LePPsignFltr_u_Index < PPSIGNFLTR_NUM;LePPsignFltr_u_Index++)
	{
		LeFltr_u_St = GetPPsignFltr_u_RTSt(LePPsignFltr_u_Index);
		if(CaPPsignFltr_u_Active[LePPsignFltr_u_Index] == LeFltr_u_St)/*开/关当前实际状态：开启*/
		{
			SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OffFltrCnt =0U;
			SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OnFltrCnt++;/*滤波计时*/
			if(SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OnFltrCnt >= PPSIGNFLTR_FLTRTIME)/*按键短按触发时间门限条件*/
			{
				SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OnFltrCnt =0U;
				SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_St = 1;/*开状态*/
			}
		}
		else/*开/关当前实际状态：关闭*/
		{
			SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OnFltrCnt =0U;
			SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OffFltrCnt++;/*滤波计时*/
			if(SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OffFltrCnt >= PPSIGNFLTR_FLTRTIME)/*按键短按触发时间门限条件*/
			{
				SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_OffFltrCnt =0U;
				SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_St = 0;/*关状态*/
			}
		}
	}
}

/******************************************************
*函数名：GetPPsignFltr_u_PeriSt

*形  参：

*返回值：

*描  述：获取外设状态

*备  注：
******************************************************/
uint8_t GetPPsignFltr_u_PeriSt(uint8_t LePPsignFltr_u_Index)
{
	return SaPPsignFltr_h_Para[LePPsignFltr_u_Index].e_u_St;
}



/******************************************************
*函数名：  GetPPsignFltr_u_RTSt

*形  参：

*返回值：

*描  述：

*备  注：
******************************************************/
static uint8_t GetPPsignFltr_u_RTSt(uint8_t LeFltr_u_Index)
{
	uint8_t Ret = 0U;

	switch(LeFltr_u_Index)
	{
		case bdcmAuthSt:
		{
			Ret = PrvtProt_SignParse_autheSt();
		}
		break;
		default:
		break;
	}

	return Ret;
}

