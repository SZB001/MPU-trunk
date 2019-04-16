#ifndef __UDS_REQUEST_H__
#define __UDS_REQUEST_H__
#include "uds_define.h"


uint32_t  uds_negative_response(UDS_T *uds, uint8_t srv_id, uint8_t nrc);

uint32_t  uds_positive_response(UDS_T *uds, uint32_t can_id, uint16_t pdu_dlc, uint8_t *pdu_data);

uint32_t uds_data_request(UDS_T *uds, UDS_TL_ID ms_id, uint32_t can_id, uint8_t *pu8_data,
                          uint16_t pdu_dlc);

#endif
