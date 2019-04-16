/**
 * @Title: dsu_file.c
 * @author yuzhimin
 * @date Nov 7, 2017
 * @version V1.0
 */

#include "dsu_main.h"

static unsigned char zip_buf[4 + DSU_ZBUF_SIZE + 512];
static pthread_mutex_t zip_mutex;

void dsu_compress_init(void)
{
    memset(zip_buf, 0, sizeof(zip_buf));
    pthread_mutex_init(&zip_mutex, NULL);
}

int dsu_set_opt(DSU_FILE *file, DSU_FILE_TYPE type, unsigned char opt)
{
    int ret;
    TCOM_MSG_HEADER msghdr;
    msghdr.sender = MPU_MID_DSU;
    msghdr.receiver = MPU_MID_DSU;
    msghdr.msglen = sizeof(DSU_FILE_TYPE);

    switch (opt)
    {
        case FILE_OPT_CREATE:
            msghdr.msgid = DSU_MSG_CREATE_FILE;
            DSU_LOCK();
            file->stat = FILE_ST_CREATING;  // add creating stat, for file open failed processing.
            DSU_UNLOCK();
            break;

        case FILE_OPT_WRITE:
            msghdr.msgid = DSU_MSG_WRITE_FILE;
            break;

        case FILE_OPT_SYNC:
            msghdr.msgid = DSU_MSG_SYNC_FILE;
            break;

        case FILE_OPT_CLOSE:
            msghdr.msgid = DSU_MSG_CLOSE_FILE;
            break;

        default:
            return -1;
    }

    ret = tcom_send_msg(&msghdr, &type);
    return ret;
}

unsigned char dsu_get_stat(DSU_FILE *file)
{
    unsigned char stat;
    DSU_LOCK();
    stat = file->stat;
    DSU_UNLOCK();
    return stat;
}

int dsu_disk_check(int mode)
{
    int ret;

    if (mode)
    {
        ret = dev_diag_get_emmc_status();
    }
    else
    {
        ret = flt_get_by_id(EMMC);
    }

    if (DIAG_EMMC_OK == ret)
    {
        ret = DISK_OK;
    }
    else if (DIAG_EMMC_FULL == ret)
    {
        ret = DISK_FULL;
    }
    else
    {
        ret = DISK_ERROR;
    }

    return ret;
}


