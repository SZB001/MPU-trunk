/******************************************************
文件名：	PrvtProt_SigParse.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include "init.h"
#include "log.h"
#include "can_api.h"
#include "gps_api.h"
#include "at.h"
#include "PrvtProt_SigParse.h"
/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/


/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static PP_canSign_t PP_canSign;
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
void InitPrvtProt_SignParse_Parameter(void)
{
	memset(&PP_canSign,0,sizeof(PP_canSign_t));
}


/******************************************************
*函数名：PrvtProt_data_parse_surfix
*形  参：
*返回值：
*描  述：读取数据
*备  注：
******************************************************/
int PrvtProt_data_parse_surfix(int sigid, const char *sfx)
{
    uint32_t pptype, ppindex;

    assert(sigid > 0 && sfx != NULL);

    if (2 != sscanf(sfx, "R%1x%3x", &pptype, &ppindex))
    {
        return 0;
    }

    switch (pptype)
    {
		case PP_RMTCTRL_CANSIGN:
		{
			if (ppindex >= PP_MAX_RMTCTRL_CANSIGN_INFO)
            {
                log_e(LOG_HOZON, "rmt ctrl can sign info over %d! ", ppindex);
                break;
            }
			PP_canSign.rmtCtrlSign.info[ppindex] = sigid;
		}
		break;
        default:
            log_o(LOG_HOZON, "unkonwn type %s%x", sfx ,pptype);
       break;
    }

    return 5;
}

/*
 	 寻车状态
  */
uint8_t PrvtProt_SignParse_findcarSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_FINDCAR] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_FINDCAR])->value: 0x0;
	return st;
}

/*
 	 天窗状态
  */
uint8_t PrvtProt_SignParse_sunroofSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_SUNROOFOPEN] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_SUNROOFOPEN])->value: 0x0;
	return st;
}

/*
 	 远程启动状态
  */
uint8_t PrvtProt_SignParse_RmtStartSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_HIGHVOIELEC] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_HIGHVOIELEC])->value: 0x0;
	return st;
}

/*
 	 主座椅加热状态
  */
uint8_t PrvtProt_SignParse_DrivHeatingSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_DRIVHEATING] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_DRIVHEATING])->value: 0x0;
	return st;
}

/*
 	 副座椅加热状态
  */
uint8_t PrvtProt_SignParse_PassHeatingSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_PASSHEATING] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_PASSHEATING])->value: 0x0;
	return st;
}

/*
 	 禁止启动状态
  */
uint8_t PrvtProt_SignParse_cancelEngiSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_ENGIFORBID] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_ENGIFORBID])->value: 0x0;
	return st;
}

/*
 	 认证状态
  */
uint8_t PrvtProt_SignParse_autheSt(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_AUTHEST] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_AUTHEST])->value: 0x0;
	return st;
}

/*
 	 认证失败原因
  */
uint8_t PrvtProt_SignParse_authefailresion(void)
{
	uint8_t st;
	st = PP_canSign.rmtCtrlSign.info[PP_CANSIGN_AUTHEFAILRESION] ?
				 dbc_get_signal_from_id(PP_canSign.rmtCtrlSign.info[PP_CANSIGN_AUTHEFAILRESION])->value: 0x0;
	return st;
}
