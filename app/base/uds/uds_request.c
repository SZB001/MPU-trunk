#include <stdlib.h>
#include <string.h>
#include "scom_api.h"
#include "uds_define.h"
#include "uds_request.h"
#include "uds_server.h"
#include "uds_proxy.h"
#include "log.h"
#include "scom_msg_def.h"


/*******************************************************************************************
function:     uds_data_request
description:  request data
input:        UDS_T * uds,
              UDS_TL_ID id,
              uint32_t can_id,
              uint8_t* pdu_data,
              uint16_t pdu_dlc
output:       none
return:       0 indicates success;
              others indicates failed
********************************************************************************************/
uint32_t uds_data_request(UDS_T *uds, UDS_TL_ID msg_id, uint32_t can_id, uint8_t *pdu_data,
                          uint16_t pdu_dlc)
{
    uint32_t pos = 0;
    static unsigned char data[UDS_SERVER_MAX_BYTE / 2];
    
    data[pos++] = msg_id;

    if ((MSG_ID_UDS_SET_S != msg_id) && (MSG_ID_UDS_SET_C != msg_id))
    {
        if (UDS_TYPE_SERVER == uds->mode)
        {
            memcpy(&data[pos], &can_id, sizeof(int));
            pos += sizeof(int);
            memcpy(&data[pos], &pdu_dlc, sizeof(short));
            pos += sizeof(short);
        }
        else if (UDS_TYPE_CLIENT == uds->mode)
        {
            memcpy(&data[pos], &can_id, sizeof(int));
            pos += sizeof(int);
            memcpy(&data[pos], &pdu_dlc, sizeof(short));
            pos += sizeof(short);
        }
    }

    if ((NULL != pdu_data) && (0 != pdu_dlc))
    {
        memcpy(&data[pos], pdu_data, pdu_dlc);
        pos += pdu_dlc;
    }
    
    uds->sid = pdu_data[0];
    uds->can_id_req = can_id;
    
    log_buf_dump(LOG_UDS, "<<<<<<<<<<<<<<<<<<uds send<<<<<<<<<<<<<<<<<<<", data, pos);
    return scom_tl_send_frame(SCOM_TL_CMD_UDS_MSG, SCOM_TL_SINGLE_FRAME, 0, data, pos);
}

/*************************************************************************************
function:     uds_positive_response
description:  request data
input:        UDS_T * uds,
              uint32_t can_id,
              uint8_t* pdu_data,
              uint16_t pdu_dlc
output:       none
return:       0 indicates success;
              others indicates failed
*************************************************************************************/
uint32_t  uds_positive_response(UDS_T *uds, uint32_t can_id, uint16_t pdu_dlc, uint8_t *pdu_data)
{
    uint32_t  ret = 0;

    if (uds->mode == UDS_TYPE_SERVER)
    {
        if ((NULL != pdu_data) && (pdu_dlc <= UDS_SERVER_MAX_BYTE))
        {
            if ((pdu_data[0] != SID_NegativeResponse) && (g_u8suppressPosRspMsgIndicationFlag == 1))
            {
                g_u8suppressPosRspMsgIndicationFlag = 0;
                return 0;
            }
            else if ((g_u32DiagID == uds->can_id_phy) && (pdu_data[0] == SID_NegativeResponse) &&
                     ((pdu_data[2] == NRC_ServiceNotSupported) ||
                      (pdu_data[2] == NRC_SubFuncationNotSupported) ||
                      (pdu_data[2] == NRC_RequestOutOfRange)))
            {
                g_u8suppressPosRspMsgIndicationFlag = 0;
                return 0;
            }
            else
            {
                ret =  uds_data_request(uds, MSG_ID_UDS_REQ, can_id, pdu_data, pdu_dlc);

                if (ret == 0)
                {
                    uds->sid = pdu_data[0];
                    uds->nrc = pdu_data[2];

                    if (NRC_RequestCorrectlyReceivedResponsePending == pdu_data[2])
                    {
                        uds_set_timer(uds, P2EXT_SERVER, 1);
                    }
                }

                g_u8suppressPosRspMsgIndicationFlag = 0;
            }
        }
    }
    else if (uds->mode == UDS_TYPE_CLIENT)
    {
        if ((pdu_data != NULL_PTR) && (pdu_dlc <= UDS_SERVER_MAX_BYTE))
        {

        }
    }

    return ret;
}

/*************************************************************************************
function:     uds_negative_response
description:  request data
input:        UDS_T * uds,
              uint8_t srv_id,
              uint8_t nrc
output:       none
return:       0 indicates success;
              others indicates failed
*************************************************************************************/
uint32_t  uds_negative_response(UDS_T *uds, uint8_t srv_id, uint8_t nrc)
{
    uint8_t  ar_pdu_data[3];

    ar_pdu_data[0] = SID_NegativeResponse ;
    ar_pdu_data[1] = srv_id ;
    ar_pdu_data[2] = nrc ;

    return uds_positive_response(uds, uds->can_id_res, 3, ar_pdu_data);
}


