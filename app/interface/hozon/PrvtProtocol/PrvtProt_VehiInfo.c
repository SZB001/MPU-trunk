/******************************************************
�ļ�����	PrvtProt_VehiInfo.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include "init.h"
#include "log.h"
#include <ql_oe.h>
#include "PrvtProt_VehiInfo.h"

/*******************************************************
description�� function declaration
*******************************************************/

/*Global function declaration*/


/*Static function declaration*/


/*******************************************************
description�� global variable definitions
*******************************************************/
//static nw_client_handle_type    h_nw     = 0;
//static QL_MCM_NW_REG_STATUS_INFO_T	t_info;
//static uint64_t	tasktimer;
/*******************************************************
description�� static variable definitions
*******************************************************/

/******************************************************
description�� function code
******************************************************/
/******************************************************
*InitPP_VehiInfo_Parameter

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
void InitPP_VehiInfo_Parameter(void)
{
	//QL_MCM_NW_Client_Init(&h_nw);
	//memset(&t_info, 0, sizeof(QL_MCM_NW_REG_STATUS_INFO_T));
	//tasktimer = 0;
}


/******************************************************
*TskPP_VehiInfo_MainFunction

*��  �Σ�

*����ֵ��

*��  ������������

*��  ע����������5ms
******************************************************/
void TskPP_VehiInfo_MainFunction(void)
{
	//int	ret	= E_QL_OK;
	#if 0
	if((tm_get_time() - tasktimer) >= 5000)
	{
		tasktimer = tm_get_time();
		QL_MCM_NW_GetRegStatus(h_nw, &t_info);

		if(t_info.voice_registration_details_3gpp_valid)
		{
			log_i(LOG_HOZON,"voice_registration_details_3gpp: \
					tech_domain=%d, radio_tech=%d, mcc=%s, mnc=%s, \
					roaming=%d, forbidden=%d, cid=%d, lac=%d, psc=%d, tac=%d\n", 
			t_info.voice_registration_details_3gpp.tech_domain, 
			t_info.voice_registration_details_3gpp.radio_tech,
			t_info.voice_registration_details_3gpp.mcc,
			t_info.voice_registration_details_3gpp.mnc,
			t_info.voice_registration_details_3gpp.roaming,
			t_info.voice_registration_details_3gpp.forbidden,                    
			t_info.voice_registration_details_3gpp.cid,
			t_info.voice_registration_details_3gpp.lac,
			t_info.voice_registration_details_3gpp.psc,
			t_info.voice_registration_details_3gpp.tac);
		}
	}
	#endif
}