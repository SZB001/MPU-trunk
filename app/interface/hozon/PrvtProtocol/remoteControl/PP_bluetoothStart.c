
 /******************************************************
文件名：	PP_autodoorCtrl.c

描述：	企业私有协议（浙江合众）
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/

#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>

#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"
#include "tcom_api.h"
#include "ble.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "gb32960_api.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "../PrvtProt_shell.h"
#include "../PrvtProt_EcDc.h"
#include "../PrvtProt.h"
#include "../PrvtProt_cfg.h"
#include "PP_rmtCtrl.h"
#include "../../../gb32960/gb32960.h"
#include "PP_canSend.h"
#include "PPrmtCtrl_cfg.h"
#include "../PrvtProt_SigParse.h"

#include "PP_bluetoothStart.h"

typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_bluetoothStart_pack_t; /**/

typedef struct
{
	PP_bluetoothStart_pack_t 	pack;
	PP_bluetoothStart_t		state;
}__attribute__((packed))  PrvtProt_bluetoothStart_t; 
#if 0
static PrvtProt_bluetoothStart_t PP_bluetoothstart;


void PP_bluetoothstart_init(void)
{
}
int PP_bluetoothstart_mainfunction(void *task)
{
}
void SetPP_bluetoothstart_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
}
int PP_bluetoothstart_start(void)
{
	if((PP_bluetoothstart.state.req == 1)&&(GetPP_rmtCtrl_fotaUpgrade() == 0))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int PP_doorLockCtrl_end(void)
{
	if((door_lock_stage == PP_DOORLOCKCTRL_IDLE) && \
			(PP_rmtdoorCtrl.state.req == 0))
	{
		return 1;
		
	}
	else
	{
		return 0;
		
	}
}

void PP_doorLockCtrl_ClearStatus(void)
{
	PP_rmtdoorCtrl.state.req = 0;

#endif







