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
#include "at_api.h"
#include "../support/protocol.h"
#include "dir.h"
#include "file.h"
#include "timer.h"
#include "dsu_api.h"
#include "uds_did.h"
#include "udef_cfg_api.h"
#include "hozon_PP_api.h"

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
    char vin[18];

    len = sizeof(unsigned int);
    cfg_get_para(CFG_ITEM_DEV_NUM, (unsigned char *)(&tmpInt), &len);
    shellprintf("零部件编号 = %u\r\n", tmpInt);

    len = sizeof(vin);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_VIN, vin, &len);
    shellprintf("车辆VIN = %s\r\n", vin);

    char tboxsn[19];
    len = sizeof(tboxsn);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXSN, tboxsn, &len);
    shellprintf("TBOX SN = %s\r\n", tboxsn);

    uint32_t tboxid;
    len = 4;
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_TBOXID, &tboxid, &len);
    shellprintf("TBOX ID = %d\r\n",tboxid);

    char iccid[21];
    getPP_rmtCfg_iccid(iccid);
    shellprintf("TBOX ICCID = %s\r\n", iccid);

    char mpusw[11];
    len = sizeof(mpusw);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_MPUSW, mpusw, &len);
    shellprintf("mpuSw = %s\r\n", mpusw);

    char mcusw[11];
    len = sizeof(mcusw);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_MCUSW, mcusw, &len);
    shellprintf("mcuSw = %s\r\n", mcusw);

    char cfgver[33] = {0};
    getPP_rmtCfg_cfgVersion(cfgver);
    shellprintf("TSP远程配置版本 = %s\r\n", cfgver);

    len = sizeof(url.url);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_URL, url.url, &len);
    shellprintf("平台地址(无PKI) = %s\r\n", url.url);
    len = sizeof(url.port);
    ret |= cfg_get_user_para(CFG_ITEM_GB32960_PORT, &url.port, &len);
    shellprintf("平台端口(无PKI) = %d\r\n", url.port);

    char sgLinkAddr[33];
	int	 sgPort;
    getPP_rmtCfg_certAddrPort(sgLinkAddr,&sgPort);
    shellprintf("单向链路地址(带PKI) = %s\r\n", sgLinkAddr);
    shellprintf("单向链路端口(带PKI) = %d\r\n", sgPort);

    char bdlLinkAddr[33];
	int	 bdlPort;
    getPP_rmtCfg_tspAddrPort(bdlLinkAddr,&bdlPort);
    shellprintf("双向链路地址(带PKI) = %s\r\n", bdlLinkAddr);
    shellprintf("双向链路端口(带PKI) = %d\r\n", bdlPort);

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_DBC_PATH, buff, &len);
    shellprintf("DBC配置 = %s\r\n", buff);

    char certSn[32] = {0};
    len = sizeof(certSn);
    ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_CERT,certSn,&len);
    shellprintf("证书 = %s\r\n", certSn);
    len = 1;
    uint8_t EnFlag;
	ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_CERT_EN,&EnFlag,&len);
    shellprintf("证书启用 = %d\r\n", EnFlag);

    char wanAPN[32] = {0};
    len = sizeof(wanAPN);
    ret |= cfg_get_para(CFG_ITEM_WAN_APN,wanAPN,&len);
    shellprintf("WAN APN = %s\r\n", wanAPN);

    len = 1;
	ret |= cfg_get_user_para(CFG_ITEM_HOZON_TSP_FORBIDEN,&EnFlag,&len);
    shellprintf("蓝牙一键启动 = %d\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_WIFI_SET,&EnFlag,&len);
    shellprintf("WIFI ENABLE = %d\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_DCOM_SET,&EnFlag,&len);
    shellprintf("DCOM ENABLE = %d\r\n", EnFlag);

    char blename[50] = {0};
    len = sizeof(blename);
    ret |= cfg_get_para(CFG_ITEM_BLE_NAME,blename,&len);
    shellprintf("BLE NAME = %s\r\n", blename);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_BLE,&EnFlag,&len);
    shellprintf("BLE ENABLE = %d\r\n", EnFlag);

    len = 1;
	ret |= cfg_get_para(CFG_ITEM_EN_PKI,&EnFlag,&len);
    shellprintf("PKI ENABLE = %d\r\n", EnFlag);

    len = sizeof(commpara.data_val);
    ret |= cfg_get_para(CFG_ITEM_FOTON_INTERVAL, &commpara.data_val, (uint32_t *)&len);
    shellprintf("�����ϴ���� = %ds\r\n", commpara.data_val);
    len = sizeof(commpara.hb_val);
    ret |= cfg_get_para(CFG_ITEM_FOTON_INTERVAL, &commpara.hb_val, (uint32_t *)&len);
    shellprintf("�����ϴ���� = %ds\r\n", commpara.hb_val);

    len = sizeof(sleepmode);
    cfg_get_para(CFG_ITEM_SLEEP_MODE, &sleepmode, &len);
    shellprintf("���߲��� = ");

    switch (sleepmode)
    {
        case 0:
            shellprintf("RUNNING ONLY \r\n");
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

    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_0, &canbaud, &len);
    shellprintf("HCAN DEFAULT������ = %dK\r\n", canbaud);
    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_1, &canbaud, &len);
    shellprintf("MCAN DEFAULT������ = %dK\r\n", canbaud);
    len = sizeof(canbaud);
    cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_2, &canbaud, &len);
    shellprintf("LCAN DEFAULT������ = %dK\r\n", canbaud);

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_ICALL, (unsigned char *)buff, &len);
    shellprintf("ICALL���� = %s\r\n", buff);

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_BCALL, (unsigned char *)buff, &len);
    shellprintf("BCALL���� = %s\r\n", buff);

    len = sizeof(buff);
    memset(buff, 0, sizeof(buff));
    cfg_get_para(CFG_ITEM_WHITE_LIST, (unsigned char *)buff, &len);
    shellprintf("���������� = %s\r\n", buff);

    shellprintf("INTEST_软件版本号 = %s\r\n", dev_get_version());
    shellprintf("HOZON_软件版本号  = %s\r\n", DID_F1B0_SW_FIXED_VER);
    shellprintf("HOZON_硬件版本号  = %s\r\n", DID_F191_HW_VERSION);

    memset(version, 0, sizeof(version));
    upg_get_fw_ver(version, sizeof(version));
    shellprintf("4Gģ��汾�� = %s\r\n", version);

    /* �������Ϊ���һ���������������ʾ�������ӵ�����֮ǰ */
    shellprintf("ϵͳ����ʱ�� = %llus\r\n", tm_get_time() / 1000);
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

