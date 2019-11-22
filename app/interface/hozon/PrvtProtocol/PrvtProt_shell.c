/******************************************************
�ļ�����	PrvtProt_Shell.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include "timer.h"
#include "log.h"
#include "at.h"
#include "shell_api.h"
#include "hozon_PP_api.h"
#include "PrvtProt_callCenter.h"
#include "PrvtProt_xcall.h"
#include "PrvtProt_remoteConfig.h"
#include "PP_rmtCtrl.h"
#include "PrvtProt_VehiSt.h"
#include "remoteDiag/PrvtProt_rmtDiag.h"
#include "PrvtProt_CertDownload.h"
#include "udef_cfg_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_fotaInfoPush.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_shell_setHeartBeatPeriod(int argc, const char **argv);
static int PP_shell_setSuspend(int argc, const char **argv);
static int PP_shell_setXcallReq(int argc, const char **argv);
static int PP_shell_setEcallResp(int argc, const char **argv);
static int PP_shell_SetRmtCfgReq(int argc, const char **argv);
static int PP_shell_SetRmtCtrlReq(int argc, const char **argv);
static int PP_shell_SetRmtVSReq(int argc, const char **argv);
static int PP_shell_SetTboxid(int argc, const char **argv);
static int PP_shell_SetTmcuSw(int argc, const char **argv);
static int PP_shell_SetTmpuSw(int argc, const char **argv);
static int PP_shell_Seticcid(int argc, const char **argv);
static int PP_shell_showpara(int argc, const char **argv);
static int PP_shell_SetdiagReq(int argc, const char **argv);
static int PP_shell_SetTboxSN(int argc, const char **argv);
static int PP_shell_SetCertDLReq(int argc, const char **argv);
static int PP_shell_SetCertDLUpdata(int argc, const char **argv);
static int PP_shell_SetCfgSaveReq(int argc, const char **argv);
static int PP_shell_SetRmtCfgEnable(int argc, const char **argv);
static int PP_shell_SetRmtFotaUpdate(int argc, const char **argv);
static int PP_shell_SetNTPTime(int argc, const char **argv);
static int PP_shell_rmtdiagtest(int argc, const char **argv);
static int PP_shell_SetRmtCfgapn1(int argc, const char **argv);
static int PP_shell_SetRmtCfgficm(int argc, const char **argv);
static int PP_shell_SetTestflag(int argc, const char **argv);
static int PP_shell_showpldata(int argc, const char **argv);
static int PP_shell_deleteCipher(int argc, const char **argv);
static int PP_shell_setFIPReq(int argc, const char **argv);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_shell_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PrvtProt_shell_init(void)
{
	shell_cmd_register("hozon_setHeartBeatPeriod", PP_shell_setHeartBeatPeriod, \
                                            "set HOZON PrvtProt HeartBeat Period");
	shell_cmd_register("hozon_setSuspend", PP_shell_setSuspend, "set HOZON PrvtProt suspend");
	shell_cmd_register("hozon_setXcallReq", PP_shell_setXcallReq, "set HOZON PrvtProt ecall request");
    shell_cmd_register("hozon_setFIPReq", PP_shell_setFIPReq, "set HOZON PrvtProt fota info push request");
	shell_cmd_register("hozon_setEcallResp", PP_shell_setEcallResp, "set HOZON PrvtProt ecall response");
	shell_cmd_register("hozon_setRmtCfgReq", PP_shell_SetRmtCfgReq, \
                                        "set HOZON PrvtProt remote config request");
	shell_cmd_register("hozon_setRmtCtrlReq", PP_shell_SetRmtCtrlReq, \
                                        "set HOZON PrvtProt remote control request");
	shell_cmd_register("hozon_setRmtVSReq", PP_shell_SetRmtVSReq, "set HOZON PrvtProt remote VS request");

	shell_cmd_register("hozon_settboxid", PP_shell_SetTboxid, "set HOZON tboxid");
	shell_cmd_register("hozon_setmcuSw", PP_shell_SetTmcuSw, "set HOZON mcuSw");
	shell_cmd_register("hozon_setmpuSw", PP_shell_SetTmpuSw, "set HOZON mpuSw");
	shell_cmd_register("hozon_seticcid", PP_shell_Seticcid, "set HOZON iccid");
	shell_cmd_register("hozon_settboxsn", PP_shell_SetTboxSN, "set tbox sn");

	/* show */
	shell_cmd_register("hozon_showpara", PP_shell_showpara, "show HOZON parameter");

	/* diag */
	shell_cmd_register("hozon_setDiagReq", PP_shell_SetdiagReq, "set diag request");

	/* cert download */
	shell_cmd_register("hozon_setcertdlReq", PP_shell_SetCertDLReq, "set cert download req");
	shell_cmd_register("hozon_setcertupdataReq", PP_shell_SetCertDLUpdata, "set cert updata req");

    shell_cmd_register("hozon_setsavecfgReq", PP_shell_SetCfgSaveReq, "set save cfg req");

    shell_cmd_register("hozon_setcfgenable", PP_shell_SetRmtCfgEnable, "set save cfg enable");
	shell_cmd_register("hozon_setcfgapn1", PP_shell_SetRmtCfgapn1, "set save cfg apn1");
	shell_cmd_register("hozon_setcfgficm", PP_shell_SetRmtCfgficm, "set save cfg ficm");
    shell_cmd_register("hozon_setfotaupdate", PP_shell_SetRmtFotaUpdate, "set rmt fota update");

    shell_cmd_register("hozon_setntptime", PP_shell_SetNTPTime, "set ntp time");

    shell_cmd_register("hozon_rmtdiagtest", PP_shell_rmtdiagtest, "rmt diag test");
	shell_cmd_register("hozon_testflag", PP_shell_SetTestflag, "rhozon_testflag");

    shell_cmd_register("hozon_pldata", PP_shell_showpldata, "show production line data");
    shell_cmd_register("hozon_delcipher", PP_shell_deleteCipher, "delete cipher");
}


