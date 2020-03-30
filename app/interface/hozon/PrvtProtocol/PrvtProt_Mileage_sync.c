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

static uint8_t sync_upgrade_mile = 0;
static uint32_t old_mileage;
static uint32_t new_mileage;
uint8_t data[8] = {0};

extern int scom_dev_openSt(void);

void PP_Mileagesync_init(void)
{
	uint32_t len = 4;
	int res;
	res = cfg_get_user_para(CFG_ITEM_HOZON_MILEAGE,&new_mileage,&len);
	if(res != 0)
	{
		log_e(LOG_HOZON,"Failed to get rom miles.........");
	}
	log_o(LOG_HOZON,"get mileage = %d from tbox",new_mileage);
}


int PP_Mileagesync_mainfunction(void *task)
{	
	static uint8_t w_flag = 0;
	int i;
	
	if(sync_upgrade_mile == 1)
	{
		data[0] = (uint8_t)(new_mileage >> 16);
		data[1] = (uint8_t)(new_mileage >> 8);
		data[2] = (uint8_t)(new_mileage);
		PP_can_send_mileage(data);
		sync_upgrade_mile = 0;
	}
	
	if(1 == dev_get_KL15_signal())  //IGN on
	{
		if(1 == PrvtProt_SignParse_OdomtrUpdtSt())  //里程同步指令
		{
			new_mileage = gb_data_vehicleOdograph();
		}	
		
		if(new_mileage != old_mileage)   //里程同步，里程有变化才同步一次
		{
			for(i = 0;i < 3 ;i++)
			{
				data[0] = (uint8_t)(new_mileage >> 16);
				data[1] = (uint8_t)(new_mileage >> 8);
				data[2] = (uint8_t)(new_mileage);
				PP_can_send_mileage(data);
			}
			old_mileage = new_mileage;
			w_flag = 1;
		}
	}
	else
	{
		if(w_flag == 1)
		{
			log_o(LOG_HOZON,"IGN off write mileage = %d  to ROM",old_mileage);
			cfg_set_user_para(CFG_ITEM_HOZON_MILEAGE,&old_mileage,sizeof(uint32_t));
			w_flag = 0;
		}
	}
	
	return 0;
}
uint32_t Get_Tbox_mileage(void)
{
	return old_mileage;
}
void PP_Mileagesync_setupgradeflag(void)
{
	sync_upgrade_mile = 1;
}

