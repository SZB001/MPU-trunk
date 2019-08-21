/****************************************************************
file:         rds.c
description:  the source file of reliable data store implementation
date:         2016/11/23
author        liuzhongwen
****************************************************************/
#include "com_app_def.h"
#include "md5.h"
#include "rds.h"

static pthread_mutex_t rds_mutex = PTHREAD_MUTEX_INITIALIZER;

RDS_ITEM rds_data_tbl[] =
{
    { RDS_DATA_DEV_INFO, COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_DEV_INFO_FILE     },
    { RDS_DATA_DISTANCE, COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_DEV_DISTANCE_FILE },
    { RDS_DATA_UDS_DIAG, COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_UDS_DIAG_FILE     },
    { RDS_J1939_FAULT,   COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_J1939_FAULT_FILE  },
    { RDS_SYS_CFG,       COM_APP_CUR_CFG_DIR,  COM_DATA_CUR_CFG_DIR,RDS_BACKUP,   RDS_SYS_CFG_FILE},
    { RDS_ADAPTIVE_CFG,  COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_ADP_CFG_FILE      },
    { RDS_FOTON_REGSEQ,  COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_FT_REGSEQ_FILE    },
    { RDS_USER_CFG,      COM_APP_CUR_DATA_DIR, COM_NULL_DIR, RDS_BACKUP,   RDS_USER_REGSEQ_FILE  },
};

/*****************************************************************************
function:     rds_get_index
description:  get the index of specified type item in rds_data_tbl
input:        RDS_DATA_TYPE type, data type
output:       none
return:       positive or 0 indicates the found index;
              -1 indicates not found;
*****************************************************************************/
static int rds_get_index(RDS_DATA_TYPE type)
{
    int i;

    for (i = 0; i < sizeof(rds_data_tbl) / sizeof(RDS_ITEM); i++)
    {
        if (rds_data_tbl[i].type == type)
        {
            break;
        }
    }

    /* not found */
    if (i >= sizeof(rds_data_tbl) / sizeof(RDS_ITEM))
    {
        return -1;
    }

    return i;
}

/*****************************************************************************
function:     rds_build_single_path
description:  build the data file path which is not need to backup
input:        int index, the index of specified type item in rds_data_tbl;
output:       the absolute data file path
return:       none
*****************************************************************************/
static void rds_build_single_path(int index, char *path)
{
    int len;

    len = strlen(rds_data_tbl[index].path_name);
    memcpy(path, rds_data_tbl[index].path_name, len);

    path[len++] = '/';
    path[len]   = 0;
    strcat(path, rds_data_tbl[index].file_name);

    return;
}

/*****************************************************************************
function:     rds_build_backup_path
description:  build the data file path in master area or backup area
input:        int index, the index of specified type item in rds_data_tbl;
              bool is_backup, true, build backup file path;
                              fasle, build master file path
output:       the absolute data file path
return:       none
*****************************************************************************/
static void rds_build_backup_path(int index, bool is_backup, char *path)
{
    int len;

    len = strlen(rds_data_tbl[index].path_name);
    memcpy(path, rds_data_tbl[index].path_name, len);
    path[len] = 0;

    if (!is_backup)
    {
        strcat(path, RDS_MASTER_PATH);
        len = len + strlen(RDS_MASTER_PATH);
    }
    else
    {
        strcat(path, RDS_SLAVE_PATH);
        len = len + strlen(RDS_SLAVE_PATH);
    }

    path[len++] = '/';
    path[len]   = 0;
    strcat(path, rds_data_tbl[index].file_name);

    return;
}

/*****************************************************************************
function:     rds_get_full_path
description:  build the data file path
input:        int index, the index of specified type item in rds_data_tbl;
              bool is_backup, true, build backup file path;
                              fasle, build master file path
output:       the absolute data file path
return:       none
*****************************************************************************/
static void rds_get_full_path(int index, RDS_PATH_TYPE type,  char *path)
{
    /* only backup is not needed, ignore type para */
    if (RDS_SINGLE == rds_data_tbl[index].backup_attr)
    {
        rds_build_single_path(index, path);
        return;
    }

    /* get file path refers to master area */
    if (RDS_PATH_MASTER == type)
    {
        rds_build_backup_path(index, false, path);
    }
    /* get file path refers to backup area */
    else if (RDS_PATH_BACKUP == type)
    {
        rds_build_backup_path(index, true, path);
    }
    else
    {
        path[0] = 0;
    }

    return;
}

