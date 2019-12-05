#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netinet/in.h> 
#include <sys/types.h>  
#include <sys/socket.h> 
#include <pthread.h>
#include "shell_api.h"
#include "cfg_api.h"
#include "tbox_ivi_api.h"
#include "tbox_ivi_pb.h"
#include "log.h"
#include "../hozon/PrvtProtocol/remoteControl/PP_rmtCtrl.h"

extern ivi_client ivi_clients[MAX_IVI_NUM];

extern void PP_rmtCtrl_HuCtrlReq(unsigned char obj, void *cmdpara);

extern void ivi_chagerappointment_request_send( int fd,ivi_chargeAppointSt tspchager);

int tbox_ivi_hu_charge_ctrl(int argc, const char **argv)
{
	unsigned int rmtCtrlReqtype;
	unsigned int hour;
	unsigned int min;
	ivi_chargeAppointSt chargectrl;
    if (argc != 3)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCtrlReq <remote ctrl req>\r\n");
        return -1;
    }
	sscanf(argv[0], "%u", &rmtCtrlReqtype);
	sscanf(argv[1], "%u", &hour);
	sscanf(argv[2], "%u", &min);
	log_o(LOG_IVI,"--------------HU chargeCtrl ------------------");

	chargectrl.cmd = rmtCtrlReqtype;
	chargectrl.effectivestate = 1;
	chargectrl.hour = hour;
	chargectrl.min = min;
	chargectrl.id = 0;
	chargectrl.targetpower = 90;
	PP_rmtCtrl_HuCtrlReq(PP_RMTCTRL_CHARGE,(void *)&chargectrl);

	return 0;
}

static int tbox_ivi_shell_setHUPKIenable(int argc, const char **argv)
{  
	unsigned int pkien;
    if (argc != 1)
    {
        shellprintf(" usage:set pki en error\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &pkien);
	cfg_set_para(CFG_ITEM_EN_HUPKI, (unsigned char *)&pkien, 1);
	shellprintf(" set pki ok\r\n");
	sleep(1);
	//system("reboot");
    return 0;
}

void tbox_shell_init(void)
{
	shell_cmd_register("HuChargeCtrl", tbox_ivi_hu_charge_ctrl, "HU charge CTRL");			
	shell_cmd_register("setHUPKIenable", tbox_ivi_shell_setHUPKIenable, "setHUPKIenable");	
}

