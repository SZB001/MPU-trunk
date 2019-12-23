#ifndef __HOZON_PP_API_H__
#define __HOZON_PP_API_H__

#define PP_RMTDIAG_FAILCODE_LEN 5

#define COM_SDCARD_DIR             "/media/sdcard"
#define COM_SDCARD_DIR_PKI         COM_SDCARD_DIR"/pki"
#define COM_SDCARD_DIR_PKI_CIPHER  "/media/sdcard/usrdata/pki/sn_sim_encinfo.txt"
#define COM_SDCARD_DIR_PKI_CERT	   "/media/sdcard/usrdata/pki/userAuth.cer"
#define COM_SDCARD_DIR_PKI_KEY	   "/media/sdcard/usrdata/pki/userAuth.key"
#define COM_SDCARD_DIR_PKI_CSR	   "/media/sdcard/usrdata/pki/userAuth.csr"

#define PP_CERTDL_CIPHER_PATH			"/usrdata/pki/sn_sim_encinfo.txt"

#define PP_CERTDL_CERTPATH			"/usrdata/pki/userAuth.cer"
#define PP_CERTDL_CERTPATH_UPDATE	"/usrdata/pki/update/userAuth.cer"
#define PP_CERTDL_CERTPATH_BKUP		"/usrdata/pki/bkup/userAuth.cer"

#define PP_CERTDL_TWOCERTKEYPATH			"/usrdata/pki/userAuth.key"
#define PP_CERTDL_TWOCERTRKEYPATH_UPDATE	"/usrdata/pki/update/userAuth.key"
#define PP_CERTDL_TWOCERTRKEYPATH_BKUP		"/usrdata/pki/bkup/userAuth.key"

#define PP_CERTDL_TWOCERTCSRPATH			"/usrdata/pki/userAuth.csr"
#define PP_CERTDL_TWOCERTRCSRPATH_UPDATE	"/usrdata/pki/update/userAuth.csr"
#define PP_CERTDL_TWOCERTRCSRPATH_BKUP		"/usrdata/pki/bkup/userAuth.csr"

#define PP_USER_CFG_PATH			"/usrdata/dev/data/usrdata/master/user_regseq.dat"
#define PP_USER_CFG_BKUP_PATH		"/media/sdcard/usrdata/bkup/user_regseq.dat"
#define PP_SYS_CFG_PATH				"/usrapp/current/data/cfg/master/sys_cfg.dat"
#define PP_SYS_CFG_BKUP_PATH		"/media/sdcard/usrdata/bkup/sys_cfg.dat"

#define PP_CERTDL_TBOXCRL			"/usrdata/pem/tbox.crl"//吊销列表

/* diag struct */
typedef struct
{
	uint8_t	 diagcode[PP_RMTDIAG_FAILCODE_LEN];
	uint8_t	 faultCodeType;
	uint8_t	 lowByte;
	uint32_t diagTime;//ʱ���
}PP_rmtDiag_faultcode_t;

typedef struct
{
	uint8_t sueecss;
	uint8_t faultNum;//������
	uint8_t failureType;
	PP_rmtDiag_faultcode_t faultcode[255];
}PP_rmtDiag_Fault_t;

typedef struct
{
	uint8_t tboxFault;
	uint8_t BMSMiss;
	uint8_t MCUMiss;
}PP_rmtDiag_NodeFault_t;

typedef enum
{
    PP_MSG_SOCKET = 1,//MPU_MID_GB32960,
    PP_MSG_BLE,
} pp_MSG_TYPE;

extern void PP_rmtDiag_queryInform_cb(void);

extern int PrvtProt_init(INIT_PHASE phase);
extern int PrvtProt_run(void);