/*****************************************************************************
function:     rds_write_raw
description:  write the data to the file refers to the path without header and md5
input:        const char *path, file path;
              unsigned char*data, data;
              unsigned int len, data length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
static int rds_write_raw(const char *path, unsigned char *data, unsigned int len)
{
    int ret;

    if (!file_exists(path))
    {
        ret = dir_make_path(path, S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            return ret;
        }
    }

    ret = file_write_atomic(path, data, len, S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

/*****************************************************************************
function:     rds_write
description:  write the data to the file refers to the path and add additional
              header and md5
input:        const char *path, file path;
              unsigned char*data, data;
              unsigned int len, data length
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
static int rds_write(const char *path, unsigned char *data, unsigned int len)
{
    int ret;
    RDS_HDR hdr;

    if (!file_exists(path))
    {
        ret = dir_make_path(path, S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            return ret;
        }
    }

    memset(&hdr, 0, sizeof(hdr));

    hdr.len = len;
    memcpy(hdr.ver, COM_APP_MAIN_VER, strlen(COM_APP_MAIN_VER));

    ret = file_update_atomic(path, (unsigned char *)&hdr, sizeof(hdr),
                             data, len, S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret != 0)
    {
        return ret;
    }

    return 0;
}

/*****************************************************************************
function:     rds_update_once
description:  update the data into the specified type file
              without any additional info
              NOTE:you must update all the data when the function is called
input:        RDS_DATA_TYPE type.  data type, refer to RDS_DATA_TYPE;
              unsigned char*data,  the data need to update;
              unsigned int len, the data length
output:       none
return:       0 indicates success,
              others indicates failed
*****************************************************************************/
int rds_update_raw(RDS_DATA_TYPE type, unsigned char *data, unsigned int len)
{
    int index, ret;
    char path[COM_APP_MAX_PATH_LEN] = {0};

    if ((NULL == data) || (0 == len))
    {
        log_e(LOG_MID, "invalid para");
        return RDS_INVALID_PARA;
    }

    pthread_mutex_lock(&rds_mutex);
    
    index = rds_get_index(type);

    if (index < 0)
    {
        pthread_mutex_unlock(&rds_mutex);
        log_e(LOG_MID, "invalid type:%u", type);
        return RDS_INVALID_TYPE;
    }

    rds_get_full_path(index, RDS_PATH_MASTER,  path);

    ret = rds_write_raw(path, data, len);

    pthread_mutex_unlock(&rds_mutex);

    return ret;
}

/*****************************************************************************
function:     rds_update_once
description:  update the data into the specified type file,
              NOTE:you must update all the data when the function is called
input:        RDS_DATA_TYPE type.  data type, refer to RDS_DATA_TYPE;
              unsigned char*data,  the data need to update;
              unsigned int len, the data length
output:       none
return:       0 indicates success,
              others indicates failed
*****************************************************************************/
int rds_update_once(RDS_DATA_TYPE type, unsigned char *data, unsigned int len)
{
    int index, ret;
    char master_path[COM_APP_MAX_PATH_LEN] = {0};
    char backup_path[COM_APP_MAX_PATH_LEN] = {0};

    if ((NULL == data) || (0 == len))
    {
        log_e(LOG_MID, "invalid para");
        return RDS_INVALID_PARA;
    }

    pthread_mutex_lock(&rds_mutex);
    index = rds_get_index(type);

    if (index < 0)
    {
        pthread_mutex_unlock(&rds_mutex);
        log_e(LOG_MID, "invalid type:%u", type);
        return RDS_INVALID_TYPE;
    }

    log_o(LOG_MID, "rds_update_once,type:%u, len:%u", type, len);

    /* write the header and data into master area */
    rds_get_full_path(index, RDS_PATH_MASTER,  master_path);
    ret = rds_write(master_path, data, len);

    if (ret != 0)
    {
        pthread_mutex_unlock(&rds_mutex);
        return ret;
    }

    /* copy it to backup area if backup is needed */
    if (RDS_BACKUP == rds_data_tbl[index].backup_attr)
    {
        rds_get_full_path(index, RDS_PATH_BACKUP,  backup_path);
        ret = file_copy(master_path, backup_path);		
    }
	
	/* copy it to another area if backup is needed */
	if (strlen(rds_data_tbl[index].area_path) != 0)
	{
	 	ret = dir_copy(rds_data_tbl[index].path_name, rds_data_tbl[index].area_path);
	}

    pthread_mutex_unlock(&rds_mutex);
    return ret;
}

