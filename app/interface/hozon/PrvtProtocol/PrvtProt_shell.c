/******************************************************
文件名：	PrvtProt_Shell.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include "timer.h"
#include "log.h"
#include "shell_api.h"
#include "hozon_PP_api.h"
#include "PrvtProt_callCenter.h"
#include "PrvtProt_xcall.h"
#include "PrvtProt_remoteConfig.h"
//#include "remoteControl/PP_doorLockCtrl.h"
#include "PP_rmtCtrl.h"
#include "PrvtProt_VehiSt.h"
#include "remoteDiag/PrvtProt_rmtDiag.h"
#include "PrvtProt_shell.h"

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
static int PP_shell_setHeartBeatPeriod(int argc, const char **argv);
static int PP_shell_setSuspend(int argc, const char **argv);
static int PP_shell_setXcallReq(int argc, const char **argv);
static int PP_shell_setEcallResp(int argc, const char **argv);
static int PP_shell_SetRmtCfgReq(int argc, const char **argv);
static int PP_shell_SetRmtCtrlReq(int argc, const char **argv);
static int PP_shell_SetRmtVSReq(int argc, const char **argv);
static int PP_shell_SetTboxid(int argc, const char **argv);
static int PP_shell_SetTmcuSw(int argc, const char **argv);
static int PP_shell_SetTmpuSw(int argc, const char **argv);
static int PP_shell_Seticcid(int argc, const char **argv);

static int PP_shell_showpara(int argc, const char **argv);


static int PP_shell_SetdiagReq(int argc, const char **argv);
static int PP_shell_SetTboxSN(int argc, const char **argv);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PrvtProt_shell_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PrvtProt_shell_init(void)
{
	shell_cmd_register("hozon_setHeartBeatPeriod", PP_shell_setHeartBeatPeriod, "set HOZON PrvtProt HeartBeat Period");
	shell_cmd_register("hozon_setSuspend", PP_shell_setSuspend, "set HOZON PrvtProt suspend");
	shell_cmd_register("hozon_setXcallReq", PP_shell_setXcallReq, "set HOZON PrvtProt ecall request");
	shell_cmd_register("hozon_setEcallResp", PP_shell_setEcallResp, "set HOZON PrvtProt ecall response");
	shell_cmd_register("hozon_setRmtCfgReq", PP_shell_SetRmtCfgReq, "set HOZON PrvtProt remote config request");
	shell_cmd_register("hozon_setRmtCtrlReq", PP_shell_SetRmtCtrlReq, "set HOZON PrvtProt remote control request");
	shell_cmd_register("hozon_setRmtVSReq", PP_shell_SetRmtVSReq, "set HOZON PrvtProt remote VS request");

	shell_cmd_register("hozon_settboxid", PP_shell_SetTboxid, "set HOZON tboxid");
	shell_cmd_register("hozon_setmcuSw", PP_shell_SetTmcuSw, "set HOZON mcuSw");
	shell_cmd_register("hozon_setmpuSw", PP_shell_SetTmpuSw, "set HOZON mpuSw");
	shell_cmd_register("hozon_seticcid", PP_shell_Seticcid, "set HOZON iccid");
	shell_cmd_register("hozon_settboxsn", PP_shell_SetTboxSN, "set tbox sn");

	/* show */
	shell_cmd_register("hozon_showpara", PP_shell_showpara, "show HOZON parameter");

	/* diag */
	shell_cmd_register("hozon_setDiagReq", PP_shell_SetdiagReq, "set diag request");
}