extern void PrvtPro_SetHeartBeatPeriod(unsigned char period);
extern void PrvtPro_Setsuspend(unsigned char suspend);
extern void PrvtPro_SettboxId(char obj,unsigned int tboxid);
extern void PrvtPro_SetEcallReq(unsigned char req);
extern void PrvtPro_SetEcallResp(unsigned char resp);
extern void PP_rmtCfg_SetmcuSw(const char *mcuSw);
extern void PP_rmtCfg_SetmpuSw(const char *mpuSw);
extern void PrvtProt_Settboxsn(const char *tboxsn);
extern void PrvtPro_ShowPara(void);
extern void PP_rmtCtrl_BluetoothCtrlReq(unsigned char obj, unsigned char cmd);
extern void PP_rmtCtrl_HuCtrlReq(unsigned char obj, void *cmdpara);

extern void SetPrvtProt_Awaken(int type);
extern unsigned char GetPrvtProt_Sleep(void);
extern void Setsocketproxy_Awaken(void);
extern char sockproxy_Sleep(void);
extern void getPPrmtDiagCfg_NodeFault(PP_rmtDiag_NodeFault_t *rmtDiag_NodeFault);
extern unsigned char GetPP_ChargeCtrl_appointSt(void);
extern unsigned char GetPP_ChargeCtrl_appointHour(void);
extern unsigned char GetPP_ChargeCtrl_appointMin(void);

extern unsigned char GetPP_CertDL_CertValid(void);
extern void PP_CertDL_CertDLReset(void);
extern unsigned char GetPP_CertDL_CertUpdate(void);
extern unsigned char GetPP_CertDL_allowBDLink(void);
extern void PrvtProt_SaveCfgPara(unsigned char req);
extern void setPrvtProt_sendHeartbeat(void);
extern void SetPP_rmtCtrl_AuthRequest(void);
extern unsigned char GetPP_rmtCtrl_AuthResult(void);
extern int SetPP_rmtCtrl_FOTA_startInform(void);
extern int SetPP_rmtCtrl_FOTA_endInform(void);
extern void PP_rmtCtrl_ShellFotaUpdateReq(unsigned char req);
extern void PP_ChargeCtrl_show(void);
extern void PP_ACCtrl_show(void);
extern void PP_ntp_Init(void);
extern void PP_ntp_run(void);
extern void PP_SetNTPTime(unsigned char ntpreq);
extern void getPP_rmtCfg_tspAddrPort(char* addr,int* port);
extern void getPP_rmtCfg_certAddrPort(char* addr,int* port);
extern unsigned char GetPP_rmtCtrl_fotaUpgrade(void);
extern char getPP_rmtDiag_Idle(void);
extern void getPP_rmtCfg_cfgVersion(char* ver);
extern int PP_rmtCfg_ultoa(unsigned long value, char *string, int radix);
extern int PP_CertDL_getCipher(char* cipher,int* len);
extern uint8_t PP_rmtCfg_getIccid(uint8_t* iccid);
extern uint8_t  GetPP_Wake_Sleep();
extern void PP_FIP_InfoPush_cb(uint8_t st);
extern uint8_t PP_rmtCfg_enable_actived(void);
extern uint8_t PP_rmtCfg_enable_remotecontorl(void);
extern uint8_t PP_rmtCfg_enable_icall(void);
extern uint8_t PP_rmtCfg_enable_bcall(void);
extern uint8_t PP_rmtCfg_enable_ecall(void);
extern uint8_t PP_rmtCfg_enable_dtcEnabled(void);
extern uint8_t PP_rmtCfg_enable_dcEnabled(void);
extern uint8_t PP_rmtCfg_enable_rChargeEnabled(void);
extern uint8_t PP_rmtCfg_enable_svtEnabled(void);
extern uint8_t PP_rmtCfg_enable_vsEnabled(void);
extern uint8_t PP_rmtCfg_enable_btKeyEntryEnabled(void);
extern uint8_t PP_rmtCfg_enable_journeysEnabled(void);
extern uint8_t PP_rmtCfg_enable_online(void);
extern uint8_t PP_rmtCfg_enable_carEmpower(void);
extern uint8_t PP_rmtCfg_enable_carAlarm(void);
extern uint8_t PP_rmtCfg_enable_evtReport(void);
#endif
