/******************************************************
�ļ�����	PrvtProt_callCenter.c

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


int ecall_flag = 0;  //����ͨ���ı�־
int bcall_flag = 0;
int icall_flag = 0;

extern int assist_get_call_status(void);

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
static PrvtProt_CC_task_t CC_task;

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_CC_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PrvtProt_CC_init(void)
{
	CC_task.callreq = 0;
}

/******************************************************
*��������Get_call_tpye

*��  �Σ�void

*����ֵ��int      0:ECallռ�� 1��BCallռ�� 2��ICall ռ��

*��  ���� ��ȡ����ͨ���ĵ绰����

*��  ע��
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
*��������PrvtProt_CC_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
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
	
	if(action == 1) //���绰
	{
		type =  tbox_ivi_get_call_type();
		switch(type)
		{
			case 0:   //����ecall
			{
				log_o(LOG_HOZON,"status = %d",assist_get_call_status());

				if(assist_get_call_status() != 5) //�Ƿ����
				{
					if( 0 == Get_call_tpye())  //ecall��ͨ��
					{
						log_o(LOG_HOZON,"Ecall dailing");
						tbox_ivi_clear_call_flag();
						return 0;	
					}
					else if( 1 == Get_call_tpye() ) //bcall ��ͨ��
					{
						log_o(LOG_HOZON,"Ecall dailing");
						disconnectcall();
						log_o(LOG_HOZON,"Bcall hang");
					}
					else if (2 == Get_call_tpye() ) //icall ��ͨ��
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
			case 1:  //����bcall
			{
				if(assist_get_call_status() == 4) //�Ƿ����
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
			case 2:  //����icall
			{
				if(assist_get_call_status() == 4) //�Ƿ����
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
*��������PrvtPro_SetEcallResp

*��  �Σ�

*����ֵ��

*��  ��������ecall response

*��  ע��
******************************************************/
void PrvtPro_SetcallCCReq(unsigned char req)
{
	CC_task.callreq = req;
}
