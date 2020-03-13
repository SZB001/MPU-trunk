/******************************************************
�ļ�����	PP_rmtDiag_cfg.c

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

#include "hozon_PP_api.h"
#include "PP_rmtDiag_cfg.h"
#include "remote_diag_api.h"
#include "mid_def.h"
#include "uds.h"
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
*��������PPrmtDiagCfg_ecallTriggerEvent
*��  �Σ�
*����ֵ��
*��  ������ȡecall����״̬
*��  ע��
******************************************************/
int PPrmtDiagCfg_ecallTriggerEvent(void)
{
	return 0;
}

/******************************************************
*��������setPPrmtDiagCfg_QueryFaultReq
*��  �Σ�
*����ֵ��
*��  ����
*��  ע��
******************************************************/
void setPPrmtDiagCfg_QueryFaultReq(uint8_t obj)
{
    char tsp_diag_msg[2];
    memset(tsp_diag_msg, 0x00, sizeof(tsp_diag_msg));
    tsp_diag_msg[0] = 0x01;
    tsp_diag_msg[1] = obj;
    
    remote_diag_request(MPU_MID_REMOTE_DIAG, tsp_diag_msg, sizeof(tsp_diag_msg));
	/*PP_rmtDiag_queryInform_cb();*/
}


void setPPrmtDiagCfg_ClearDTCReq(uint8_t obj)
{
    char tsp_diag_msg[2];
    memset(tsp_diag_msg, 0x00, sizeof(tsp_diag_msg));
    
    tsp_diag_msg[0] = 0x02;
    tsp_diag_msg[1] = obj;
    
    remote_diag_request(MPU_MID_REMOTE_DIAG, tsp_diag_msg, sizeof(tsp_diag_msg));
	/*PP_rmtDiag_queryInform_cb();*/
}

void setPPrmtDiagCfg_GetDTCTimeReq(uint8_t obj, PP_rmtDiag_faultcode_t * faultcode)
{
    char tsp_diag_msg[8];
    memset(tsp_diag_msg, 0x00, sizeof(tsp_diag_msg));
    
    tsp_diag_msg[0] = 0x03;
    tsp_diag_msg[1] = obj;

    memcpy(&(tsp_diag_msg[2]), faultcode->diagcode, 5);
    tsp_diag_msg[7] = faultcode->lowByte;
    
    remote_diag_request(MPU_MID_REMOTE_DIAG, tsp_diag_msg, sizeof(tsp_diag_msg));
}



/******************************************************
*��������getPPrmtDiagCfg_Faultcode
*��  �Σ�
*����ֵ��
*��  ��
*��  ע��
******************************************************/
void getPPrmtDiagCfg_Faultcode(uint8_t obj,void *rmtDiag_Fault)
{
    #if 0
	uint8_t i;
	PP_rmtDiag_Fault_t *rmtDiag_Fault_ptr = (PP_rmtDiag_Fault_t*)rmtDiag_Fault;
	rmtDiag_Fault_ptr->faultNum = 3;
	for(i = 0;i < rmtDiag_Fault_ptr->faultNum;i++)
	{
		memcpy(rmtDiag_Fault_ptr->faultcode[i].diagcode,"12345",5);
		rmtDiag_Fault_ptr->faultcode[i].faultCodeType  = 1;
		rmtDiag_Fault_ptr->faultcode[i].lowByte = 0;
		rmtDiag_Fault_ptr->faultcode[i].diagTime = 123456789;
	}
	#endif
	PP_get_remote_result(obj,rmtDiag_Fault);
}

/******************************************************
*��������getPPrmtDiagCfg_NodeFault
*��  �Σ�
*����ֵ��
*��  �� 读取节点故障
*��  ע��
******************************************************/
void getPPrmtDiagCfg_NodeFault(PP_rmtDiag_NodeFault_t *rmtDiag_NodeFault)
{
	PP_rmtDiag_NodeFault_t * NodeFault;
	NodeFault = get_PP_rmtDiag_NodeFault_t();
	rmtDiag_NodeFault->BMSMiss = NodeFault->BMSMiss;
	rmtDiag_NodeFault->MCUMiss = NodeFault->MCUMiss;
	rmtDiag_NodeFault->tboxFault = NodeFault->tboxFault;
}

/*
*获取故障码清除结果
*/
unsigned char getPPrmtDiagCfg_clearDTCresult(unsigned char obj,unsigned char *failureType)
{
	int result;

	result = PP_get_remote_clearDTCresult(obj, failureType);
	if(!result)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
