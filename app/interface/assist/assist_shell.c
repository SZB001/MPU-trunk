/****************************************************************
file:         assist.c
description:  the source file of tbox assist shell implementation
date:         2017/7/14
author        liuwei
****************************************************************/
#include <stdio.h>
#include <sys/statfs.h>
#include <dirent.h>
#include <libgen.h>
#include "com_app_def.h"
#include "shell_api.h"
#include "nm_api.h"
#include "assist_shell.h"
#include "cfg_para_api.h"
#include "cfg_api.h"
#include "can_api.h"
#include "dev_api.h"
#include "at.h"
#include "at_api.h"
#include "../support/protocol.h"
#include "dir.h"
#include "file.h"
#include "timer.h"
#include "dsu_api.h"
#include "gps_api.h"
#include "uds_did.h"
#include "udef_cfg_api.h"
#include "fault_sync.h"
#include "diag.h"
#include "ble.h"
#include "uds.h"
#include "hozon_PP_api.h"

extern uint8_t tbox_ivi_get_link_fault(uint64_t *timestamp);

/****************************************************************
function:     app_shell_drcfg
description:  drcfg
input:        unsigned int argc, para count;
              unsigned char **argv, para list
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int app_shell_drcfg(int argc, const char **argv)
{
    unsigned int tmpInt;
    unsigned int len;
    unsigned short canbaud;
    svr_addr_t url;
    CFG_COMM_INTERVAL commpara;
    unsigned char sleepmode;
    unsigned char buff[512];
    unsigned char version[64];
    int ret = 0;
    char vin[18] = {0};
    int i;
    unsigned char otamodein;

    len = sizeof(unsigned int);
    cfg_get_para(CFG_ITEM_DEV_NUM, (unsigned char *)(&tmpInt), &len);
    shellprintf("parts number = %u\r\n", tmpInt);

    len = sizeof(vin);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_VIN, vin, &len);
    if(vin[0] == 0)
    {
        vin[0] = '0';
    }
    shellprintf("vehicle VIN = %s\r\n", vin);

    char tboxsn[19] = {0};
    len = sizeof(tboxsn);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXSN, tboxsn, &len);
    if(tboxsn[0]== 0)
    {
        tboxsn[0] = '0';
    }
    shellprintf("TBOX SN = %s\r\n", tboxsn);

    uint32_t tboxid;
    len = 4;
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXID, &tboxid, &len);
    shellprintf("TBOX ID = %u\r\n",tboxid);

    shellprintf("NET TYPE = ");
    switch (at_get_net_type())
    {
        case 0:
            shellprintf("%s\r\n","2G");
        break;
        case 2:
            shellprintf("%s\r\n","3G");
        break;
        case 7:
            shellprintf("%s\r\n","4G");
        break;
        case 100:  // CDMA
            shellprintf("%s\r\n","3G");
        break;
        default:
            shellprintf("%s\r\n","unknown");
        break;
    }
    shellprintf("4G SIGNAL = %d\r\n", nm_get_signal());
    shellprintf("SIM STATUS = %s\r\n", 1 == dev_diag_get_sim_status() ? "normal" : "fault");
    char iccid[21] = {0};
    PP_rmtCfg_getIccid((uint8_t*)iccid);
    if(iccid[0] == 0)
    {
        iccid[0] = '0';
    }
    shellprintf("TBOX ICCID = %s\r\n", iccid);
    char imeiimsi[16] = {0};
    memset(imeiimsi, 0, sizeof(imeiimsi));
    at_get_imei((char *)imeiimsi);
    if(imeiimsi[0] == 0)
    {
        imeiimsi[0] = '0';
    }
    shellprintf("IMEI = %s\r\n", imeiimsi);

    memset(imeiimsi, 0, sizeof(imeiimsi));
    at_get_imsi((char *)imeiimsi);
    if(imeiimsi[0] == 0)
    {
        imeiimsi[0] = '0';
    }
    shellprintf("IMSI = %s\r\n", imeiimsi);

    char mpusw[11] = {0};
    len = sizeof(mpusw);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_MPUSW, mpusw, &len);
    if(mpusw[0] == 0)
    {
        mpusw[0] = '0';
    }
    shellprintf("mpuSw = %s\r\n", mpusw);

    char mcusw[11] = {0};
    len = sizeof(mcusw);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_MCUSW, mcusw, &len);
    if(mcusw[0] == 0)
    {
        mcusw[0] = '0';
    }
    shellprintf("mcuSw = %s\r\n", mcusw);

    char cfgver[256] = {0};
    getPP_rmtCfg_cfgVersion(cfgver);
    if(cfgver[0] == 0)
    {
        cfgver[0] = '0';
    }
    shellprintf("TSP remote config version = %s\r\n", cfgver);

    len = sizeof(url.url);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_URL, url.url, &len);
    if(url.url[0] == 0)
    {
        url.url[0] = '0';
    }
    shellprintf("TSP ADDR NO PKI = %s\r\n", url.url);
    len = sizeof(url.port);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_PORT, &url.port, &len);
    shellprintf("TSP PORT NO PKI = %u\r\n", url.port);

    char sgLinkAddr[33] = {0};
	int	 sgPort;
    getPP_rmtCfg_certAddrPort(sgLinkAddr,&sgPort);
    if(sgLinkAddr[0] == 0)
    {
        sgLinkAddr[0] = '0';
    }
    shellprintf("SG LINK ADDR PKI = %s\r\n", sgLinkAddr);
    shellprintf("SG LINK PORT PKI = %d\r\n", sgPort);

    char bdlLinkAddr[33] = {0};
	int	 bdlPort;
    getPP_rmtCfg_tspAddrPort(bdlLinkAddr,&bdlPort);
    if(bdlLinkAddr[0] == 0)
    {
        bdlLinkAddr[0] = '0';
    }
    shellprintf("BDL ADDR PKI = %s\r\n", bdlLinkAddr);
    shellprintf("BDL PORT PKI = %d\r\n", bdlPort);

	uint8_t esk[16] = {0};
    PP_identificat_readESK(esk);
    shellprintf("ESK =");
    for(i = 0;i < 16;i++)
    {
        shellprintf("%02X", esk[i]);
        if(i < 15)
        {
            shellprintf(" ");
        }
    }
    shellprintf("\r\n");

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_DBC_PATH, buff, &len);
    if(buff[0] == 0)
    {
        buff[0] = '0';
    }
    shellprintf("DBC PATH CONFIG = %s\r\n", buff);

    char certSn[32] = {0};
    len = sizeof(certSn);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_CERT,certSn,&len);
    if(certSn[0] == 0)
    {
        certSn[0] = '0';
    }
    shellprintf("CERT FILE = %s\r\n", certSn);
    len = 1;
    uint8_t EnFlag;
	ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_CERT_EN,&EnFlag,&len);
    shellprintf("CERT ENABLE = %u\r\n", EnFlag);

    char localAPN[32] = {0};
    len = sizeof(localAPN);
    ret |= cfg_get_para(CFG_ITEM_LOCAL_APN,localAPN,&len);
    if(localAPN[0] == 0)
    {
        localAPN[0] = '0';
    }
    shellprintf("LOCAL APN = %s\r\n", localAPN);

    char wanAPN[32] = {0};
    len = sizeof(wanAPN);
    ret |= cfg_get_para(CFG_ITEM_WAN_APN,wanAPN,&len);
    if(wanAPN[0] == 0)
    {
        wanAPN[0] = '0';
    }
    shellprintf("WAN APN = %s\r\n", wanAPN);

    uint64_t timestamp;
    shellprintf("HU LINK STATUS = %s\r\n", tbox_ivi_get_link_fault(NULL)?"fault":"normal");
    shellprintf("PUBLIC STATUS = %s\r\n", PP_netstatus_pubilcfaultsts(&timestamp)?"fault":"normal");

    len = 1;
	ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,&EnFlag,&len);
    shellprintf("ONE KEY START OF BLE ENABLE = %u\r\n", EnFlag);

    if (2 == flt_get_by_id(WIFI))
    {
        shellprintf(" WIFI STATUS = %s\r\n","fault");
    }
    else
    {
        shellprintf(" WIFI STATUS = %s\r\n",  1 == at_get_wifi_status() ? "opened" : "closed");
    }

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_WIFI_SET,&EnFlag,&len);
    shellprintf("WIFI ENABLE = %u\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_DCOM_SET,&EnFlag,&len);
    shellprintf("DCOM ENABLE = %u\r\n", EnFlag);

    char blename[50] = {0};
    len = sizeof(blename);
    ret |= cfg_get_para(CFG_ITEM_BLE_NAME,blename,&len);
    if(blename[0] == 0)
    {
        blename[0] = '0';
    }
    shellprintf("BLE NAME = %s\r\n", blename);

    char bleaddr[32] = {0};
    BleGetMac((uint8_t*)bleaddr);
    shellprintf("BLE ADDR =");
    for(i = 0;i < 6;i++)
    {
        shellprintf("%02X", bleaddr[i]);
        if(i < 5)
        {
            shellprintf(":");
        }
    }
    shellprintf("\r\n");

    EnFlag = get_ble_ctl();
    shellprintf("BLE ENABLE = %u\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_PKI,&EnFlag,&len);
    shellprintf("TSP PKI ENABLE = %u\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_HUPKI,&EnFlag,&len);
    shellprintf("HU PKI ENABLE = %u\r\n", EnFlag);

	
	len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_CANFILE,&EnFlag,&len);
	shellprintf("CANFILE ENABLE = %u\r\n", EnFlag);
	
    len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_OTAMODEIN,&otamodein,&len);
    shellprintf("OTA Mode in ENABLE = %u\r\n", otamodein);

    EnFlag = PP_rmtCfg_enable_actived();
    shellprintf("ACTIVED ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_remotecontorl();
    shellprintf("RMT CTRL ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_icall();
    shellprintf("ICALL ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_bcall();
    shellprintf("BCALL ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_ecall();
    shellprintf("ECALL ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_dtcEnabled();
    shellprintf("RMT DIAG ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_dcEnabled();
    shellprintf("DATA COLLECT ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_rChargeEnabled();
    shellprintf("RMT CHARGE ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_svtEnabled();
    shellprintf("ANTITHEFT ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_vsEnabled();
    shellprintf("VEHI STATUS ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_btKeyEntryEnabled();
    shellprintf("BLE KEY ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_journeysEnabled();
    shellprintf("JOURNEY ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_online();
    shellprintf("ONLINE ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_carEmpower();
    shellprintf("CAR EMPOWER ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_carAlarm();
    shellprintf("CAR ALARM ENABLE = %u\r\n", EnFlag);

    EnFlag = PP_rmtCfg_enable_evtReport();
    shellprintf("EVT REPORT ENABLE = %u\r\n", EnFlag);

    len = sizeof(commpara.data_val);
    ret |= cfg_get_para(CFG_ITEM_FOTON_INTERVAL, &commpara.data_val, (uint32_t *)&len);
    shellprintf("Data reporting interval = %ds\r\n", commpara.data_val);
    len = sizeof(commpara.hb_val);
    ret |= cfg_get_para(CFG_ITEM_FOTON_INTERVAL, &commpara.hb_val, (uint32_t *)&len);
    shellprintf("heartbeat interval = %ds\r\n", commpara.hb_val);

    len = sizeof(sleepmode);
    cfg_get_para(CFG_ITEM_SLEEP_MODE, &sleepmode, &len);
    shellprintf("SLEEP MODE = ");
    switch (sleepmode)
    {
        case 0:
            shellprintf("RUNNING ONLY\r\n");
            break;

        case 1:
            shellprintf("LISTEN ONLY\r\n");
            break;

        case 2:
            shellprintf("SLEEP ONLY\r\n");
            break;

        case 3:
            shellprintf("AUTO ONLY\r\n");
            break;

        default:
            shellprintf("Unknown\r\n");
            break;
    }

    shellprintf("GPS STATUS = ");
    switch (gps_get_fix_status())
    {
        case GPS_UNCONNECTED:
            shellprintf("unconnected\r\n");
            break;

        case GPS_UNFIX:
            shellprintf("unfix\r\n");
            break;

        case GPS_FIX:
            shellprintf("fix\r\n");
            break;

        case GPS_ERROR:
            shellprintf("fault\r\n");
            break;

        default:
            shellprintf("unknow Err\r\n");
            break;
    }

    shellprintf("EMMC STATUS = ");
    switch (dev_diag_get_emmc_status())
    {
        case DIAG_EMMC_OK:
            shellprintf("eMMC OK\r\n");
            struct statfs diskInfo;
            statfs(COM_SDCARD_DIR, &diskInfo);

            unsigned long long blocksize = diskInfo.f_bsize;                    
            unsigned long long totalsize = blocksize *
                                           diskInfo.f_blocks;                   
            unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   

            unsigned int total_size_mb = totalsize >> 20;
            unsigned int free_size_mb  = availableDisk >> 20;

            shellprintf(" eMMC total size = %u MB\r\n", total_size_mb);
            shellprintf(" eMMC free size = %u MB\r\n", free_size_mb);
			
            break;

        case DIAG_EMMC_FULL:
            shellprintf("eMMC full\r\n");
            break;

        case DIAG_EMMC_UMOUNT:
            shellprintf("eMMC umount\r\n");
            break;

        case DIAG_EMMC_NOT_EXIST:
            shellprintf("eMMC no exist\r\n");
            break;

        case DIAG_EMMC_NOT_FORMAT:
            shellprintf("eMMC no format\r\n");
            break;

        case DIAG_EMMC_UMOUNT_POINT_NOT_EXIST:
            shellprintf("eMMC mount point is not exist\r\n");
            break;

        case DIAG_EMMC_FORMATTING:
            shellprintf("eMMC is formatting\r\n");
            break;

        default:
            shellprintf("eMMC unknow Err\r\n");
            break;
    }

    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_0, &canbaud, &len);
    shellprintf("HCAN DEFAULT RATE = %dK\r\n", canbaud);
    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_1, &canbaud, &len);
    shellprintf("MCAN DEFAULT RATE = %dK\r\n", canbaud);
    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_2, &canbaud, &len);
    shellprintf("LCAN DEFAULT RATE = %dK\r\n", canbaud);

    unsigned char xcall[32];
    len = 32;
    memset(xcall, 0, sizeof(xcall));
    cfg_get_para(CFG_ITEM_ICALL, (unsigned char *)xcall, &len);
    if(xcall[0] == 0)
    {
        xcall[0] = '0';
    }
    shellprintf("ICALL = %s\r\n", xcall);

    len = 32;
    memset(xcall, 0, sizeof(xcall));
    cfg_get_para(CFG_ITEM_BCALL, (unsigned char *)xcall, &len);
    if(xcall[0] == 0)
    {
        xcall[0] = '0';
    }
    shellprintf("BCALL = %s\r\n", xcall);

    len = 32;
    memset(xcall, 0, sizeof(xcall));
    cfg_get_para(CFG_ITEM_ECALL, (unsigned char *)xcall, &len);
    if(xcall[0] == 0)
    {
        xcall[0] = '0';
    }
    shellprintf("ECALL = %s\r\n", xcall);

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_WHITE_LIST, (unsigned char *)buff, &len);
    if(buff[0] == 0)
    {
        buff[0] = '0';
    }
    shellprintf("WHITE LIST = %s\r\n", buff);

    shellprintf("INTEST MPU SOFTWARE VERSION = %s\r\n", dev_get_version());

    memset(version, 0, sizeof(version));
    upg_get_mcu_run_ver(version, sizeof(version));
    shellprintf("INTEST MCU SOFTWARE VERSION = %s\r\n", version);
    memset(version, 0, sizeof(version));
    ret = upg_get_mcu_upg_ver(version, sizeof(version));
    if (0 == ret)
    {
        shellprintf("INTEST MCU UPGRADE VERSION = %s\r\n", version);
    }
    else
    {
        shellprintf("INTEST MCU UPGRADE VERSION = UNKNOWN\r\n");
    }

    memset(version, 0, sizeof(version));
    upg_get_mcu_blt_ver(version, sizeof(version));
    shellprintf("INTEST MCU BOOTLOADER VERSION = %s\r\n", version);

    shellprintf("HOZON SOFTWARE VERSION  = %s\r\n", DID_F1B0_SW_UPGRADE_VER);

    char hw[32] = {0};
    len = sizeof(hw);
    ret |= cfg_get_para(CFG_ITEM_INTEST_HW,hw,&len);
    if(hw[0] == 0)
    {
        memcpy(hw,"00.00",5);
    }
    shellprintf("HOZON HARDWARE VERSION = %s\r\n", hw);

    memset(version, 0, sizeof(version));
    upg_get_fw_ver(version, sizeof(version));
    shellprintf("FIRMWARE VERSION = %s\r\n", version);

    /* �������Ϊ���һ���������������ʾ�������ӵ�����֮ǰ */
    shellprintf("System running time = %llus\r\n", tm_get_time() / 1000);
    return 0;
}

