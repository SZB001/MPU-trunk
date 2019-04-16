#ifndef __FOTA_FOTON_H__
#define __FOTA_FOTON_H__


extern int foton_erase(uint32_t base, int size);
extern int foton_check0(void);
extern int foton_check1(void);
extern int foton_check2(void);
extern int foton_security(uint8_t *seed, int *par, uint8_t *key, int keysz);
extern int foton_swver(uint8_t *buff, int size);
extern int foton_hwver(uint8_t *buff, int size);
extern const char* foton_verstr(uint8_t *ver, int size);
extern void foton_update_ecu_info(void);

#endif
