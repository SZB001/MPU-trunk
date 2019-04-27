#ifndef __GB32960_API_H__
#define __GB32960_API_H__

extern int gb_set_addr(const char *url, uint16_t port);
extern int gb_set_vin(const char *vin);
extern int gb_set_datintv(uint16_t period);
extern int gb_set_regintv(uint16_t period);
extern int gb_set_timeout(uint16_t timeout);
extern int gb_init(INIT_PHASE phase);
extern int gb_run(void);


extern int gb32960_getNetworkSt(void);
extern void gb32960_getURL(void* ipaddr);
extern int gb32960_getAllowSleepSt(void);
extern int gb32960_getsuspendSt(void);

#endif