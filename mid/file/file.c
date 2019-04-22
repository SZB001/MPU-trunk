/****************************************************************
file:         file.c
description:  the source file of file operation implementation
date:         2016/11/22
author        liuzhongwen
****************************************************************/
#include <sys/sendfile.h>
#include "file.h"
#include "dir.h"
#include "md5.h"
#include "dev_rw.h"


/*****************************************************************************
function:     file_exists
description:  check whether the path refers to a file
input:        const char* path, the file path name.
output:       none
return:       true if the path refers to a file.
              false otherwise
*****************************************************************************/
bool file_exists(const char *path)
{
    struct stat file_status;

    /* use stat(2) to check for existence of the file. */
    if (stat(path, &file_status) != 0)
    {
        /* ENOENT indicates the file doesn't exist, anything else warrants and error report. */
        if (errno != ENOENT)
        {
            log_e(LOG_MID, "stat(%s) failed, error:%s", path, strerror(errno));
        }

        return false;
    }
    else
    {
        /* something exists. make sure it's a file. */
        /* NOTE: stat() follows symlinks. */
        if (S_ISREG(file_status.st_mode))
        {
            return true;
        }
        else
        {
            log_e(LOG_MID, "unexpected file system object type (%#o) at path '%s'.",
                  file_status.st_mode & S_IFMT, path);
            return false;
        }
    }
}

/*****************************************************************************
function:     bfile_exists
description:  check whether the path refers to a file
input:        const char* path, the file path name.
output:       none
return:       true if the path refers to a file.
              false otherwise
*****************************************************************************/
bool bfile_exists(const char *path)
{
    struct stat file_status;

    /* use stat(2) to check for existence of the file. */
    if (stat(path, &file_status) != 0)
    {
        /* ENOENT indicates the file doesn't exist, anything else warrants and error report. */
        if (errno != ENOENT)
        {
            log_e(LOG_MID, "stat(%s) failed, error:%s", path, strerror(errno));
        }

        return false;
    }
    else
    {
        /* something exists. make sure it's a file. */
        /* NOTE: stat() follows symlinks. */
        if (S_ISBLK(file_status.st_mode))
        {
            return true;
        }
        else
        {
            log_e(LOG_MID, "unexpected file system object type (%#o) at path '%s'.",
                  file_status.st_mode & S_IFMT, path);

            return false;
        }
    }
}

/*****************************************************************************
function:     path_exists
description:  check whether the path exist
input:        const char* path, the  path name.
output:       none
return:       true if the path exist
              false otherwise
*****************************************************************************/
bool path_exists(const char *path)
{
    struct stat file_status;

    /* use stat(2) to check for existence of the file. */
    if (stat(path, &file_status) != 0)
    {
        /* ENOENT indicates the file doesn't exist, anything else warrants and error report. */
        if (errno != ENOENT)
        {
            log_e(LOG_MID, "stat(%s) failed, error:%s", path, strerror(errno));
        }

        return false;
    }
    else
    {
        return true;
    }
}

/*****************************************************************************
function:     file_size
description:  get the file size
input:        const char* path, the file path name.
output:       none
return:       positive indicates the size of the file;
              negative indicates failed
*****************************************************************************/
int file_size(const char *path)
{
    struct stat file_status;

    /* use stat(2) to check for existence of the file. */
    if (stat(path, &file_status) != 0)
    {
        return -1;
    }
    else
    {
        return file_status.st_size;
    }
}


/*****************************************************************************
function:     file_delete
description:  delete the file which the path refers to
input:        const char* path, the file path name.
output:       none
return:       none
*****************************************************************************/
void file_delete(const char *path)
{
    if ((unlink(path) != 0) && (errno != ENOENT))
    {
        log_e(LOG_MID, "unlink(%s) failed, error:%s", path, strerror(errno));
    }
}

