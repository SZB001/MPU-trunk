#ifndef __HOZON_PP_API_H__
#define __HOZON_PP_API_H__


extern int PrvtProt_init(INIT_PHASE phase);
extern int PrvtProt_run(void);

extern void PrvtPro_SetHeartBeatPeriod(unsigned char period);
extern void PrvtPro_Setsuspend(unsigned char suspend);
extern void PrvtPro_SettboxId(unsigned long int tboxid);
extern void PrvtPro_SetEcallReq(unsigned char req);
extern void PrvtPro_SetEcallResp(unsigned char resp);
extern void PP_rmtCfg_SetmcuSw(const char *mcuSw);
extern void PP_rmtCfg_SetmpuSw(const char *mpuSw);
extern void PP_rmtCfg_Seticcid(const char *iccid);
extern void PrvtPro_ShowPara(void);
#endif
