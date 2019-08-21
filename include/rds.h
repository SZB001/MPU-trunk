
/****************************************************************
file:         rds.h
description:  the header file of reliable data store definition
date:         2016/11/23
author        liuzhongwen
****************************************************************/

#ifndef __RDS_H__
#define __RDS_H__

#include "com_app_def.h"
#include "file.h"
#include "dir.h"
#include "dev_rw.h"
typedef enum RDS_ERROR_CODE
{
    RDS_INVALID_PARA     = (MPU_MID_MID_RDS << 16) | 0x01,
    RDS_INVALID_TYPE,
    RDS_FILE_NOT_EXIST,
    RDS_INVALID_FILE,
    RDS_OPEN_FILE_FAILED,
} RDS_ERROR_CODE;


#define RDS_MAX_FILE_NAME     (64)

/*-------------dynamic data path definition begin-----------------*/

/*dynamic data refers to the data which is changed periodically, such as distance,
  history data, vehicle fault and so on */

/* there is no partition backup for data, data is restored only in one partition */



#define RDS_MASTER_PATH           "/master"
#define RDS_SLAVE_PATH            "/slave"


#define RDS_DEV_INFO_FILE         "dev_info.dat"         /* store dev info */
#define RDS_DEV_DISTANCE_FILE     "dev_distance.dat"     /* store distance */
#define RDS_UDS_DIAG_FILE         "uds_diag.dat"         /* store uds diag fault */
#define RDS_J1939_FAULT_FILE      "J1939_fault.dat"      /* store J1939 fault */
#define RDS_SYS_CFG_FILE          "sys_cfg.dat"          /* store configuration */
#define RDS_OTA_FAULT_FILE        "ota_fault.dat"
#define RDS_ADP_CFG_FILE          "adp_cfg.dat"
#define RDS_FT_REGSEQ_FILE        "ft_regseq.dat"
#define RDS_USER_REGSEQ_FILE        "user_regseq.dat"
/*--------------dynamic data path definition end------------------*/

typedef enum
{
    RDS_DATA_DEV_INFO,     /* dev info */
    RDS_DATA_DISTANCE,     /* distance */
    RDS_DATA_UDS_DIAG,     /* uds diag fault info */
    RDS_J1939_FAULT,       /* J1939 fault info */
    RDS_SYS_CFG,           /* t-box app configuration */
    RDS_ADAPTIVE_CFG,     /* the cfg change more frequently */
    RDS_FOTON_REGSEQ,     /* FOTON login sequence */
    RDS_USER_CFG
} RDS_DATA_TYPE;

typedef enum
{
    RDS_SINGLE,            /* not backup */
    RDS_BACKUP,            /* backup in current system */
} RDS_BACKUP_ATTR;

typedef enum
{
    RDS_PATH_MASTER,      /* master area */
    RDS_PATH_BACKUP,      /* backup area */
} RDS_PATH_TYPE;

typedef struct
{
    unsigned short type;
    char path_name[RDS_MAX_FILE_NAME];
	char area_path[RDS_MAX_FILE_NAME];
    unsigned short backup_attr;
    char file_name[RDS_MAX_FILE_NAME];
} RDS_ITEM;

typedef struct
{
    char ver[COM_APP_VER_LEN];
    unsigned int len;
} RDS_HDR;

/* get the data from the specified type file */
int rds_get(RDS_DATA_TYPE type, unsigned char *data, unsigned int *len, char *ver);

/* update the data into the specified type file, add header and MD5 into file */
int rds_update_once(RDS_DATA_TYPE type, unsigned char *data, unsigned int len);

/* update the raw data into the specified type file */
int rds_update_raw(RDS_DATA_TYPE type, unsigned char *data, unsigned int len);

void rds_set_default(void);

#endif