/*****************************************************************************
function:     file_rename
description:  rename the file which the old_path refers to with the new_path
input:        const char* old_path, the old file path name;
              const char* new_path, the new file path name.
output:       none
return:       none
*****************************************************************************/
void file_rename(const char *old_path, const char *new_path)
{
    /* if the file which new_path refers to is already exist,
       it will be deleted */
    if (rename(old_path, new_path) != 0)
    {
        log_e(LOG_MID, "rename(old:%s,new:%s) failed, error:%s",
              old_path, new_path, strerror(errno));
    }
}

/*****************************************************************************
function:     file_write_atomic
description:  atomically replace a file with another containing the data,
              file_path.new will be created with the contents of data,
              then renamed it to file_path
input:        const char* path, the file path name;
              unsigned char *data, the data which need to write into file;
              unsigned int len, data length;
              mode_t mode, access mode
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_write_atomic(const char *path, unsigned char *data, unsigned int len, mode_t mode)
{
    int fd, ret ;
    char temp_path[COM_APP_MAX_PATH_LEN] = {0};

    if (snprintf(temp_path, sizeof(temp_path), "%s.new", path) >= sizeof(temp_path))
    {
        log_e(LOG_MID, "file path '%s' is too long", path);
        return FILE_OPEN_FAILED;
    }

    /* create file_path.new */
    fd = open(temp_path, O_WRONLY | O_TRUNC | O_CREAT, mode);

    if (fd < 0)
    {
        log_e(LOG_MID, "unable to open file '%s' for writing", temp_path);
    }

    ret = dev_write(fd, data, len);
    fsync(fd);
    close(fd);

    if (ret != 0)
    {
        return ret;
    }

    file_rename(temp_path, path);

    return ret;
}

/*****************************************************************************
function:     file_update_atomic
description:  atomically replace a file with another containing the data,
              file_path.new will be created with the contents of data,
              then renamed it to file_path
input:        const char* path, the file path name;
              unsigned char *hdr, file header;
              unsigned int hdr_len, file header length;
              unsigned char *data, the data which need to write into file;
              unsigned int len, data length;
              mode_t mode, access mode
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_update_atomic(const char *path, unsigned char *hdr, unsigned int hdr_len,
                       unsigned char *data, unsigned int data_len, mode_t mode)
{
    int fd, ret;
    char temp_path[COM_APP_MAX_PATH_LEN] = {0};
    MD5_CTX  md5_ctx;

    if (snprintf(temp_path, sizeof(temp_path), "%s.new", path) >= sizeof(temp_path))
    {
        log_e(LOG_MID, "file path '%s' is too long", path);
        return FILE_OPEN_FAILED;
    }

    /* create file_path.new */
    fd = open(temp_path, O_WRONLY | O_TRUNC | O_CREAT, mode);

    if (fd < 0)
    {
        log_e(LOG_MID, "unable to open file '%s' for writing", temp_path);
    }

    /* write file header */
    ret = dev_write(fd, hdr, hdr_len);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    /* write file body */
    ret = dev_write(fd, data, data_len);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    /* compute md5 and write */
    MD5Init(&md5_ctx);
    MD5Update(&md5_ctx, hdr, hdr_len);
    MD5Update(&md5_ctx, data, data_len);
    MD5Final(&md5_ctx);

    ret = dev_write(fd, md5_ctx.digest, MD5_LENGTH);
    fsync(fd);
    close(fd);

    if (ret != 0)
    {
        return ret;
    }

    file_rename(temp_path, path);

    return ret;
}


/*****************************************************************************
function:     file_copy_attr
description:  copy file attribute
input:        const char* src_path, the source path name;
              const char* dst_path, the destination path name;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_copy_attr(const char *src_path, const char *dst_path)
{
    struct stat src_status;

    if (stat(src_path, &src_status) != 0)
    {
        log_e(LOG_MID, "stat(%s) failed, error:%s", src_path, strerror(errno));
        return FILE_STAT_FAILED;
    }

    /* set the owner of the dest file. */
    if (chown(dst_path, src_status.st_uid, src_status.st_gid) < 0)
    {
        log_e(LOG_MID, "chown(%s) failed, error:%s", dst_path, strerror(errno));
        return FILE_SET_ATTR_FAILED;
    }

    return 0;
}

