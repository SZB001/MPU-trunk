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

int PP_get_remote_result(unsigned char obj, PP_rmtDiag_Fault_t * pp_rmtdiag_fault);



#endif //__REMOTE_DIAG_API_H__
