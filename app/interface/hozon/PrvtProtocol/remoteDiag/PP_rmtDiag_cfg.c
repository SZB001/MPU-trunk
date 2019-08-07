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
    remote_diag_request(MPU_MID_REMOTE_DIAG, (char *)(&obj), 1);
	/*PP_rmtDiag_queryInform_cb();*/
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
	rmtDiag_NodeFault = get_PP_rmtDiag_NodeFault_t();
}