/******************************************************
*��������PP_shell_setHeartBeatPeriod

*��  �Σ�
argv[0] - �������� ����λ:��


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setHeartBeatPeriod(int argc, const char **argv)
{
	unsigned int period;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetheartbeatPeriod <heartbeat period>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &period);
	log_o(LOG_HOZON, "heartbeat period = %d",period);
	if(period == 0)
	{
		 shellprintf(" usage: heartbeat period invalid\r\n");
		 return -1;
	}	
	PrvtPro_SetHeartBeatPeriod((uint8_t)period);
	
	sleep(1);
    return 0;
}


/******************************************************
*��������PP_shell_setSuspend

*��  �Σ�������ͣ


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setSuspend(int argc, const char **argv)
{
	unsigned int suspend;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <suspend>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &suspend);
	log_o(LOG_HOZON, "suspend = %d",suspend);
	PrvtPro_Setsuspend((uint8_t)suspend);
	
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_setXcallReq

*��  �Σ�����xcall request


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setXcallReq(int argc, const char **argv)
{
	unsigned int XcallReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Setsuspend <xcall req>\r\n");
        return -1;
    }
	
	sscanf(argv[0], "%u", &XcallReq);
	log_o(LOG_HOZON, "XcallReq = %d",XcallReq);
	PP_xcall_SetXcallReq((uint8_t)XcallReq);
	
    sleep(1);
    return 0;
}

/******************************************************
*PP_shell_setFIPReq

*


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setFIPReq(int argc, const char **argv)
{
	PP_FIP_shellReq();
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_setEcallResp

*��  �Σ�����ecall response


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_setEcallResp(int argc, const char **argv)
{
	unsigned int EcallResp;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetecallResponse <ecall resp>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &EcallResp);
	log_o(LOG_HOZON, "EcallReq = %d",EcallResp);
	//PP_xcall_SetEcallResp((uint8_t)EcallResp);
	PrvtPro_SetcallCCReq((uint8_t)EcallResp);
    sleep(1);
    return 0;
}


/******************************************************
*��������PP_shell_SetRmtCfgReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetRmtCfgReq(int argc, const char **argv)
{
	unsigned int rmtCfgReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgReq <remote config req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtCfgReq);
	PP_rmtCfg_SetCfgReq((uint8_t)rmtCfgReq);
    sleep(1);
    return 0;
}


/******************************************************
*PP_shell_SetRmtCfgEnable

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetRmtCfgEnable(int argc, const char **argv)
{
    unsigned int obj;
	unsigned int enable;
    if (argc != 2)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgEnable <remote config enable>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &obj);
    sscanf(argv[1], "%u", &enable);
	PP_rmtCfg_setCfgEnable((uint8_t)obj,(uint8_t)enable);
    sleep(1);
    return 0;
}

static int PP_shell_SetRmtCfgapn1(int argc, const char **argv)
{
    unsigned int obj;
	//unsigned int str;
    if (argc != 3)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgEnable <remote config str>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &obj);
    //sscanf(argv[1], "%u", str);
    PP_rmtCfg_setCfgapn1((uint8_t)obj,argv[1],argv[2]);
    sleep(1);
    return 0;
}
static int PP_shell_SetRmtCfgficm(int argc, const char **argv)
{
    unsigned int obj;
	//unsigned int str;
    if (argc != 2)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgEnable <remote config str>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &obj);
    //sscanf(argv[1], "%u", str);
	PP_rmtCfg_setCfgficm((uint8_t)obj,argv[1]);
    sleep(1);
    return 0;
}
/******************************************************
*��������PP_shell_SetRmtCtrlReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetRmtCtrlReq(int argc, const char **argv)
{
	unsigned int rmtCtrlReq;
	unsigned int rmtCtrlReqtype;
    if (argc != 2)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCtrlReq <remote ctrl req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtCtrlReq);
	sscanf(argv[1], "%u", &rmtCtrlReqtype);

	PP_rmtCtrl_SetCtrlReq((uint8_t)rmtCtrlReq,rmtCtrlReqtype);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetRmtVSReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetRmtVSReq(int argc, const char **argv)
{
	unsigned int rmtVSReq;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteVSReq <remote check VS req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtVSReq);

	PP_VS_SetVSReq((uint8_t)rmtVSReq);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetTboxid

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetTboxid(int argc, const char **argv)
{
	unsigned int tboxid;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetTboxid <set tboxid>\r\n");
        return -1;
    }

	sscanf(argv[0], "%d", &tboxid);

	PrvtPro_SettboxId(0,tboxid);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetTmcuSw

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetTmcuSw(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetmcuSw <set mcuSw>\r\n");
        return -1;
    }

    PP_rmtCfg_SetmcuSw(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetTmpuSw

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetTmpuSw(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetmpuSw <set mpuSw>\r\n");
        return -1;
    }

    PP_rmtCfg_SetmpuSw(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_Seticcid

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_Seticcid(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Seticcid <set iccid>\r\n");
        return -1;
    }

    PP_rmtCfg_Seticcid(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_showremoteCfg

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_showpara(int argc, const char **argv)
{
	PrvtPro_ShowPara();
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetdiagReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetdiagReq(int argc, const char **argv)
{
	unsigned int diagReq;
	unsigned int reqtype;
    if (argc != 2)
    {
        shellprintf(" usage: hozon_setDiagReq <set diag request>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &diagReq);
	sscanf(argv[1], "%u", &reqtype);
	PP_diag_SetdiagReq((uint8_t)diagReq,(uint8_t)reqtype);
    sleep(1);
    return 0;
}

/******************************************************
*PP_shell_rmtdiagtest

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_rmtdiagtest(int argc, const char **argv)
{
	unsigned int diagType;
	unsigned int sueecss;
    unsigned int faultNum;
    if (argc != 3)
    {
        shellprintf(" usage: hozon_RmtDiagTest <rmt diag test>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &diagType);
	sscanf(argv[1], "%u", &sueecss);
    sscanf(argv[2], "%u", &faultNum);
	PP_diag_rmtdiagtest((uint8_t)diagType,(uint8_t)sueecss,(uint8_t)faultNum);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetTboxSN

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetTboxSN(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_Settboxsn <set tboxsn>\r\n");
        return -1;
    }

    PrvtProt_Settboxsn(argv[0]);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetCertDLReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetCertDLReq(int argc, const char **argv)
{
	unsigned int CDLReq;
    if (argc != 1)
    {
        shellprintf(" usage: hozon_setcertdlReq <set cert download req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &CDLReq);

	PP_CertDL_SetCertDLReq((uint8_t)CDLReq);
    sleep(1);
    return 0;
}

/******************************************************
*��������PP_shell_SetCertDLUpdata

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetCertDLUpdata(int argc, const char **argv)
{
	unsigned int CupdataReq;
    unsigned int expTime;
    if (argc != 2)
    {
        shellprintf(" usage: hozon_setcertdlReq <set cert download updata req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &CupdataReq);
    sscanf(argv[1], "%d", &expTime);

	PP_CertDL_SetCertDLUpdata((uint8_t)CupdataReq,expTime);
    sleep(1);
    return 0;
}

/******************************************************
*PP_shell_SetCfgSaveReq

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetCfgSaveReq(int argc, const char **argv)
{
	unsigned int saveReq;
    if (argc != 1)
    {
        shellprintf(" usage: hozon_setsavecfgReq <set save cfg req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &saveReq);

	PrvtProt_SaveCfgPara((uint8_t)saveReq);
    sleep(1);
    return 0;
}

/******************************************************
*PP_shell_SetRmtFotaUpdate

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetRmtFotaUpdate(int argc, const char **argv)
{
	unsigned int rmtfotaUpdateReq;
    if (argc != 1)
    {
        shellprintf(" usage: hozon_setfotaupdate <remote fota update req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &rmtfotaUpdateReq);

	PP_rmtCtrl_ShellFotaUpdateReq((uint8_t)rmtfotaUpdateReq);
    sleep(1);
    return 0;
}

/******************************************************
*PP_shell_SetNTPTime

*��  �Σ�����


*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static int PP_shell_SetNTPTime(int argc, const char **argv)
{
	unsigned int ntpreq;
    if (argc != 1)
    {
        shellprintf(" usage: hozon_setntptime <set ntp req>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &ntpreq);

	PP_SetNTPTime((uint8_t)ntpreq);
    sleep(1);
    return 0;
}

static int PP_shell_SetTestflag(int argc, const char **argv)
{
    unsigned int obj;
    if (argc != 1)
    {
        shellprintf(" usage: HOZON_PP_SetRemoteCfgEnable <remote config enable>\r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &obj);
  
	PP_rmtCtrl_settestflag((uint8_t)obj);
    sleep(1);
    return 0;
}

/*
* 获取产线数据，pki相关
*/
static int PP_shell_showpldata(int argc, const char **argv)
{
    unsigned int len;
    char showbuff[32] = {0};

    len = 19;
    cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXSN, showbuff, &len);
    if(showbuff[0]== 0)
    {
        showbuff[0] = '0';
    }
    shellprintf("TBOX SN = %s\r\n", showbuff);

    memset(showbuff, 0, sizeof(showbuff));
    PP_rmtCfg_getIccid((uint8_t*)showbuff);
    if(showbuff[0] == 0)
    {
        showbuff[0] = '0';
    }
    shellprintf("TBOX ICCID = %s\r\n", showbuff);

    //memset(showbuff, 0, sizeof(showbuff));
    //at_get_telno((char *)showbuff);
    shellprintf("TELNO = %s\r\n", "0");

    memset(showbuff, 0, sizeof(showbuff));
    at_get_imei((char *)showbuff);
    shellprintf("IMEI = %s\r\n", showbuff);

    memset(showbuff, 0, sizeof(showbuff));
    at_get_imsi((char *)showbuff);
    shellprintf("IMSI = %s\r\n", showbuff);

    shellprintf("TBOX MODEL = %s\r\n", "EP30");

    memset(showbuff, 0, sizeof(showbuff));
    shellprintf("BLUETOOTH ADDR= %s\r\n", "0");

    sleep(1);
    return 0;
}

/*
* 删密文
*/
static int PP_shell_deleteCipher(int argc, const char **argv)
{
    PP_CertDL_deleteCipher();
    sleep(1);
    return 0;
}