/*****************************************************************************
function:     rds_read
description:  build the data file path
input:        const char *path, file path;
output:       unsigned char*data, data buffer;
              unsigned int *len, the length of data;
              unsigned char *ver, version
return:       0 indicates success,
              others indicates failed
*****************************************************************************/
static int rds_read(const char *path, unsigned char *data, unsigned int *len,  char *ver)
{
    int fd, size, ret;
    RDS_HDR hdr;
    unsigned char md5[MD5_LENGTH];
    MD5_CTX  md5_ctx;

    if (!file_exists(path))
    {
        log_o(LOG_MID, "the file not exist, path:%s", path);
        return RDS_FILE_NOT_EXIST;
    }

    size = file_size(path);

    if (size <= sizeof(hdr) + MD5_LENGTH)
    {
        log_e(LOG_MID, "invalid file,size:%u, path:%s", size, path);
        return RDS_INVALID_FILE;
    }

    fd = file_open_read(path);

    if (fd < 0)
    {
        log_e(LOG_MID, "open file failed, path:%s", path);
        return RDS_OPEN_FILE_FAILED;
    }

    ret = dev_read(fd, (unsigned char *)&hdr, sizeof(hdr));

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    if (size != sizeof(hdr) + MD5_LENGTH + hdr.len)
    {
        log_e(LOG_MID, "invalid file,size:%u, data len:%u, path:%s", size, hdr.len, path);
        close(fd);
        return RDS_INVALID_FILE;
    }

    if (*len < hdr.len)
    {
        log_e(LOG_MID, "invalid data len,actual len:%u, buf len:%u, path:%s", hdr.len, *len, path);
        return RDS_INVALID_PARA;
    }

    ret = dev_read(fd, data, hdr.len);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    ret = dev_read(fd, md5, MD5_LENGTH);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    close(fd);

    /* compute md5 and write */
    MD5Init(&md5_ctx);
    MD5Update(&md5_ctx, (unsigned char *)&hdr, sizeof(hdr));
    MD5Update(&md5_ctx, data, hdr.len);
    MD5Final(&md5_ctx);

    if (0 != memcmp(md5_ctx.digest, md5, MD5_LENGTH))
    {
        log_e(LOG_MID, "md5 is different,path:%s", path);
        return RDS_INVALID_FILE;
    }

    *len = hdr.len;

    if (ver)
    {
        strncpy(ver, hdr.ver, COM_APP_VER_LEN);
        ver[COM_APP_VER_LEN - 1] = 0;
    }

    return 0;
}

/*****************************************************************************
function:     rds_get
description:  get the data from the specified type file
input:        RDS_DATA_TYPE type, data type;
output:       unsigned char*data, data buffer;
              unsigned int *len, the length of data;
              unsigned char *ver, version
return:       0 indicates success,
              others indicates failed
*****************************************************************************/
int rds_get(RDS_DATA_TYPE type, unsigned char *data, unsigned int *len, char *ver)
{
    int index, ret;
    char master_path[COM_APP_MAX_PATH_LEN] = {0};
    char backup_path[COM_APP_MAX_PATH_LEN] = {0};

    if ((NULL == data) || (NULL == len))
    {
        log_e(LOG_MID, "invalid para");
        return RDS_INVALID_PARA;
    }

    pthread_mutex_lock(&rds_mutex);
    index = rds_get_index(type);

    if (index < 0)
    {
        pthread_mutex_unlock(&rds_mutex);
        log_e(LOG_MID, "invalid type:%u", type);
        return RDS_INVALID_TYPE;
    }

    /* get data from master area */
    rds_get_full_path(index, RDS_PATH_MASTER,  master_path);
    ret = rds_read(master_path, data, len, ver);

    if (0 == ret)
    {
        pthread_mutex_unlock(&rds_mutex);
        return 0;
    }

    /* if the data is not need to backup, just return */
    if (RDS_BACKUP != rds_data_tbl[index].backup_attr)
    {
        pthread_mutex_unlock(&rds_mutex);
        return ret;
    }

    /* if master data is invalid, try to get data from backup area */
    rds_get_full_path(index, RDS_PATH_BACKUP,  backup_path);
    ret = rds_read(backup_path, data, len, ver);

    if (0 != ret)
    {
        pthread_mutex_unlock(&rds_mutex);
        return ret;
    }

    /* recover file in master area */
    file_copy(backup_path, master_path);
    pthread_mutex_unlock(&rds_mutex);
    
    return 0;
}

/*****************************************************************************
function:     rds_set_default
description:  delete all the rds file 
input:        none
output:       none
return:       0 indicates success,
              others indicates failed
*****************************************************************************/
void rds_set_default(void)
{
    int i;
    char path[COM_APP_MAX_PATH_LEN]={0};

    pthread_mutex_lock(&rds_mutex);
    
    for (i = 0; i < sizeof(rds_data_tbl) / sizeof(RDS_ITEM); i++)
    {
        if( RDS_SINGLE == rds_data_tbl[i].backup_attr )
        {
            rds_get_full_path(i, RDS_PATH_MASTER, path); 
            file_delete(path);
        }
        else if (RDS_USER_CFG == rds_data_tbl[i].type)
        {
            continue;
        }
        else 
        {
            rds_get_full_path(i, RDS_PATH_MASTER, path); 
            file_delete(path);

            rds_get_full_path(i, RDS_PATH_BACKUP, path); 
            file_delete(path);
        }
    }

    pthread_mutex_unlock(&rds_mutex);

    return;
}


