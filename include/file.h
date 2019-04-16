
/****************************************************************
file:         file.h
description:  the header file of file operation definition
date:         2016/11/22
author        liuzhongwen
****************************************************************/

#ifndef __FILE_H__
#define __FILE_H__

#include <fcntl.h>
#include <fts.h>
#include "com_app_def.h"

typedef enum FILE_ERROR_CODE
{
    FILE_INVALID_PARAMETER = (MPU_MID_MID_FILE << 16) | 0x01,
    FILE_MAKE_FAILED,
    FILE_STAT_FAILED,
    FILE_REMOVE_FAILED,
    FILE_OPEN_FAILED,
    FILE_SET_ATTR_FAILED,
    FILE_INVALID_FILE_PATH,
    FILE_COPY_FAILED,
    FILE_READ_FAILED,
} FILE_ERROR_CODE;

/* checks whether the path refers to a file  */
bool file_exists(const char *path);

/* checks whether the block file is exist  */
bool bfile_exists(const char *path);

/* checks whether the path exist  */
bool path_exists(const char *path);

/* get the size of a file  */
int file_size(const char *path);

/* delete the file which the path refers to  */
void file_delete(const char *path);

/* rename the file which the old_path refers to with the new_path */
void file_rename(const char *old_path, const char *new_path);

/* write data to file atomically */
int file_write_atomic(const char *path, unsigned char *data, unsigned int len, mode_t mode);

/* copy file from src_path to dst_path */
int file_copy(const char *src_path, const char *dst_path);

/* write data and header to file atomically */
int file_update_atomic(const char *path, unsigned char *hdr, unsigned int hdr_len,
                       unsigned char *data, unsigned int data_len, mode_t mode);

/*open a file*/
int file_open_read(const char *path);

int file_read(const char *path, unsigned char *data, unsigned int *read_len);

int file_read_size(const char *path, unsigned char *data, unsigned int len);

int file_create(const char *path, mode_t mode);

int file_md5(const char *path, unsigned char *md5);
int file_comp(const char *file1, const char *file2);
bool file_isusing(const char *file);

#endif

