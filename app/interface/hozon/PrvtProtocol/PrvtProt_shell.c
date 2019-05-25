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
static int PP_shell_setEcallReq(int argc, const char **argv);
static int PP_shell_setEcallResp(int argc, const char **argv);
static int PP_shell_SetRmtCfgReq(int argc, const char **argv);
static int PP_shell_SetRmtCtrlReq(int argc, const char **argv);
static int PP_shell_SetRmtVSReq(int argc, const char **argv);
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
	shell_cmd_register("HOZON_SetHeartBeatPeriod", PP_shell_setHeartBeatPeriod, "set HOZON PrvtProt HeartBeat Period");
	shell_cmd_register("HOZON_SetSuspend", PP_shell_setSuspend, "set HOZON PrvtProt suspend");
	shell_cmd_register("HOZON_SetEcallReq", PP_shell_setEcallReq, "set HOZON PrvtProt ecall request");
	shell_cmd_register("HOZON_SetEcallResp", PP_shell_setEcallResp, "set HOZON PrvtProt ecall response");
	shell_cmd_register("HOZON_SetRmtCfgReq", PP_shell_SetRmtCfgReq, "set HOZON PrvtProt remote config request");
	shell_cmd_register("HOZON_SetRmtCtrlReq", PP_shell_SetRmtCtrlReq, "set HOZON PrvtProt remote control request");
	shell_cmd_register("HOZON_SetRmtVSReq", PP_shell_SetRmtVSReq, "set HOZON PrvtProt remote VS request");
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
*函数名：PP_shell_setEcallReq

*形  参：设置ecall request


*返回值：void

*描  述：

*备  注：
******************************************************/
static int PP_shell_setEcallReq(int argc, const char **argv)
{
	unsigned int EcallReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <ecall req>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &EcallReq);
	log_o(LOG_HOZON, "EcallReq = %d",EcallReq);
	PP_xcall_SetEcallReq((uint8_t)EcallReq);
	
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