/*****************************************************************************
function:     file_open_read
description:  open file for reading only
input:        const char* path, the file path name;
output:       none
return:       positive and 0 indicates success;
              negative indicates failed
*****************************************************************************/
int file_open_read(const char *path)
{
    int fd;

    do
    {
        fd = open(path, O_RDONLY);
    }
    while ((fd < 0) && (errno == EINTR));

    if (fd < 0)
    {
        log_e(LOG_MID, "error when opening file for reading, '%s', error:%s", path, strerror(errno));
    }

    return fd;
}

/*****************************************************************************
function:     file_create
description:  create file for for writing,
              if it exists, just open it for writing
input:        const char* path, the file path name;
              mode_t mode, access mode
output:       none
return:       positive and 0 indicates success;
              negative indicates failed
*****************************************************************************/
int file_create(const char *path, mode_t mode)
{
    int fd;

    do
    {
        fd = creat(path, mode);
    }
    while ((fd < 0) && (errno == EINTR));

    if (fd < 0)
    {
        log_e(LOG_MID, "error when create file for writing, '%s', error:%s ", path, strerror(errno));
    }

    return fd;
}

/*****************************************************************************
function:     file_copy
description:  copy file from src_path to dst_path
input:        const char* src_path, the source path name;
              const char* dst_path, the destination path name;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_copy(const char *src_path, const char *dst_path)
{
    int ret;
    int read_fd, write_fd;
    struct stat src_status;
    struct stat dst_status;
    ssize_t written_size = 0;
    off_t offset = 0;
    ssize_t written_ret = 0;

    /* the source file must be exist */
    if (stat(src_path, &src_status) != 0)
    {
        log_e(LOG_MID, "stat(%s) failed, error:%s", src_path, strerror(errno));
        return FILE_STAT_FAILED;
    }

    /* there's something there, but it's not a file or a symlink to a file. */
    if (!S_ISREG(src_status.st_mode))
    {
        log_e(LOG_MID, "invalid file path:%s", src_path);
        return FILE_INVALID_FILE_PATH;
    }

    /* if the destination file or path not exist, create it */
    if (stat(dst_path, &dst_status) != 0)
    {
        if (ENOENT == errno)
        {
            ret = dir_make_path(dst_path, src_status.st_mode, true);

            if (ret != 0)
            {
                log_e(LOG_MID, "make path(%s) failed", dst_path);
                return FILE_MAKE_FAILED;
            }
        }
        else
        {
            log_e(LOG_MID, "stat(%s) failed, error:%s", dst_path, strerror(errno));
            return FILE_STAT_FAILED;
        }
    }

    read_fd = file_open_read(src_path);

    if (read_fd < 0)
    {
        log_e(LOG_MID, "open src file(%s) failed", src_path);
        return FILE_STAT_FAILED;
    }

    write_fd = file_create(dst_path, src_status.st_mode);

    if (write_fd < 0)
    {
        log_e(LOG_MID, "open dst file(%s) failed", dst_path);
        close(read_fd);
        return FILE_STAT_FAILED;
    }

    ret = file_copy_attr(src_path, dst_path);

    if (ret != 0)
    {
        close(read_fd);
        close(write_fd);
        return ret;
    }

    while (written_size < src_status.st_size)
    {
        written_ret = sendfile(write_fd, read_fd, &offset,
                               src_status.st_size - written_size);

        if (written_ret < 0)
        {
            log_e(LOG_MID, "error when copying file '%s' to '%s', error:%s", src_path, dst_path,
                  strerror(errno));
            break;
        }

        written_size += written_ret;
    }

    close(read_fd);

    /* sync the file into flash */
    fsync(write_fd);
    close(write_fd);

    if (written_ret < 0)
    {
        return FILE_COPY_FAILED;
    }

    return 0;
}