int dsu_dir_check(void)
{
    int ret;
    // check sd card or emmc dir
    ret = dsu_disk_check(1);

    if (DISK_OK != ret)
    {
        return ret;
    }

    if (!dir_exists(DSU_IWDZ_FILE_PATH))
    {
        ret = dir_make_path(DSU_IWDZ_FILE_PATH"/", S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            log_e(LOG_DSU, "make iwdz path error(%d):%s", ret, DSU_IWDZ_FILE_PATH);
            return DISK_ERROR;
        }
    }

    if (!dir_exists(DSU_INX_FILE_PATH))
    {
        ret = dir_make_path(DSU_INX_FILE_PATH"/", S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            log_e(LOG_DSU, "make inx path error(%d):%s", ret, DSU_INX_FILE_PATH);
            return DISK_ERROR;
        }
    }

    if (!dir_is_dir(DSU_IWD_FILE_PATH))
    {
        ret = dir_make_path(DSU_IWD_FILE_PATH"/", S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (ret != 0)
        {
            log_e(LOG_DSU, "make iwd path error(%d):%s", ret, DSU_IWD_FILE_PATH);
            return DISK_ERROR;
        }
    }

    return DISK_OK;
}

int _dsu_fwrite(FILE *fp, unsigned char *buf, unsigned int len)
{
    #if 0
    assert(fp);
    assert(buf);
    #else

    if (NULL == fp || NULL == buf)
    {
        log_e(LOG_DSU, "fp or buf is NULL");
        return FR_ERR_WRITE;
    }

    #endif

    int ret, writecnt = 0;

    while (writecnt < len)
    {
        ret = fwrite(buf + writecnt, sizeof(unsigned char), len - writecnt, fp);

        if (ret > 0)
        {
            writecnt += ret;
        }
        else if (0 == ret)  /* timeout */
        {
            continue; /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            log_e(LOG_DSU, "fwrite error:%s", strerror(ferror(fp)));
            return FR_ERR_WRITE;
        }
    }

    return FR_OK;
}

static int dsu_fwrite(DSU_FILE *file)
{
    #if 0
    assert(file);
    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR_WRITE;
    }

    #endif

    int ret, size, pos = 0;
    unsigned long zout = sizeof(zip_buf) - 4;

    if (RAW_DATA == file->type)
    {
        ret = _dsu_fwrite(file->fp, file->fbuf, file->used);
    }
    else if (ZLIB_DATA == file->type)
    {
        pthread_mutex_lock(&zip_mutex);

        while (pos < file->used)
        {
            zout = sizeof(zip_buf) - 4;
            size = MIN(DSU_ZBUF_SIZE, file->used - pos);
            ret = compress(&zip_buf[4], &zout, file->fbuf + pos, size);

            if (Z_OK != ret)
            {
                log_e(LOG_DSU, "compress error!,%d", ret);
                file->used = 0;
                pthread_mutex_unlock(&zip_mutex);
                return FR_ERR_COMPRESS;
            }

            zip_buf[0] = zout >> 0;
            zip_buf[1] = zout >> 8;
            zip_buf[2] = zout >> 16;
            zip_buf[3] = zout >> 24;
            zout = zout + 4;
            ret = _dsu_fwrite(file->fp, zip_buf, (unsigned int) zout);
            pos += size;
        }

        pthread_mutex_unlock(&zip_mutex);
    }

    file->used = 0;
    return ret;
}

int dsu_file_init(DSU_FILE *file, unsigned char *buffer, unsigned int size, unsigned char type)
{
    #if 0
    assert(file);
    assert(buffer);
    #else

    if (NULL == file || NULL == buffer)
    {
        log_e(LOG_DSU, "dsu file or buf is NULL");
        return FR_ERR;
    }

    #endif

    memset(buffer, 0, size);
    file->fp = NULL;
    file->fbuf = buffer;
    file->size = size;
    file->used = 0;
    file->type = type;
    file->stat = FILE_ST_NONE;
    return FR_OK;
}

int dsu_fcreate(DSU_FILE *file, unsigned char type, const char *path, const char *mode)
{
    #if 0
    assert(file);
    assert(path);
    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR_OPEN;
    }

    if (NULL == path)
    {
        log_e(LOG_DSU, "file path is NULL");
        return FR_ERR_OPEN;

    }

    #endif


    file->fp = fopen(path, mode);
    file->used = 0;
    file->type = type;

    if (file->fp)
    {
        DSU_LOCK();
        file->stat = FILE_ST_OPEN;
        DSU_UNLOCK();
        log_o(LOG_DSU, "file creat:%s", path);
        return FR_OK;
    }
    else
    {
        DSU_LOCK();
        file->stat = FILE_ST_NONE;
        DSU_UNLOCK();
        log_e(LOG_DSU, "fopen failed, error:%s, file name;%s", strerror(errno), path);
        return FR_ERR_OPEN;
    }
}

int dsu_fb_used(DSU_FILE *file)
{
    #if 0
    assert(file);
    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return 0;
    }

    #endif

    return file->used;
}

int dsu_write_fb(DSU_FILE *file, unsigned char *data, unsigned int len)
{
    #if 0
    assert(file);
    assert(file->fp);
    assert(data);

    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR;
    }

    if (NULL == file->fp)
    {
        log_e(LOG_DSU, "fp is NULL,file:stat[%u],used[%d],name[%s]",
              file->stat, file->used, file->name);
        return FR_ERR;
    }

    if (NULL == data)
    {
        log_e(LOG_DSU, "write data is NULL");
        return FR_ERR;
    }

    #endif

    if (len > (file->size - file->used))
    {
        return FR_ERR_OVERFLOW;
    }

    memcpy(file->fbuf + file->used, data, len);
    file->used += len;

    return FR_OK;
}

