
/****************************************************************
file:         dir.h
description:  the header file of directory operation definition
date:         2016/11/22
author        liuzhongwen
****************************************************************/

#ifndef __DIR_H__
#define __DIR_H__

#include <fts.h>
#include "com_app_def.h"

typedef enum DIR_ERROR_CODE
{
    DIR_INVALID_PARAMETER = (MPU_MID_MID_DIR << 16) | 0x01,
    DIR_MAKE_FAILED,
    DIR_STAT_FAILED,
    DIR_REMOVE_FAILED,
    DIR_COPY_FAILED,
    DIR_OPEN_FAILED,
} DIR_ERROR_CODE;

/* checks whether the path refers to a directory */
bool dir_is_dir(const char *path);

/* checks whether the path exists */
bool dir_exists(const char *path);

/* checks whether the dir is empty */
bool dir_is_empty(const char *path);

/* make directory recursively which the path refers to a directory */
int dir_make_path(const char *path, mode_t mode, bool is_file);

/* remove directory recursively which the path refers to a directory */
int dir_remove_path(const char *path);

/* copy directory recursively from src_path refers to dst_path */
int dir_copy_path(const char *src_path, const char *dst_path);

/*read all the file and dir in basePath by recursive*/
int dir_read_list(const char *path, char *tbl, int entsz, int entno);

/* get directory size */
int dir_get_size(const char *path);

/* start to update  the files in the directory */
int dir_update_start(const char *dst_dir);

/* commmit the changes in the directory */
int dir_update_commit(const char *dst_dir);

/* get directory completeness status */
bool dir_get_status(const char *dir);

/* copy all the file in dir and ensure the dst_dir is integrated */
int dir_copy(const char *src_dir, const char *dst_dir);

#endif

