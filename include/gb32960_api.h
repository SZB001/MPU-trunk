#ifndef __GB32960_API_H__
#define __GB32960_API_H__

extern int gb_set_addr(const char *url, uint16_t port);
extern int gb_set_vin(const char *vin);
extern int gb_set_datintv(uint16_t period);
extern int gb_set_regintv(uint16_t period);
extern int gb_set_timeout(uint16_t timeout);
extern int gb_init(INIT_PHASE phase);
extern int gb_run(void);

//#if GB32960_SHARE_LINK
extern int gb32960_ServiceMsgSend(char* objDescri,char *Msg,int len);
extern void gb32960_ServiceRxConfig(char objType,char *startChar,void* rx_callback_fn);
//#endif
#endif