int dsu_fsync(DSU_FILE *file)
{
    #if 0
    assert(file);
    assert(file->fp);
    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR;
    }

    if (NULL == file->fp)
    {
        log_e(LOG_DSU, "fp is NULL,file:stat[%u],used[%d],name[%s]",
              file->stat, file->used, file->name);
        return FR_ERR;
    }

    #endif

    int ret = FR_OK;

    if (0 != file->used)
    {
        ret = dsu_fwrite(file);
    }

    return ret;
}

int dsu_fclose(DSU_FILE *file)
{
    #if 0
    assert(file);
    assert(file->fp);

    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR;
    }

    if (NULL == file->fp)
    {
        log_e(LOG_DSU, "fp is NULL,file:stat[%u],used[%d],name[%s]",
              file->stat, file->used, file->name);
        return FR_ERR;
    }

    #endif

    int ret = FR_OK;

    if (0 != file->used)
    {
        ret = dsu_fwrite(file);
    }

    fflush(file->fp);
    fclose(file->fp);
    file->used = 0;
    DSU_LOCK();
    file->stat = FILE_ST_NONE;
    file->fp = NULL;
    DSU_UNLOCK();
    log_o(LOG_DSU, "file close:%s", file->name);
    return ret;
}

int dsu_change_fb_type(DSU_FILE *file, unsigned char type)
{
    #if 0
    assert(file);
    #else

    if (NULL == file)
    {
        log_e(LOG_DSU, "dsu file is NULL");
        return FR_ERR;
    }

    #endif

    int ret;

    if (type != file->type)
    {
        ret = dsu_fsync(file);
    }

    file->used = 0;
    file->type = type;
    return ret;
}

/* find the oldest file in the specified folder */
char *dsu_find_oldfile(char const *path, char **exlist, unsigned int ex_cnt)
{
    #if 0
    assert(path);
    #else

    if (NULL == path)
    {
        log_e(LOG_DSU, "path is NULL");
        return NULL;
    }

    #endif
    DIR *dir;
    struct dirent *file;
    struct stat fst;
    char *ret = NULL;
    static char old_file[128];
    unsigned long long x = 0xFFFFFFFFFFFFFFFF, y;
    unsigned int reboot;
    unsigned int tmpIndex;
    unsigned int i, skip;

    if ((dir = opendir(path)) == NULL)
    {
        log_e(LOG_DSU, "Open dir error:%s, path;%s", strerror(errno), path);
        return ret;
    }

    memset(old_file, 0, sizeof(old_file));

    while ((file = readdir(dir)) != NULL)
    {
        if ((stat(file->d_name, &fst) >= 0) && S_ISDIR(fst.st_mode))
        {
            continue;
        }

        if (strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0)
        {
            continue;
        }

        if (ex_cnt)
        {
            skip = 0;

            for (i = 0; i < ex_cnt; i++)
            {
                if (strstr(exlist[i], file->d_name)) // Skip the read-write file
                {
                    log_e(LOG_DSU, "file is opened,skip:%s!", exlist[i]);
                    skip = 1;
                    break;
                }
            }

            if (skip)
            {
                continue;
            }
        }

        if (2 != sscanf(file->d_name, "%*[^(](%u_%u)%*[^)]", &reboot, &tmpIndex))
        {
            snprintf(old_file, sizeof(old_file), "%s/%s", path, file->d_name);
            ret = old_file;
            closedir(dir);
            return ret;
        }

        y = reboot * 10000 + tmpIndex;

        {
            if (y < x)
            {
                x = y;
                snprintf(old_file, sizeof(old_file), "%s/%s", path, file->d_name);
                ret = old_file;
            }
        }

    }

    closedir(dir);

    return ret;
}