/* get blocksize of sdcard */
int app_shell_drsdinfo(int argc, const char **argv)
{
    struct statfs diskInfo;

    statfs(COM_SDCARD_DIR, &diskInfo);
    unsigned long long blocksize = diskInfo.f_bsize;    //ÿ��block��������ֽ���
    unsigned long long totalsize = blocksize * diskInfo.f_blocks;   //�ܵ��ֽ�����f_blocksΪblock����Ŀ
    log_e(LOG_ASSIST, "Total_size = %llu B = %llu KB = %llu MB = %llu GB\n",
          totalsize, totalsize >> 10, totalsize >> 20, totalsize >> 30);

    unsigned long long freeDisk = diskInfo.f_bfree * blocksize; //ʣ��ռ�Ĵ�С
    unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //���ÿռ��С
    log_e(LOG_ASSIST, "free_size = %llu B = %llu KB = %llu MB = %llu GB\n",
          freeDisk, freeDisk >> 10, freeDisk >> 20, freeDisk >> 30);
    log_e(LOG_ASSIST, "available_size = %llu B = %llu KB = %llu MB = %llu GB\n",
          availableDisk, availableDisk >> 10, availableDisk >> 20, availableDisk >> 30);

    shellprintf("SD Status: \t%llu / %llu(MB)\r\n", availableDisk >> 20, totalsize >> 20);

    return 0;
}

