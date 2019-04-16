#ifndef __UDS_PROXY_H__
#define __UDS_PROXY_H__
#include "uds_define.h"

/*************init uds config**************/
int uds_proxy_init(void);

/*******set uds transport layer mode*******/
int uds_set_server(void);
int uds_set_client(void);
int uds_set_client_by_app(uint8_t a_u8CanPort, uint32_t a_u32CanIDPhy, uint32_t a_u32CanIDFunc, uint32_t a_u32CanIDRes);

UDS_T * uds_get_client_info(void);
/********uds module timeout handle*********/
void uds_timeout(UDS_TIMER_E timer_id, uint16_t seq);

void  uds_set_timer(UDS_T *uds, UDS_TIMER_E  name, uint8_t switch_value);

/***distribute confirm¡¢indication frame***/
void uds_proxy(uint8_t *msg, uint16_t length);

#endif
