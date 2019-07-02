#include <string.h>
#include <stdlib.h>
#include "cfg_api.h"
#include "uds_diag.h"
#include "uds_request.h"
#include "uds_server.h"


void UDS_SRV_WriteDataByID(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[3];
    uint32_t value = 0;
    int ret;

    if (u16PDU_DLC < 3)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    value = (p_u8PDU_Data[1] << 8) + p_u8PDU_Data[2];

    ret = is_uds_diag_set_did_invalue(value, &p_u8PDU_Data[3], u16PDU_DLC - 3);
    
    if (ret != 0)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], ret);
        return;
    }

    ret = uds_diag_set_did_value(value, &p_u8PDU_Data[3], u16PDU_DLC - 3);

    if (ret != 0)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], ret);
        return;
    }

    Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1];
    Ar_u8RePDU_DATA[2] =  p_u8PDU_Data[2];
    uds_positive_response(tUDS, tUDS->can_id_res, 3, Ar_u8RePDU_DATA);
}