/*****************************************************************************
function:     file_read
description:  read data from file
input:        const char *path, file path;
              the expext data length to read;
output:       unsigned char *data, data buffer;
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_read(const char *path, unsigned char *data, unsigned int *read_len)
{
    int size, fd, ret;

    size = file_size(path);

    if (size < 0)
    {
        log_e(LOG_MID, "get file(%s) size failed,error:%s", path, strerror(errno));
        return FILE_STAT_FAILED;
    }

    if (*read_len < size)
    {
        size = *read_len;
    }

    fd = file_open_read(path);

    if (fd < 0)
    {
        log_e(LOG_MID, "invalid file, path:%s, fd:%d", path, fd);
        return FILE_OPEN_FAILED;
    }

    ret = dev_read(fd, data, size);

    if (ret != 0)
    {
        log_e(LOG_MID, "invalid md5 file, path:%s, ret:0x%08x", path, ret);
    }
    else
    {
        *read_len = size;
    }

    close(fd);

    return ret;
}

/*****************************************************************************
function:     file_read_line
description:  read data from file
input:        const char *path, file path;
              the expext data length to read;
output:       unsigned char *data, data buffer;
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_read_size(const char *path, unsigned char *data, unsigned int len)
{
    int  fd, ret;

    fd = file_open_read(path);

    if (fd < 0)
    {
        log_e(LOG_MID, "invalid file, path:%s, fd:%d", path, fd);
        return FILE_OPEN_FAILED;
    }

    ret = dev_read(fd, data, len);

    if (ret != 0)
    {
        log_e(LOG_MID, "read file failed, path:%s, ret:0x%08x", path, ret);
    }

    close(fd);

    return ret;
}

/*****************************************************************************
function:     file_md5
description:  compute the md5 of the file
input:        const char *path, file path;
output:       unsigned char *md5, the buffer to save md5 value
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_md5(const char *path, unsigned char *md5)
{
    int size, fd, ret, len = 0;
    MD5_CTX  md5_ctx;
    unsigned char buf[1024] = {0};

    if (!file_exists(path))
    {
        log_e(LOG_MID, "file is not exist, path:%s", path);
        return FILE_INVALID_FILE_PATH;
    }

    fd = file_open_read(path);

    if (fd < 0)
    {
        log_e(LOG_MID, "invalid path, path:%s, fd:%d", path, fd);
        return FILE_OPEN_FAILED;
    }

    /* compute md5 */
    MD5Init(&md5_ctx);

    size = file_size(path);

    while (len < size)
    {
        ret = read(fd, buf, sizeof(buf));

        if (ret > 0)
        {
            MD5Update(&md5_ctx, buf, (unsigned int)ret);
            len = len + ret;
        }
        else
        {
            MD5Final(&md5_ctx);
            log_e(LOG_MID, "read ret:%s,path:%s", strerror(errno), path);
            close(fd);
            return FILE_READ_FAILED;
        }
    }

    close(fd);
    MD5Final(&md5_ctx);

    memcpy(md5, md5_ctx.digest, MD5_LENGTH);

    return 0;
}

int file_comp(const char *file1, const char *file2)
{
    unsigned char md5_1[16], md5_2[16];

    return file_md5(file1, md5_1) || file_md5(file2, md5_2) || memcmp(md5_1, md5_2, 16);
}

bool file_isusing(const char *file)
{
    FILE *ptream;
    char cmd[280] = {0};
    char buff[128] = {0};
    int len;
    memset(cmd, 0, sizeof(cmd));
    memset(buff, 0, sizeof(buff));
    snprintf(cmd, sizeof(cmd), "fuser '%s'", file);
    ptream = popen(cmd, "r");

    if (!ptream)
    {
        log_e(LOG_MID, "popen failed:%s", strerror(errno));
        return false;
    }

    len = fread(buff, sizeof(char), sizeof(buff), ptream);
    pclose(ptream);

    if (len)
    {
        log_e(LOG_MID, "file is using by:%s", buff);
        return true;
    }
    else
    {
        log_i(LOG_MID, "file is not using");
        return false;
    }
}

