#ifndef __UDS_H__
#define __UDS_H__
#include "mid_def.h"
#include "init.h"
#include "uds_define.h"
#include "hozon_PP_api.h"

#define  UDS_DIAG_TIMER        TIMER_MAX
#define  UDS_DIAG_INTERVAL     1000

/* errcode definition */
typedef enum UDS_ERROR_CODE
{
    UDS_INVALID_PARA = (MPU_MID_UDS << 16) | 0x01,
    UDS_STATUS_INVALID,
    UDS_PARA_OVERFLOW,
    UDS_CHECK_TABLE_FAILED,
} UDS_ERROR_CODE;

typedef enum
{
    UDS_SCOM_MSG_IND = MPU_MID_UDS,
} UDS_MSG_ID;

typedef enum
{
    UDS_12V_POWER = 12,
	UDS_24V_POWER = 24,
} UDS_POWER;

int uds_init(INIT_PHASE phase);
void uds_hold_client(int en);
int uds_run(void);
int uds_set_client_ex(int port, int pid, int fid, int rid, void *cb);
void uds_clr_client(void);
int uds_client_request(int msg_id, int can_id, char *data, int len);
uint32_t uds_data_request(UDS_T *uds, UDS_TL_ID msg_id, uint32_t can_id, uint8_t *pdu_data,
                          uint16_t pdu_dlc);
PP_rmtDiag_NodeFault_t * get_PP_rmtDiag_NodeFault_t(void);


#endif
