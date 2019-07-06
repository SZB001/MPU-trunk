#ifndef __HOZON_PP_API_H__
#define __HOZON_PP_API_H__

#define PP_RMTDIAG_FAILCODE_LEN 5
/* diag struct */
typedef struct
{
	uint8_t	 diagcode[PP_RMTDIAG_FAILCODE_LEN];
	uint8_t	 faultCodeType;
	uint8_t	 lowByte;
	uint32_t diagTime;// ±º‰¥¡
}PP_rmtDiag_faultcode_t;

typedef struct
{
	uint8_t failNum;//π ’œ ˝
	PP_rmtDiag_faultcode_t faultcode[255];
}PP_rmtDiag_Fault_t;

extern void PP_rmtDiag_queryInform_cb(void);

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
extern void PrvtProt_Settboxsn(const char *tboxsn);
extern void PrvtPro_ShowPara(void);
#endif
