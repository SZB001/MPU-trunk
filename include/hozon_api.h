#ifndef __HOZON_API_H__
#define __HOZON_API_H__

extern int hz_set_addr(const char *url, uint16_t port);
extern int hz_set_vin(const char *vin);
extern int hz_set_datintv(uint16_t period);
extern int hz_set_regintv(uint16_t period);
extern int hz_set_timeout(uint16_t timeout);
extern int hz_init(INIT_PHASE phase);
extern int hz_run(void);

extern int PrvtProt_init(INIT_PHASE phase);
extern int PrvtProt_run(void);

#endif