/******************************************************
*函数名：PP_shell_setHeartBeatPeriod

*形  参：
argv[0] - 心跳周期 ，单位:秒


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_setHeartBeatPeriod(int argc, const char **argv)
{
	unsigned int period;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetheartbeatPeriod <heartbeat period>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &period);
	log_o(LOG_HOZON, "heartbeat period = %d",period);
	if(period == 0)
	{
		 shellprintf(" usage: heartbeat period invalid\r\n");
		 return -1;
	}	
	PrvtPro_SetHeartBeatPeriod((uint8_t)period);
	
	sleep(1);
    return 0;
}


/******************************************************
*函数名：PP_shell_setSuspend

*形  参：设置暂停


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_setSuspend(int argc, const char **argv)
{
	unsigned int suspend;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <suspend>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &suspend);
	log_o(LOG_HOZON, "suspend = %d",suspend);
	PrvtPro_Setsuspend((uint8_t)suspend);
	
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_setXcallReq

*形  参：设置xcall request


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_setXcallReq(int argc, const char **argv)
{
	unsigned int XcallReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <xcall req>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &XcallReq);
	log_o(LOG_HOZON, "XcallReq = %d",XcallReq);
	PP_xcall_SetXcallReq((uint8_t)XcallReq);
	
    sleep(1);
    return 0;
}


/******************************************************
*函数名：PP_shell_setEcallResp

*形  参：设置ecall response


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_setEcallResp(int argc, const char **argv)
{
	unsigned int EcallResp;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetecallResponse <ecall resp>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &EcallResp);
	log_o(LOG_HOZON, "EcallReq = %d",EcallResp);
	//PP_xcall_SetEcallResp((uint8_t)EcallResp);
	PrvtPro_SetcallCCReq((uint8_t)EcallResp);
    sleep(1);
    return 0;
}


/******************************************************
*函数名：PP_shell_SetRmtCfgReq

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetRmtCfgReq(int argc, const char **argv)
{
	unsigned int rmtCfgReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgReq <remote config req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtCfgReq);
	PP_rmtCfg_SetCfgReq((uint8_t)rmtCfgReq);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetRmtCtrlReq

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetRmtCtrlReq(int argc, const char **argv)
{
	unsigned int rmtCtrlReq;
	unsigned int rmtCtrlReqtype;
    if (argc != 2)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCtrlReq <remote ctrl req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtCtrlReq);
	sscanf(argv[1], "%u", &rmtCtrlReqtype);

	PP_rmtCtrl_SetCtrlReq((uint8_t)rmtCtrlReq,rmtCtrlReqtype);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetRmtVSReq

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetRmtVSReq(int argc, const char **argv)
{
	unsigned int rmtVSReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteVSReq <remote check VS req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtVSReq);

	PP_VS_SetVSReq((uint8_t)rmtVSReq);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetTboxid

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetTboxid(int argc, const char **argv)
{
	uint32_t tboxid;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetTboxid <set tboxid>\r\n");
        return -1;
    }

	sscanf(argv[0], "%d", &tboxid);

	PrvtPro_SettboxId(tboxid);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetTmcuSw

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetTmcuSw(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetmcuSw <set mcuSw>\r\n");
        return -1;
    }

    PP_rmtCfg_SetmcuSw(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetTmpuSw

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetTmpuSw(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetmpuSw <set mpuSw>\r\n");
        return -1;
    }

    PP_rmtCfg_SetmpuSw(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_Seticcid

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_Seticcid(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Seticcid <set iccid>\r\n");
        return -1;
    }

    PP_rmtCfg_Seticcid(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_showremoteCfg

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_showpara(int argc, const char **argv)
{
	PrvtPro_ShowPara();
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetdiagReq

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetdiagReq(int argc, const char **argv)
{
	unsigned int diagReq;
    if (argc != 1)
    {
        shellprintf(" usage: hozon_setDiagReq <set diag request>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &diagReq);
	PP_diag_SetdiagReq((uint8_t)diagReq);
    sleep(1);
    return 0;
}

/******************************************************
*函数名：PP_shell_SetTboxSN

*形  参：设置


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_SetTboxSN(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Settboxsn <set tboxsn>\r\n");
        return -1;
    }

    PrvtProt_Settboxsn(argv[0]);
    sleep(1);
    return 0;
}
