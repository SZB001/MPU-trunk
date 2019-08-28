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
#include "tbox_ivi_api.h"
#include "../../../../base/uds/server/uds_did.h"

#include "PP_CameraCtrl.h"

typedef struct
{
	PrvtProt_pack_Header_t	Header;
	PrvtProt_DisptrBody_t	DisBody;
}__attribute__((packed))  PP_rmtCameraCtrl_pack_t; /**/

typedef struct
{
	PP_rmtCameraCtrl_pack_t 	pack;
	 PP_rmtCameraCtrlSt_t		state;
}__attribute__((packed))  PrvtProt_rmtCameraCtrl_t; 

static  PrvtProt_rmtCameraCtrl_t PP_rmtCameraCtrl;

void PP_CameraCtrl_init(void)
{
	memset(&PP_rmtCameraCtrl,0,sizeof(PrvtProt_rmtCameraCtrl_t));
	memcpy(PP_rmtCameraCtrl.pack.Header.sign,"**",2);
	PP_rmtCameraCtrl.pack.Header.ver.Byte = 0x30;
	PP_rmtCameraCtrl.pack.Header.commtype.Byte = 0xe1;
	PP_rmtCameraCtrl.pack.Header.opera = 0x02;
	PP_rmtCameraCtrl.pack.Header.tboxid = 27;
	memcpy(PP_rmtCameraCtrl.pack.DisBody.aID,"110",3);
	PP_rmtCameraCtrl.pack.DisBody.eventId = PP_AID_RMTCTRL + PP_MID_RMTCTRL_RESP;
	PP_rmtCameraCtrl.pack.DisBody.appDataProVer = 256;
	PP_rmtCameraCtrl.pack.DisBody.testFlag = 1;	
}



int PP_CameraCtr_mainfunction(void *task)
{
	if(PP_rmtCameraCtrl.state.req == 1)
	{
		ivi_remotediagnos tspInformHU;
		tspInformHU.aid = PP_AID_RMTCTRL;
		tspInformHU.mid = PP_MID_RMTCTRL_RESP;
		tspInformHU.eventid = PP_rmtCameraCtrl.pack.DisBody.eventId;
		tspInformHU.datatype = 2;
		tspInformHU.cameraname = 1;
		tbox_ivi_set_tspInformHU(&tspInformHU);  //通知车机
		
		log_o(LOG_HOZON,"Has notified HU to collect videos and pictures");
		PP_rmtCtrl_Stpara_t rmtCtrl_Stpara;
		rmtCtrl_Stpara.rvcReqStatus = 2;         //执行成功
		rmtCtrl_Stpara.rvcFailureType = 0;
		rmtCtrl_Stpara.reqType =PP_rmtCameraCtrl.state.reqType;
		rmtCtrl_Stpara.eventid = PP_rmtCameraCtrl.pack.DisBody.eventId;
		rmtCtrl_Stpara.Resptype = PP_RMTCTRL_RVCSTATUSRESP;
		PP_rmtCtrl_StInformTsp(&rmtCtrl_Stpara);
		
	}
	PP_rmtCameraCtrl.state.req = 0 ;
	return 0;
}


void SetPP_CameraCtrl_Request(char ctrlstyle,void *appdatarmtCtrl,void *disptrBody)
{
	switch(ctrlstyle)
	{
		case RMTCTRL_TSP:
		{
			PrvtProt_App_rmtCtrl_t *appdatarmtCtrl_ptr = (PrvtProt_App_rmtCtrl_t *)appdatarmtCtrl;
			PrvtProt_DisptrBody_t *  disptrBody_ptr= (PrvtProt_DisptrBody_t *)disptrBody;
			log_i(LOG_HOZON, "remote Camera control req");
			PP_rmtCameraCtrl.state.reqType = appdatarmtCtrl_ptr->CtrlReq.rvcReqType;
			PP_rmtCameraCtrl.state.req = 1;
			PP_rmtCameraCtrl.pack.DisBody.eventId = disptrBody_ptr->eventId;
			PP_rmtCameraCtrl.state.style = RMTCTRL_TSP;	
		}
		break;
		default:
		break;
	}
}

int PP_CameraCtrl_start(void)
{	
	if(PP_rmtCameraCtrl.state.req == 1)
	{
		return 1;
	}
	else
	{
		return 0;
	}	
}

int PP_CameraCtrl_end(void)
{
	if(PP_rmtCameraCtrl.state.req == 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void PP_CameraCtrl_ClearStatus(void)
{
	PP_rmtCameraCtrl.state.req = 0;
}

/************************shell命令测试使用**************************/

void PP_CameraCtrl_SetCtrlReq(unsigned char req,uint16_t reqType)
{
	PP_rmtCameraCtrl.state.reqType = (long)reqType;
	
	PP_rmtCameraCtrl.state.req = 1;
	PP_rmtCameraCtrl.state.style = RMTCTRL_TSP;
#if 0
	char vin[18] = {0};
	char iccid[21] = {0};
	char imei[16] = {0};
	char iccid1[21] = {0};
	char *pt = iccid1; 
	char sw[12] = {0};
	char hw[12] = {0};
	gb32960_getvin(vin);
	log_o(LOG_HOZON,"VIN = %s",vin);
	if(PP_rmtCfg_getIccid((uint8_t *)iccid) == 1)
	{
		pt = iccid;
	}
	else
	{
		pt  = "00000000000000000000";
	}
	log_o(LOG_HOZON,"ICCID = %s",pt);
	at_get_imei(imei);
	log_o(LOG_HOZON,"IMEI = %s",imei);
	strcpy(sw,DID_F1B0_SW_FIXED_VER);
	strcpy(hw,DID_F191_HW_VERSION);
	log_o(LOG_HOZON,"sw = %s",sw);
	log_o(LOG_HOZON,"hw = %s",hw);
#endif
}
/************************shell命令测试使用**************************/