int app_shell_drlist(int argc, const char **argv)
{
    DIR *dir;
    struct dirent *ptr;

    if (1 !=  argc || '/' != argv[0][0])
    {
        shellprintf("usage: drlist /file\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[0], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[0], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    log_o(LOG_ASSIST, "current path=%s", argv[0]);

    if ((dir = opendir(argv[0])) == NULL)
    {
        shellprintf(" error:open dir failed\r\n");
        return -1;
    }

    while ((ptr = readdir(dir)) != NULL)
    {
        if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
        {
            continue;
        }

        shellprintf(" %s\r\n", ptr->d_name);
    }

    closedir(dir);
    shellprintf(" ok:end\r\n");
    return 0;
}

/* list all files in SD card */
int app_shell_drnewdir(int argc, const char **argv)
{
    int ret;

    if (1 != argc || '/' != argv[0][0])
    {
        log_e(LOG_ASSIST, "usage: drnewdir dstpath");
        shellprintf(" error:dstpath error!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[0], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[0], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    log_o(LOG_ASSIST, "make path=%s", argv[0]);

    if (!path_exists(argv[0]))
    {
        ret = dir_make_path(argv[0], S_IRUSR | S_IWUSR | S_IXUSR, false);

        if (ret != 0)
        {
            shellprintf(" error:make dir failed!\r\n");
            return ret;
        }

        shellprintf(" ok\r\n");
    }
    else
    {
        shellprintf(" error:dir already exist!\r\n");
    }

    return 0;
}

/* delete file in SD card */
int app_shell_drdel(int argc, const char **argv)
{
    int ret;

    if (1 != argc)
    {
        log_e(LOG_ASSIST, "usage: drdel dstpath");
        shellprintf(" error:dstpath error!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[0], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[0], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    log_o(LOG_ASSIST, "del path=%s", argv[0]);

    if (path_exists(argv[0]))
    {
        if (file_isusing(argv[0]))
        {
            shellprintf(" error:file is using\r\n");
            return SHELL_FILE_BE_USING;
        }

        ret = dir_remove_path(argv[0]);

        if (ret != 0)
        {
            shellprintf(" error:delete dir failed!\r\n");
            return ret;
        }

        shellprintf(" ok\r\n");
    }
    else
    {
        shellprintf(" error:dir is not exist!\r\n");
    }

    return 0;
}

int app_shell_sdnew(int argc, const char **argv)
{
    int ret, fd;
    char path[256];

    if ((1 !=  argc) || strlen(argv[0]) >= sizeof(path))
    {
        log_e(LOG_ASSIST, "usage: sdnew file");
        shellprintf(" usage: sdnew file\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    // check if the input is a file
    if ('/' == argv[0][strlen(argv[0]) - 1])
    {
        log_e(LOG_ASSIST, "(%s) is not a file", argv[0]);
        shellprintf(" error:is not a file!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[0], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[0], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    log_o(LOG_ASSIST, "make file=%s", argv[0]);

    // check if the input file exists
    if (file_exists(argv[0]))
    {
        log_e(LOG_ASSIST, "(%s) is existed", argv[0]);
        shellprintf(" error:file is existed!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    memset(path, 0, sizeof(path));
    strncpy(path, argv[0], strlen(argv[0]));

    if (!dir_exists(dirname((char *)argv[0])))
    {
        ret = dir_make_path(path, S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            shellprintf(" error:make dir failed!\r\n");
            return ret;
        }
    }

    fd = file_create(path, 0644);

    if (fd > 0)
    {
        close(fd);
        shellprintf(" ok\r\n");
    }
    else
    {
        shellprintf(" error:make file failed!\r\n");
    }

    return 0;
}

int app_shell_sddel(int argc, const char **argv)
{
    if (1 != argc)
    {
        log_e(LOG_ASSIST, "usage: sddel dstfile");
        shellprintf(" error:dstfile error\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    // check if the input is a file
    if ('/' == argv[0][strlen(argv[0]) - 1])
    {
        log_e(LOG_ASSIST, "(%s) is not a file", argv[0]);
        shellprintf(" error:is not a file!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[0], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[0], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    log_o(LOG_ASSIST, "del file=%s", argv[0]);

    if (file_exists(argv[0]))
    {
        if (file_isusing(argv[0]))
        {
            shellprintf(" error:file is using\r\n");
            return SHELL_FILE_BE_USING;
        }
        else
        {
            file_delete(argv[0]);
            shellprintf(" ok\r\n");
        }
    }
    else
    {
        shellprintf(" error:file is not exist!\r\n");
    }

    return 0;
}

int app_shell_sdcopy(int argc, const char **argv)
{
    if (2 != argc || ('/' != argv[0][0]) || ('/' != argv[1][0]))
    {
        log_e(LOG_ASSIST, "usage:sdcopy /src-file /des-file");
        shellprintf(" error:para error\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    // check if the input is a file
    if ('/' == argv[0][strlen(argv[0]) - 1])
    {
        log_e(LOG_ASSIST, "(%s) is not a file", argv[0]);
        shellprintf(" error:is not a file!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if ('/' == argv[1][strlen(argv[1]) - 1])
    {
        log_e(LOG_ASSIST, "(%s) is not a file", argv[1]);
        shellprintf(" error:is not a file!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (strncmp(argv[1], COM_SDCARD_DIR, strlen(COM_SDCARD_DIR))
        && strncmp(argv[1], COM_USRDATA_DIR, strlen(COM_USRDATA_DIR)))
    {
        shellprintf(" error:can't operate the dir!\r\n");
        return SHELL_INVALID_PARAMETER;
    }

    if (0 != file_copy(argv[0], argv[1]))
    {
        shellprintf(" error:cppy failed!\r\n");
        return SHELL_INVALID_PARAMETER;
    }
    else
    {
        shellprintf(" ok\r\n");
    }

    return 0;
}

/****************************************************************
function:     assist_shell_init
description:  initiaze assist shell module
input:        INIT_PHASE phase, init phase;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int assist_shell_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            shell_cmd_register_ex("ascfg", "drcfg", app_shell_drcfg, "dump cfg for assist");
            /* sd information */
            shell_cmd_register_ex("assdinfo", "drsdinfo", app_shell_drsdinfo, "get sdcard info");
            /* Directory operation in sdcard */
            shell_cmd_register_ex("aslist", "drlist", app_shell_drlist, "list file and dir in sdcard");
            shell_cmd_register_ex("asmkdir", "drnewdir", app_shell_drnewdir, "new dir in sdcard or usrdata");
            shell_cmd_register_ex("asdel", "drdel", app_shell_drdel, "del file or dir");
            /* file operation in sdcard */
            shell_cmd_register_ex("assdnew", "sdnew", app_shell_sdnew, "new file in sdcard");
            shell_cmd_register_ex("assddel", "sddel", app_shell_sddel, "delete file in sdcard");
            shell_cmd_register_ex("assdcp", "sdcopy", app_shell_sdcopy, "duplicate file in sdcard");
            break;

        default:
            break;
    }

    return 0;
}

