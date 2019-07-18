/******************************************************
文件名：	PP_rmtDiag_cfg.c

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
#include "init.h"

#include "hozon_PP_api.h"
#include "PP_rmtDiag_cfg.h"
#include "remote_diag_api.h"
#include "mid_def.h"
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

/******************************************************
description： function code
******************************************************/

/******************************************************
*函数名：PPrmtDiagCfg_ecallTriggerEvent
*形  参：
*返回值：
*描  述：读取ecall触发状态
*备  注：
******************************************************/
int PPrmtDiagCfg_ecallTriggerEvent(void)
{
	return 0;
}

/******************************************************
*函数名：setPPrmtDiagCfg_QueryFaultReq
*形  参：
*返回值：
*描  述：
*备  注：
******************************************************/
void setPPrmtDiagCfg_QueryFaultReq(uint8_t obj)
{
    remote_diag_request(MPU_MID_REMOTE_DIAG, (char *)(&obj), 1);
	/*PP_rmtDiag_queryInform_cb();*/
}

/******************************************************
*函数名：getPPrmtDiagCfg_Faultcode
*形  参：
*返回值：
*描  述
*备  注：
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
