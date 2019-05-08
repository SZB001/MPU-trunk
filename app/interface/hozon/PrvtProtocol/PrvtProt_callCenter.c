/******************************************************
文件名：	PrvtProt_callCenter.c

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
#include "PrvtProt_shell.h"
#include "at_api.h"
#include "PrvtProt_callCenter.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static PrvtProt_CC_task_t CC_task;

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PrvtProt_CC_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PrvtProt_CC_init(void)
{
	CC_task.callreq = 0;
}

/******************************************************
*函数名：PrvtProt_CC_mainfunction

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PrvtProt_CC_mainfunction(void)
{
	char argv[11] = "17783007443";
	if((1 == CC_task.callreq) || (PrvtProt_CC_callreq()))
	{
	    if( strlen(argv) > 32 )
	    {
	    	log_e(LOG_HOZON,"the telephone num too long\r\n");
	        return;
	    }

	    makecall(argv);
	    log_e(LOG_HOZON,"begin to make call\r\n");
	    CC_task.callreq = 0;
	}
}

/******************************************************
*函数名：PrvtPro_SetEcallResp

*形  参：

*返回值：

*描  述：设置ecall response

*备  注：
******************************************************/
void PrvtPro_SetcallCCReq(unsigned char req)
{
	CC_task.callreq = req;
}
