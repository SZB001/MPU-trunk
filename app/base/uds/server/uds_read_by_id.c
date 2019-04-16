#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>
#include "uds_request.h"
#include "uds_server.h"
#include "status_sync.h"
#include "cfg_api.h"
#include "pm_api.h"
#include "uds_diag.h"
#include "bcd.h"
#include "diag.h"
#include "timer.h"
#include "at.h"


void UDS_SRV_ReadDataByID(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    static uint8_t  Ar_u8RePDU_DATA[2000];
    uint32_t i = 0, did = 0, m = 0, len;
    int ret;

    if ((u16PDU_DLC != 3) | ((u16PDU_DLC % 2) == 0))
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    Ar_u8RePDU_DATA[m++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;

    for (i = 1; i < u16PDU_DLC; i += 2)
    {
        did = (p_u8PDU_Data[i] << 8) + p_u8PDU_Data[i + 1];

        Ar_u8RePDU_DATA[m++] = (uint8_t)(did >> 8);
        Ar_u8RePDU_DATA[m++] = (uint8_t)did;

        len = sizeof(Ar_u8RePDU_DATA) - m;
        ret = uds_diag_get_did_value(did, Ar_u8RePDU_DATA + m, &len);

        if (ret != 0)
        {
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
            return;
        }

        m += len;
    }

    uds_positive_response(tUDS, tUDS->can_id_res, m, Ar_u8RePDU_DATA);
}

