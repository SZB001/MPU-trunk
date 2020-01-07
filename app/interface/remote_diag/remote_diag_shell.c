#include "log.h"
#include "shell_api.h"
#include <string.h>
#include <unistd.h>
#include "remote_diag_api.h"
#include "remote_diag.h"
#include <stdlib.h>

extern void setPPrmtDiagCfg_QueryFaultReq(uint8_t obj);
extern void setPPrmtDiagCfg_ClearDTCReq(uint8_t obj);

unsigned int remote_diag_tsp(char * diag_msg, int diag_msg_len)
{
    int ret = 0;
    remote_diag_request(MPU_MID_SHELL, diag_msg, diag_msg_len);

    /* 设置超时条件阻塞, to do*/
    //read(remote_diag_fds[0], &ret, sizeof(unsigned int));

    return ret;
}


static int PP_get_remote_clearDTCresult_test(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: <UDS msg> \r\n");
        return -1;
    }
    unsigned char request_msg[DIAG_REQUEST_LEN];
    memset(request_msg, 0x00, DIAG_REQUEST_LEN);
    
    shellprintf("argv[0]:%s", argv[0]);
    StrToHex(request_msg, (unsigned char *)argv[0], strlen(argv[0])/2);
    shellprintf("request_msg:%x", request_msg[0]);

    unsigned char failureType = 0;
    shellprintf("PP_get_remote_clearDTCresult:%d", PP_get_remote_clearDTCresult(request_msg[0], &failureType));

    return 0;

}

static int PPrmtDiagCfg_QueryFaultReq_test(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: <UDS msg> \r\n");
        return -1;
    }
    unsigned char request_msg[DIAG_REQUEST_LEN];
    memset(request_msg, 0x00, DIAG_REQUEST_LEN);
    
    shellprintf("argv[0]:%s", argv[0]);
    StrToHex(request_msg, (unsigned char *)argv[0], strlen(argv[0])/2);
    shellprintf("request_msg:%x", request_msg[0]);
    setPPrmtDiagCfg_QueryFaultReq(request_msg[0]);

    return 0;
}

static int setPPrmtDiagCfg_ClearDTCReq_test(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: <UDS msg> \r\n");
        return -1;
    }
    unsigned char request_msg[DIAG_REQUEST_LEN];
    memset(request_msg, 0x00, DIAG_REQUEST_LEN);
    
    shellprintf("argv[0]:%s", argv[0]);
    StrToHex(request_msg, (unsigned char *)argv[0], strlen(argv[0])/2);
    shellprintf("request_msg:%x", request_msg[0]);
    setPPrmtDiagCfg_ClearDTCReq(request_msg[0]);

    return 0;
}



static int remote_diag_shell_tbox(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: <ecutype> \r\n");
        return -1;
    }
    
    shellprintf("argv[0]:%s", argv[0]);
    remote_diag_tsp((char *)argv[0], strlen(argv[0]));

    return 0;
}

static int remote_diag_shell_get_result(int argc, const char **argv)
{
    int dtc_num = 0;
    unsigned char request_num = 0;
    if (argc != 1)
    {
        shellprintf(" usage: <UDS msg> \r\n");
        return -1;
    }
    PP_rmtDiag_Fault_t pp_rmtdiag_fault;
    memset(&pp_rmtdiag_fault,0x00, sizeof(PP_rmtDiag_Fault_t));
    
    shellprintf("remote_diag_shell_get_result recv argv[0]:%s", argv[0]);
    StrToHex(&request_num, (unsigned char *)(argv[0]), 1);
    PP_get_remote_result(request_num,&pp_rmtdiag_fault);
    log_o(LOG_REMOTE_DIAG, "pp_rmtdiag_fault.faultNum:%d\n", pp_rmtdiag_fault.faultNum);
    for(dtc_num=0;dtc_num<pp_rmtdiag_fault.faultNum;dtc_num++)
    {
        
        log_buf_dump(LOG_UDS, "remote diag result diagcode:", pp_rmtdiag_fault.faultcode[dtc_num].diagcode,5);
        log_buf_dump(LOG_UDS, "remote diag result lowByte", &(pp_rmtdiag_fault.faultcode[dtc_num].lowByte),1);
        log_o(LOG_REMOTE_DIAG, "cmd status:%d\n", pp_rmtdiag_fault.faultcode[dtc_num].faultCodeType);
    }
    log_o(LOG_REMOTE_DIAG, "pp_rmtdiag_fault.sueecss:%d\n", pp_rmtdiag_fault.sueecss);
    return 0;
}


int remote_diag_shell_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= shell_cmd_register_ex("rdiagtbox", "rdiagtbox", remote_diag_shell_tbox,
                                         "remote diag tbox");
            
            ret |= shell_cmd_register_ex("rdiagret", "rdiagret", remote_diag_shell_get_result,
                                         "remote diag get result");
                                         
            ret |= shell_cmd_register_ex("rdiagPPQF", "rdiagPPQF", PPrmtDiagCfg_QueryFaultReq_test,
                                         "PPrmtDiagCfg_QueryFaultReq_test");

            ret |= shell_cmd_register_ex("rdiagPPCD", "rdiagPPCD", setPPrmtDiagCfg_ClearDTCReq_test,
                             "setPPrmtDiagCfg_ClearDTCReq_test");

            ret |= shell_cmd_register_ex("rdiagPPCDRES", "rdiagPPCDRES", PP_get_remote_clearDTCresult_test,
                          "PP_get_remote_clearDTCresult_test");

            break;
    }

    return ret;
}



