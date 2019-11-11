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
#include "PrvtProt_cfg.h"
#include "PrvtProt_remoteConfig.h"
#include "cfg_api.h"


int ecall_flag = 0;  //通话类型标志位
int bcall_flag = 0;
int icall_flag = 0;

extern int assist_get_call_status(void);
extern void ivi_callstate_response_send(int fd  );
extern ivi_client ivi_clients[MAX_IVI_NUM];
extern int audio_basic_ECall(void);
extern int audio_basic_ICall(void);
extern void at_get_call_incoming_telno(char *incoming_num_str);


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

void PrvtProt_CC_init(void)
{
	CC_task.callreq = 0;
}


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

int PrvtProt_CC_Dial_Hang(void)
{
	int type;
	int action;
	int ret;
	unsigned int len;
	unsigned char xcall[32];
	action = tbox_ivi_get_call_action();
	
	if(action == 1) //拨号
	{
		type =  tbox_ivi_get_call_type();
		switch(type)
		{
			case 0:   //ECALL
			{
				if(PP_rmtCfg_enable_ecall() == 1)  //ecall使能
				{
					log_i(LOG_HOZON,"ECALL ENABLE");
					if(assist_get_call_status() != 5) //电话状态非空闲
					{
						if( 0 == Get_call_tpye())  //ecall正在通话中
						{
							log_o(LOG_HOZON,"Ecall dailing");
							tbox_ivi_clear_call_flag();
							return 0;	
						}
						else if( 1 == Get_call_tpye() ) //bcall正在通话中
						{
							log_o(LOG_HOZON,"Bcall hang");
							disconnectcall();
							if(assist_get_call_status() == 5)
							{
								ivi_callstate_response_send(ivi_clients[0].fd);
							}
							else
							{
								return 0;
							}
							tbox_ivi_clear_bcall_flag();
							log_o(LOG_HOZON,"Ecall dailing");
							
						}
						else if (2 == Get_call_tpye() ) //icall 正在通话中
						{
							log_o(LOG_HOZON,"Icall hang");
							disconnectcall();
							if(assist_get_call_status() == 5)
							{
								ivi_callstate_response_send(ivi_clients[0].fd);
							}
							else
							{
								return 0;
							}
							tbox_ivi_clear_icall_flag();
							log_o(LOG_HOZON,"ecall dailing");
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
					audio_basic_ICall();
					if (strlen((char *)xcall) > 0)
			        {
			             makecall((char *)xcall);
						 tbox_ivi_clear_call_flag();
						 PrvtProtCfg_ecallSt(1);  //֪ͨTSP
						 log_o(LOG_HOZON,"Ecall dail");
						 ecall_flag = 1;
			        }
					else
					{
						tbox_ivi_clear_call_flag();
						log_e(LOG_HOZON,"No phone number set");
					}
				}
				else
				{
					tbox_ivi_clear_call_flag();
					log_e(LOG_HOZON,"ECALL NOT ENABLE");
				}
				break;	
			}
			case 1:  //拨打bcall
			{
				if(PP_rmtCfg_enable_bcall() == 1)
				{
					log_i(LOG_HOZON,"BCALL ENABNLE");
					if(assist_get_call_status() != 5) ///电话状态非空闲
					{
						tbox_ivi_clear_bcall_flag();
						return 0;
					}
			        memset(xcall, 0, sizeof(xcall));
			        len = sizeof(xcall);
			        ret = cfg_get_para(CFG_ITEM_BCALL, xcall, &len);

			        if (ret != 0)
			        {
			            log_e(LOG_IVI, "bcall read failed!!!"); 
			        }
					audio_basic_ICall();
					if (strlen((char *)xcall) > 0)
			        {
			             makecall((char *)xcall);
						 PrvtProtCfg_bcallSt(1);
						 tbox_ivi_clear_bcall_flag();
						 bcall_flag = 1;
			        }
					else
					{
						log_e(LOG_HOZON,"No phone number set");
						tbox_ivi_clear_bcall_flag();
					}
				}
				else
				{
					tbox_ivi_clear_bcall_flag();
					log_e(LOG_HOZON,"BCALL NOT ENABLE");
				}
				break;
			}
			case 2:  //拨打icall
			{
				if(PP_rmtCfg_enable_icall() == 1)
				{
					log_i(LOG_HOZON,"ICALL ENABNLE");
					if(assist_get_call_status() != 5) //电话状态非空闲
					{
						tbox_ivi_clear_icall_flag();
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
						 tbox_ivi_clear_icall_flag();
						 icall_flag = 1;
			        }
					else
					{
						tbox_ivi_clear_icall_flag();
						log_e(LOG_HOZON,"No phone number set");
					}
				}
				else
				{
					tbox_ivi_clear_icall_flag();
					log_e(LOG_HOZON,"ICALL NOT ENABLE");
				}
				break;	
			}
			default: break;
		}
	}
	else if(action == 2)  //挂电话
	{
		disconnectcall();
		if(assist_get_call_status() == 5)
		{
			ivi_callstate_response_send(ivi_clients[0].fd);
			tbox_ivi_clear_call_flag();
			ecall_flag = 0;
			bcall_flag = 0;
			icall_flag = 0;
		}
	}
	else
	{
		return 0;
	}
	return 0;
}

 int PrvtProt_CC_Answercall(void)
{
	char number[32] = {0};
	char xcall[32] = {0};
	uint32_t len = 32;
	int ret;
	if(assist_get_call_status() == 1)   //TBOX来电
	{
		at_get_call_incoming_telno(number);//获取来电号码
		log_o(LOG_HOZON,"incoming number %s",number);
		memset(xcall, 0, sizeof(xcall));
		ret = cfg_get_para(CFG_ITEM_ECALL, xcall, &len);
		if (ret != 0)
		{
			log_e(LOG_HOZON, "get ecall error"); 
		}
		if(strcmp(number,xcall) == 0) //ECALL来电
		{
			audio_basic_ICall();
			answercall();//接听电话
			ecall_flag = 1; //ECALLͨ通话标志位置起
			ivi_callstate_response_send(ivi_clients[0].fd);
			return 0;
		}
		
		memset(xcall, 0, sizeof(xcall));
		ret = cfg_get_para(CFG_ITEM_BCALL, xcall, &len);
		if (ret != 0)
		{
			log_e(LOG_HOZON, "get bcall error"); 
		}
		if(strcmp(number,xcall) == 0) //BCALL来电
		{
			bcall_flag = 1; //BCALLͨ通话标志位置起
			return 0;
		}
		
		memset(xcall, 0, sizeof(xcall));
		ret = cfg_get_para(CFG_ITEM_BCALL, xcall, &len);
		if (ret != 0)
		{
			log_e(LOG_HOZON, "get bcall error"); 
		}
		if(strcmp(number,xcall) == 0) //ICALL来电
		{
			icall_flag = 1; //ICALLͨ通话标志位置起
			return 0;
		}
	}
	return 0;
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
	PrvtProt_CC_Dial_Hang();//�������绰���߹ҵ绰
	PrvtProt_CC_Answercall();//�����ӵ绰
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
