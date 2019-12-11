#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/times.h>
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>
#include "udef_cfg_api.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "XcallReqinfo.h"
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "init.h"
#include "log.h"
#include "list.h"
#include "dev_api.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_EcDc.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "gb32960_api.h"
#include "./remoteControl/PP_canSend.h"
#include "PrvtProt_SigParse.h"
#include "PrvtProt_Mileage_sync.h"

static uint32_t mileage;
static uint64_t lastsendtime;

void PP_Mileagesync_init(void)
{
	uint32_t len = 4;
	int res;
	res = cfg_get_user_para(CFG_ITEM_HOZON_MILEAGE,&mileage,&len);

	if(res != 0)
	{
		log_e(LOG_HOZON,"Failed to get rom miles.........");
	}
	log_o(LOG_HOZON,"get mileage = %d from tbox",mileage);
}


int PP_Mileagesync_mainfunction(void *task)
{
	uint8_t data[8] = {0};
	static uint8_t w_flag ;
	if(1 == dev_get_KL15_signal())  //IGN on
	{
		if(1 == PrvtProt_SignParse_OdomtrUpdtSt())  //里程同步指令
		{
			mileage = gb_data_vehicleOdograph();
		}	
		w_flag = 0;

		data[0] = (uint8_t)(mileage >> 16);
		data[1] = (uint8_t)(mileage >> 8);
		data[2] = (uint8_t)(mileage);
		
		if(tm_get_time() -  lastsendtime > 200)  //200ms同步一次
		{
			PP_can_send_mileage(data);
			lastsendtime = tm_get_time();
		}
	}
	else
	{
		if(w_flag == 0)
		{
			log_o(LOG_HOZON,"IGN off write mileage = %d  to ROM",mileage);
			cfg_set_user_para(CFG_ITEM_HOZON_MILEAGE,&mileage,sizeof(uint32_t));
			w_flag = 1;
		}
	}
	
	return 0;
}


