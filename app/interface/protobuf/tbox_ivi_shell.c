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

extern void ivi_activestate_response_send( int fd );

extern void ivi_logfile_request_send( int fd);

extern void ivi_chagerappointment_request_send( int fd);

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
int tbox_ivi_logctrl(void)
{
	ivi_logfile_request_send(ivi_clients[0].fd);
	log_o(LOG_HOZON,"shangchuang rizhi success\n");
	return 0;
}
int tbox_ivi_active(void)
{
	ivi_activestate_response_send(ivi_clients[0].fd);
	return 0;
}
int tbox_ivi_chargeappoint(void)
{
	ivi_chagerappointment_request_send(ivi_clients[0].fd);
	return 0;
}
int tbox_ivi_test(void)
{
	int szlen =0;
	int i;
	unsigned char sendbuf[4096] = {0};
	unsigned char buf[2048] = {0};
	Tbox__Net__TopMessage TopMsg;
	Tbox__Net__IhuChargeAppoointmentSts charge;
	tbox__net__top_message__init( &TopMsg );
	tbox__net__ihu_charge_appoointment_sts__init(&charge);
	
	TopMsg.message_type = TBOX__NET__MESSAGETYPE__REQUEST_TBOX_CHARGEAPPOINTMENTSET;
	charge.timestamp = 0;
	charge.hour = 8;
	charge.min = 8;
	charge.id = 8;
	charge.targetpower = 8;
	charge.effectivestate = 1;
	TopMsg.ihu_charge_appoointmentsts = &charge;
	
	szlen = tbox__net__top_message__get_packed_size( &TopMsg );

    tbox__net__top_message__pack(&TopMsg,buf);
    
    memcpy(sendbuf,IVI_PKG_MARKER,IVI_PKG_S_MARKER_SIZE);

    sendbuf[IVI_PKG_S_MARKER_SIZE] = szlen >> 8;
    sendbuf[IVI_PKG_S_MARKER_SIZE + 1] = szlen;

    for( i = 0; i < szlen; i ++ )
    {
        sendbuf[ i + IVI_PKG_S_MARKER_SIZE + 2 ] = buf[i];
    }

    memcpy(( sendbuf + IVI_PKG_S_MARKER_SIZE + szlen + 2),IVI_PKG_ESC,IVI_PKG_E_MARKER_SIZE);
	log_buf_dump(LOG_IVI, "test", sendbuf, 5+7+2+szlen);
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
	shell_cmd_register("hozon_tsp_log_to_hu", tbox_ivi_logctrl, "HU charge CTRL");	
	shell_cmd_register("hozon_tsp_active_to_hu", tbox_ivi_active, "HU charge CTRL");	
	shell_cmd_register("hozon_tsp_charge_to_hu", tbox_ivi_chargeappoint, "TSP CHARGE TO HU");	
	shell_cmd_register("hozon_test", tbox_ivi_test, "hozon_test");	
	shell_cmd_register("setHUPKIenable", tbox_ivi_shell_setHUPKIenable, "setHUPKIenable");	
}

