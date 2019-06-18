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
#include "tbox_ivi_api.h"
#include "cfg_api.h"


int ecall_flag = 0;  //正在通话的标志
int bcall_flag = 0;
int icall_flag = 0;

extern int assist_get_call_status(void);

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
*函数名：Get_call_tpye

*形  参：void

*返回值：int      0:ECall占线 1：BCall占线 2：ICall 占线

*描  述： 获取正在通话的电话类型

*备  注：
******************************************************/
int Get_call_tpye(void)
{
	if(ecall_flag == 1)
	{
		return 0;
	}
	else if( bcall_flag == 1)
	{
		return 1;
	}
	else if(icall_flag == 1)
	{
		return 2;
	}
	else
	{
		return 3;
	}
	return 3;
}

/******************************************************
*函数名：PrvtProt_CC_mainfunction

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
int PrvtProt_CC_mainfunction(void *task)
{
#if 0

	char argv[11] = "17783007443";
	if((1 == CC_task.callreq) || (PrvtProt_CC_callreq()))
	{
	    if( strlen(argv) > 32 )
	    {
	    	log_e(LOG_HOZON,"the telephone num too long\r\n");
	        return -1;
	    }

	    makecall(argv);
	    log_e(LOG_HOZON,"begin to make call\r\n");
	    CC_task.callreq = 0;
	}
#endif

	int type;
	int action;
	int ret;
	unsigned int len;
	unsigned char xcall[32];
	action = tbox_ivi_get_call_action();
	
	if(action == 1) //拨电话
	{
		type =  tbox_ivi_get_call_type();
		switch(type)
		{
			case 0:   //拨打ecall
			{
				log_o(LOG_HOZON,"status = %d",assist_get_call_status());

				if(assist_get_call_status() != 5) //是否空闲
				{
					if( 0 == Get_call_tpye())  //ecall在通话
					{
						log_o(LOG_HOZON,"Ecall dailing");
						tbox_ivi_clear_call_flag();
						return 0;	
					}
					else if( 1 == Get_call_tpye() ) //bcall 在通话
					{
						log_o(LOG_HOZON,"Ecall dailing");
						disconnectcall();
						log_o(LOG_HOZON,"Bcall hang");
					}
					else if (2 == Get_call_tpye() ) //icall 在通话
					{
						log_o(LOG_HOZON,"Icall dailing");
						disconnectcall();
						log_o(LOG_HOZON,"Icall hang");
					}
					
				}
		        memset(xcall, 0, sizeof(xcall));
		        len = sizeof(xcall);
		        ret = cfg_get_para(CFG_ITEM_ECALL, xcall, &len);

		        if (ret != 0)
		        {
		            log_e(LOG_IVI, "ecall read failed!!!");
		           
		        }
				log_o(LOG_HOZON,"xcall = %s",xcall);
				if (strlen((char *)xcall) > 0)
		        {
		             makecall((char *)xcall);
					 tbox_ivi_clear_call_flag();
					 log_o(LOG_HOZON,"Ecall dail");
					 ecall_flag = 1;
		        }
				break;	
			}
			case 1:  //拨打bcall
			{
				if(assist_get_call_status() == 4) //是否空闲
				{
					return 0;
				}
		        memset(xcall, 0, sizeof(xcall));
		        len = sizeof(xcall);
		        ret = cfg_get_para(CFG_ITEM_BCALL, xcall, &len);

		        if (ret != 0)
		        {
		            log_e(LOG_IVI, "bcall read failed!!!"); 
		        }
				if (strlen((char *)xcall) > 0)
		        {
		             makecall((char *)xcall);
					 bcall_flag = 1;
		        }
				break;
			}
			case 2:  //拨打icall
			{
				if(assist_get_call_status() == 4) //是否空闲
				{
					return 0;
				}
		        memset(xcall, 0, sizeof(xcall));
		        len = sizeof(xcall);
		        ret = cfg_get_para(CFG_ITEM_BCALL, xcall, &len);
		        if (ret != 0)
		        {
		            log_e(LOG_IVI, "icall read failed!!!"); 
		        }
				if (strlen((char *)xcall) > 0)
		        {
		             makecall((char *)xcall);
					 icall_flag = 1;
		        }	
				break;	
			}
			default: break;
		}
	}
	else if(action == 2)
	{
		disconnectcall();
		tbox_ivi_clear_call_flag();
		ecall_flag = 0;
		bcall_flag = 0;
		icall_flag = 0;
	}
	else
	{
		return 0;
	}
	return 0;
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
