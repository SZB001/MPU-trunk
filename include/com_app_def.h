
/****************************************************************
file:         com_app_def.h
description:  the header file of tbox para definition
date:         2016/11/17
author        liuzhongwen
****************************************************************/

#ifndef __COM_APP_DEF_H__
#define __COM_APP_DEF_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include "mid_def.h"
#include "log.h"
#include "pwdg.h"
#include "shell_api.h"

#define COM_APP_VER_LEN            (32)
#define COM_APP_MAIN_VER           "MPU_J311HZCOMM003[A-31]"
#define COM_APP_SYS_VER            (COM_APP_MAIN_VER "\\" __DATE__ "\\" __TIME__)

#define COM_HW_VER                 "INP01"

#define COM_APP_MAX_PATH_LEN       (256)

#define COM_APP_UPG_PROXY_NAME     "com_app_upg_proxy"
#define COM_APP_UPG_CTL_NAME       "com_app_upg_manager"

#define COM_APP_SHELL_CMD_NAME     "com_app_shell_cmd"
#define COM_APP_SHELL_CTL_NAME     "com_app_shell_manager"

#define COM_APP_IMAGE              "tbox_app.bin"
#define COM_SCRIPT                 "script.sh"
#define COM_APPL_IMAGE             "appl.bin"
#define COM_MCU_APP                "mcu_app.bin"
#define COM_MCU_BOOT               "mcu_boot.bin"
#define COM_FW_UPDATE              "update.zip"

#define COM_MCU_VER                "mcu_ver.dat"
#define COM_PKG_VER                "pkg_ver.dat"
#define COM_PKG_FILE               "TerminalUpdates.pkg"
#define COM_PKG_FILE_BAK           "TerminalUpdates.bak"


#define COM_FW_EX_VER_FILE         "/intest.ver"
#define COM_APP_STATUS_FILE        "status.conf"
#define COM_APP_STARTUP_FILE       "shm_tbox_start_up"

#define COM_APP_PRE_DIR            "/usrapp/previous"
#define COM_APP_CUR_DIR            "/usrapp/current"
#define COM_DATA_CUR_DIR           "/usrdata/current"

#define COM_USRDATA_DIR            "/usrdata"
#define COM_APP_DATA_DIR           "/usrdata/dev"
#define COM_APP_UPG_DIR            "/usrdata/upgrade"
#define COM_APP_PKG_DIR            "/usrdata/pkg"
#define COM_APP_UPLOAD_DIR         "/usrdata/upload"  /* if emmc is used, this dir is not used */
#define COM_APP_LOG_DIR            "/usrdata/log"     /* if emmc is used, this dir is not used */

#define COM_APP_PRE_IMAGE_DIR      COM_APP_PRE_DIR  "/data/image"
#define COM_APP_PRE_CFG_DIR        COM_APP_PRE_DIR  "/data/cfg"
#define COM_APP_PRE_DBC_DIR        COM_APP_PRE_DIR  "/data/dbc"
#define COM_APP_PRE_DATA_DIR       COM_APP_PRE_DIR  "/data/usrdata"

#define COM_APP_CUR_IMAGE_DIR      COM_APP_CUR_DIR  "/data/image"
#define COM_APP_CUR_CFG_DIR        COM_APP_CUR_DIR  "/data/cfg"
#define COM_APP_CUR_DBC_DIR        COM_APP_CUR_DIR  "/data/dbc"
#define COM_APP_CUR_DATA_DIR       COM_APP_DATA_DIR "/data/usrdata"

#define COM_DATA_CUR_IMAGE_DIR     COM_DATA_CUR_DIR "/data/image"
#define COM_DATA_CUR_CFG_DIR       COM_DATA_CUR_DIR "/data/cfg"
#define COM_DATA_CUR_DBC_DIR       COM_DATA_CUR_DIR "/data/dbc"
#define COM_DATA_CUR_DATA_DIR      COM_DATA_CUR_DIR "/data/usrdata"

#define COM_APP_UPG_IMAGE_DIR      COM_APP_UPG_DIR  "/data/image"
#define COM_APP_UPG_FW_DIR         COM_APP_UPG_DIR  "/data/fw"

#define COM_SDCARD_DIR             "/media/sdcard"

#define COM_LOG_DIR               COM_SDCARD_DIR"/log/"

#define COM_NULL_DIR 			   ""


#endif
