#ifndef __REMOTE_DIAG_API_H__
#define __REMOTE_DIAG_API_H__
#include "init.h"
#include <stdint.h>
#include "hozon_PP_api.h"



/* remote diag request cmd */
typedef struct
{
    unsigned int token;
    unsigned int diag_msg_len;
    char diag_msg[1024];
} remote_diag_req_cmd_t;


extern int remote_diag_fds[2];

int remote_diag_run(void);
int remote_diag_init(INIT_PHASE phase);

int remote_diag_send_tbox_response(unsigned char *msg, unsigned int len);
int remote_diag_shell_init(INIT_PHASE phase);
int remote_diag_request(unsigned short sender, char * diag_cmd, int diag_msg_len);
int send_msg_to_remote_diag(unsigned short sender, unsigned int msgid,
                            unsigned char *diag_cmd, int diag_msg_len);

int PP_get_remote_result(unsigned char obj, PP_rmtDiag_Fault_t * pp_rmtdiag_fault);

/*
input:
obj:ecu type

return:
const char clear_result_success = 0;
const char clear_result_failed = 1;
*/
int PP_get_remote_clearDTCresult(uint8_t obj, unsigned char *failureType);

#endif //__REMOTE_DIAG_API_H__
