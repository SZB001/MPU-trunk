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
#include "PrvtProt_shell.h"
#include "hozon_PP_api.h"

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
	shell_cmd_register("HOZON_SetEcall", PP_shell_setEcallReq, "set HOZON PrvtProt ecall request");
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
	unsigned char period;
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
	PrvtPro_SetHeartBeatPeriod(period);
	
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
	unsigned char suspend;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <suspend>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &suspend);
	log_o(LOG_HOZON, "suspend = %d",suspend);
	PrvtPro_Setsuspend(suspend);
	
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
	unsigned char EcallReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <ecall req>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &EcallReq);
	log_o(LOG_HOZON, "EcallReq = %d",EcallReq);
	PrvtPro_SetEcallReq(EcallReq);
	
    sleep(1);
    return 0;
}

