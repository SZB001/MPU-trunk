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
extern unsigned char gb32960_PowerOffSt(void);

extern uint8_t gb_data_vehicleState(void);
extern long gb_data_vehicleSOC(void);
extern long gb_data_vehicleOdograph(void);
extern long gb_data_vehicleSpeed(void);
extern uint8_t gb_data_doorlockSt(void);
extern uint8_t gb_data_reardoorSt(void);
extern int gb_data_LHTemp(void);
extern uint8_t gb_data_chargeSt(void);
extern uint8_t gb_data_reardoorlockSt(void);
extern uint8_t gb_data_ACMode(void);
extern uint8_t gb_data_ACOnOffSt(void);
extern uint8_t gb_data_chargeOnOffSt(void);
extern uint8_t gb_data_chargeGunCnctSt(void);
extern uint8_t gb_data_BlowerGears(void);
extern uint8_t gb_data_outTemp(void);
extern uint8_t gb_data_InnerTemp(void);
extern uint8_t gb_data_CanbusActiveSt(void);
extern uint8_t gb_data_CrashOutputSt(void);
extern void gb32960_getvin(char* vin);
extern uint8_t gb_data_ACTemperature(void);
extern uint8_t gb_data_TwinFlashLampSt(void);
#endif
