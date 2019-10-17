/****************************************************************
 file:         dir.c
 description:  the source file of directory operation implementation
 date:         2016/11/22
 author        liuzhongwen
 ****************************************************************/
#include "dir.h"
#include "file.h"
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

/**************************************************************************
 function:     dir_is_dir
 description:  checks whether the path refers to a directory
 input:        const char* path, the path name.
 output:       none
 return:       true if the path refers to a directory.
 false otherwise
 ***************************************************************************/
bool dir_is_dir(const char *path)
{
    struct stat path_stat;

    if ((NULL == path) || ('\0' == path[0]))
    {
        log_e(LOG_MID, "invalid para");
        return false;
    }

    if (stat(path, &path_stat) < 0)
    {
        return false;
    }

    return S_ISDIR(path_stat.st_mode);
}

bool dir_exists(const char *path)
{
    DIR *dir = NULL;

    if ((NULL == path) || ('\0' == path[0]))
    {
        log_e(LOG_MID, "invalid para");
        return false;
    }

    dir = opendir(path);

    if (NULL == dir)
    {
        return false;
    }
    else
    {
        closedir(dir);
        return true;
    }
}

/**************************************************************************
 function:     dir_is_empty
 description:  checks whether the dir is empty
 input:        const char* path, the path name.
 output:       none
 return:       true if  th path is not empty.
 false otherwise
 ***************************************************************************/
bool dir_is_empty(const char *path)
{
    int i = 0;
    DIR *dp;
    struct dirent *cur;

    if (!dir_is_dir(path))
    {
        log_e(LOG_MID, "the path is not a dir");
        return false;
    }

    dp = opendir(path);

    if (dp == NULL)
    {
        return false;
    }

    while ((cur = readdir(dp)) != NULL)
    {
        i++;
    }

    closedir(dp);

    if (i > 2)
    {
        return false;
    }
    else
    {
        return true;
    }
}

/**************************************************************************
 function:     dir_make
 description:  make a directory which the path refers to a directory
 input:        const char* path, the path name;
 mode_t mode, the access mode of the directory
 output:       none
 return:       0 indicates success;
 others indicates failed;
 **************************************************************************/
int dir_make(const char *path, mode_t mode)
{
    if ((NULL == path) || ('\0' == path[0]))
    {
        log_e(LOG_MID, "invalid para");
        return DIR_INVALID_PARAMETER;
    }

    if (mkdir(path, mode) < 0)
    {
        if (EEXIST == errno)
        {
            return 0;
        }

        log_e(LOG_MID, "mkdir(%s) failed, error:%s", path, strerror(errno));
        return DIR_MAKE_FAILED;
    }
    else
    {
        return 0;
    }
}

/**************************************************************************
 function:     dir_make_path
 description:  make directory recursively which the path refers to a directory
 the path must be absolute path
 input:        const char* path, the directory path name or a file path name;
 mode_t mode, the access mode of the directory;
 bool is_file, true indicates  a file path name,
 false indicates  a directory path name,
 output:       none
 return:       0 indicates success;
 others indicates failed;
 ***************************************************************************/
int dir_make_path(const char *path, mode_t mode, bool is_file)
{
    int ret, i;
    char rec_path[COM_APP_MAX_PATH_LEN];

    if ((NULL == path) || ('\0' == path[0]))
    {
        log_e(LOG_MID, "invalid para");
        return DIR_INVALID_PARAMETER;
    }

    /* the path must be absolute path */
    if (path[0] != '/')
    {
        log_e(LOG_MID, "the path must be absolute path");
        return DIR_INVALID_PARAMETER;
    }

    rec_path[0] = path[0];

    for (i = 1; path[i] != 0; i++)
    {
        if ('/' == path[i])
        {
            /* add '\0' and make the directory */
            rec_path[i] = 0;
            ret = dir_make(rec_path, mode);

            if (ret != 0)
            {
                return ret;
            }
        }

        rec_path[i] = path[i];
    }

    /* if is is a file path and the last path is a file name,
     not a directory, ignore it */
    if ((rec_path[i - 1] != '/') && (!is_file))
    {
        rec_path[i] = 0;
        ret = dir_make(rec_path, mode);

        if (ret != 0)
        {
            return ret;
        }
    }

    return 0;
}

/*****************************************************************************
 function:     dir_remove_path
 description:  remove directory recursively which the path refers to a directory
 input:        const char* path, the path name;
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************************/
int dir_remove_path(const char *path)
{
    FTS *fts;
    FTSENT *entry;
    struct stat path_stat;
    char *path_array[] = { (char *)path, NULL };
    int last_errno;

    /* check to see if we're dealing with a single file or a symlink to a directory.
     In either case, we simply have to delete the link or file. */
    if (lstat(path, &path_stat) == -1)
    {
        if (errno == ENOENT)  /* NO such file */
        {
            return 0;
        }
        else
        {
            log_e(LOG_MID, "lstat(%s) failed, error:%s", path, strerror(errno));
            return DIR_STAT_FAILED;
        }
    }
    else if ((S_ISLNK(path_stat.st_mode))     /* it is symlink */
             || (S_ISREG(path_stat.st_mode)))      /* it is file */
    {
        if (unlink(path) < 0)
        {
            log_e(LOG_MID, "unlink(%s) failed, error:%s", path, strerror(errno));
            return DIR_REMOVE_FAILED;
        }

        return 0;
    }

    /* open the directory tree to search. */
    errno = 0;
    fts = fts_open(path_array, FTS_PHYSICAL | FTS_NOSTAT, NULL);

    if (fts == NULL)
    {
        log_e(LOG_MID, "fts_open(%s) failed, error:%s", path, strerror(errno));
        return DIR_OPEN_FAILED;
    }

    /* step through the directory tree. */
    while ((entry = fts_read(fts)) != NULL)
    {
        switch (entry->fts_info)
        {
            case FTS_DP:
            case FTS_DNR:

                /* these are directories. */
                if (rmdir(entry->fts_accpath) != 0)
                {
                    log_e(LOG_MID, "fts_open(%s) failed, error:%s", entry->fts_accpath, strerror(errno));
                    fts_close(fts);
                    return DIR_REMOVE_FAILED;
                }

                break;

            case FTS_F:
            case FTS_NSOK:

                /* these are files. */
                if (remove(entry->fts_accpath) != 0)
                {
                    log_e(LOG_MID, "fts_open(%s) failed, error:%s", entry->fts_accpath, strerror(errno));
                    fts_close(fts);
                    return DIR_REMOVE_FAILED;
                }
        }
    }

    last_errno = errno;
    fts_close(fts);

    if (last_errno != 0)
    {
        return DIR_REMOVE_FAILED;
    }

    return 0;
}

/*****************************************************************************
 function:     dir_copy_path
 description:  copy directory recursively from src_path to dst_path
 input:        const char* src_path, the source path name;
 const char* dst_path, the destination path name;
 output:       none
 return:       0 indicates success;
 others indicates failed;
 *****************************************************************************/
int dir_copy_path(const char *src_path, const char *dst_path)
{
    size_t src_path_len = strlen(src_path);
    char *path_array[] = { (char *)src_path, NULL };
    FTS *fts;
    FTSENT *entry;
    struct stat src_stat;
    char newPath[COM_APP_MAX_PATH_LEN] = {0};
    int ret;

    if (!dir_is_dir(src_path))
    {
        log_e(LOG_MID, "src_path(%s) is not a directory", src_path);
        return DIR_INVALID_PARAMETER;
    }

    if (!dir_is_dir(dst_path))
    {
        log_e(LOG_MID, "dst_path(%s) is not a directory", dst_path);
        return DIR_INVALID_PARAMETER;
    }

    fts = fts_open(path_array, FTS_PHYSICAL, NULL);
    
    if (fts == NULL)
    {
        log_e(LOG_MID, "fts_open(%s) failed, error:%s", src_path, strerror(errno));
        return DIR_OPEN_FAILED;
    }
    while ((entry = fts_read(fts)) != NULL)
    {
        memset(newPath, 0, sizeof(newPath));
        strcat(newPath, dst_path);
        strcat(newPath, entry->fts_path + src_path_len);

        switch (entry->fts_info)
        {
            /* a directory, visited in pre-order, create it if it is not exist in dst_path. */
            case FTS_D:
                if (entry->fts_level > 0)
                {
                    if (stat(entry->fts_path, &src_stat) < 0)
                    {
                        log_e(LOG_MID, "stat(%s) failed, error:%s", entry->fts_path, strerror(errno));
                        fts_close(fts);
                        return DIR_STAT_FAILED;
                    }

                    ret = dir_make_path(newPath, src_stat.st_mode, false);

                    if (0 != ret)
                    {
                        log_e(LOG_MID, "make path(%s) failed, ret:0x%08x", newPath, ret);
                        fts_close(fts);
                        return ret;
                    }
                }

                break;

            /* a file, copy it to dst_path */
            case FTS_F:
                ret = file_copy(entry->fts_path, newPath);

                if (0 != ret)
                {
                    log_e(LOG_MID, "copy file(%s->%s) failed, ret:0x%08x", entry->fts_path, newPath, ret);
                    fts_close(fts);
                    return ret;
                }

                break;

            case FTS_DP:
            case FTS_DEFAULT:
            case FTS_SL:
            case FTS_SLNONE:
                break; /* do nothing on it */

            case FTS_DC: /* cyclic directory */
            case FTS_DNR: /* a directory which cannot be read */
            case FTS_NS: /* a file for which no stat information was available */
            case FTS_ERR:
                log_e(LOG_MID, "error reading file or directory information, '%s', error:%s",
                      entry->fts_path, strerror(entry->fts_errno));
                fts_close(fts);
                return DIR_COPY_FAILED;

            default:
                log_e(LOG_MID, "unexpected file type, %d, on file %s.",
                      entry->fts_info, entry->fts_path);
                fts_close(fts);
                return DIR_COPY_FAILED;
        }
    }
    fts_close(fts);
    return 0;
}

/*****************************************************************************
 function:     dir_copy_no_st
 description:  copy directory recursively from src_path to dst_path
 input:        const char* src_path, the source path name;
 const char* dst_path, the destination path name;
 output:       none
 return:       0 indicates success;
               others indicates failed;
 *****************************************************************************/
static int dir_copy_no_st(const char *src_path, const char *dst_path)
{
    size_t src_path_len = strlen(src_path);
    char *path_array[] = { (char *)src_path, NULL };
    FTS *fts;
    FTSENT *entry;
    struct stat src_stat;
    char newPath[COM_APP_MAX_PATH_LEN] = {0};
    char dst_path_st[COM_APP_MAX_PATH_LEN] = {0};
    int ret;

    if (!dir_is_dir(src_path))
    {
        log_e(LOG_MID, "src_path(%s) is not a directory", src_path);
        return DIR_INVALID_PARAMETER;
    }

    if (!dir_is_dir(dst_path))
    {
        log_e(LOG_MID, "dst_path(%s) is not a directory", dst_path);
        return DIR_INVALID_PARAMETER;
    }
    
    memcpy(dst_path_st, dst_path, strlen(dst_path));
    if('/' == dst_path[strlen(dst_path_st) - 1])
    {
        strcat(dst_path_st,"status.conf");
    }
    else
    {
        strcat(dst_path_st,"/status.conf");
    }

    fts = fts_open(path_array, FTS_PHYSICAL, NULL);
    
    if (fts == NULL)
    {
        log_e(LOG_MID, "fts_open(%s) failed, error:%s", src_path, strerror(errno));
        return DIR_OPEN_FAILED;
    }
    
    while ((entry = fts_read(fts)) != NULL)
    {
        memset(newPath, 0, sizeof(newPath));
        strcat(newPath, dst_path);
        strcat(newPath, entry->fts_path + src_path_len);

        if(0 == strcmp(newPath, dst_path_st))
        {
            log_i(LOG_MID,"newPath: %s, dst_path_st: %s", newPath, dst_path_st);
            continue;
        }

        switch (entry->fts_info)
        {
            /* a directory, visited in pre-order, create it if it is not exist in dst_path. */
            case FTS_D:
                if (entry->fts_level > 0)
                {
                    if (stat(entry->fts_path, &src_stat) < 0)
                    {
                        log_e(LOG_MID, "stat(%s) failed, error:%s", entry->fts_path, strerror(errno));
                        fts_close(fts);
                        return DIR_STAT_FAILED;
                    }

                    ret = dir_make_path(newPath, src_stat.st_mode, false);

                    if (0 != ret)
                    {
                        log_e(LOG_MID, "make path(%s) failed, ret:0x%08x", newPath, ret);
                        fts_close(fts);
                        return ret;
                    }
                }

                break;

            /* a file, copy it to dst_path */
            case FTS_F:
                ret = file_copy(entry->fts_path, newPath);

                if (0 != ret)
                {
                    log_e(LOG_MID, "copy file(%s->%s) failed, ret:0x%08x", entry->fts_path, newPath, ret);
                    fts_close(fts);
                    return ret;
                }

                break;

            case FTS_DP:
            case FTS_DEFAULT:
            case FTS_SL:
            case FTS_SLNONE:
                break; /* do nothing on it */

            case FTS_DC: /* cyclic directory */
            case FTS_DNR: /* a directory which cannot be read */
            case FTS_NS: /* a file for which no stat information was available */
            case FTS_ERR:
                log_e(LOG_MID, "error reading file or directory information, '%s', error:%s",
                      entry->fts_path, strerror(entry->fts_errno));
                fts_close(fts);
                return DIR_COPY_FAILED;

            default:
                log_e(LOG_MID, "unexpected file type, %d, on file %s.",
                      entry->fts_info, entry->fts_path);
                fts_close(fts);
                return DIR_COPY_FAILED;
        }
    }
    fts_close(fts);
    return 0;
}

/*****************************************************************************
 function   : dir_read_list
 description: read file name list from a directory
 input      : path  - directory path
              tbl   - table address for name list
              entsz - size of table entry
              entno - number of table entries
 output     : tbl   - table filled with name list
 return     : < 0   - error
              = 0   - no file in directory
              > 0   - number of names
 *****************************************************************************/
int dir_read_list(const char *path, char *tbl, int entsz, int entno)
{
    DIR *dir;
    struct dirent *dent;
    int cnt;
    char subpath[256];

    if ((dir = opendir(path)) == NULL)
    {
        log_e(LOG_MID, "open directory fail: %s", strerror(errno));
        return -1;
    }

    for (cnt = 0; (dent = readdir(dir)) != NULL && cnt < entno;)
    {
        if (dent->d_type == DT_REG)
        {
            if (strlen(path) + strlen(dent->d_name) + 1 < entsz)
            {
                struct stat fstat;
                struct tm *ftm;
                char info[64];

                strcpy(tbl, path);
                strcat(tbl, "/");
                strcat(tbl, dent->d_name);

                if (stat(tbl, &fstat) < 0)
                {
                    log_e(LOG_MID, "file %s stat error: %s", tbl, strerror(errno));
                    continue;
                }

                ftm = localtime(&fstat.st_mtime);
                sprintf(info, "\tC\t%lu\t%04d-%02d-%02d %02d:%02d:%02d",
                        fstat.st_size, ftm->tm_year + 1900, ftm->tm_mon + 1,
                        ftm->tm_mday, ftm->tm_hour, ftm->tm_min, ftm->tm_sec);

                if (strlen(tbl) + strlen(info) < entsz)
                {
                    strcat(tbl, info);
                    tbl += entsz;
                    cnt += 1;
                    continue;
                }
            }

            log_e(LOG_MID, "file name '%s/%s' is too long, it's ignored", path, dent->d_name);
        }
        else if (dent->d_type == DT_DIR && strcmp(dent->d_name, ".") != 0 &&
                 strcmp(dent->d_name, "..") != 0)
        {
            if (strlen(path) + strlen(dent->d_name) + 1 < sizeof(subpath))
            {
                int subcnt;

                strcpy(subpath, path);
                strcat(subpath, "/");
                strcat(subpath, dent->d_name);
                subcnt = dir_read_list(subpath, tbl, entsz, entno - cnt);
                cnt += subcnt;
                tbl += entsz * subcnt;
            }
            else
            {
                log_e(LOG_MID, "subpath '%s/%s' is too long, it's ignored", path, dent->d_name);
            }
        }
    }

    closedir(dir);
    return cnt;
}


/*****************************************************************************
 function:     dir_get_size
 description:  get the directory size
 input:        const char* path, the path name;
 output:       none
 return:       directory size;
 *****************************************************************************/
int dir_get_size(const char *path)
{
    DIR *dir;
    struct dirent *dent;
    char fullpath[256] = {0};
    int size = 0;
    struct stat fstat;

    if ((dir = opendir(path)) == NULL)
    {
        log_e(LOG_MID, "open directory fail: %s", strerror(errno));
        return -1;
    }

    for (dent = readdir(dir); dent != NULL; dent = readdir(dir))
    {
        if (dent->d_type == DT_REG)
        {
            if (strlen(path) + strlen(dent->d_name) + 1 < sizeof(fullpath))
            {
                memset(fullpath, 0, sizeof(fullpath));
                strcpy(fullpath, path);
                strcat(fullpath, "/");
                strcat(fullpath, dent->d_name);

                if (stat(fullpath, &fstat) < 0)
                {
                    log_e(LOG_MID, "file %s stat error: %s", fullpath, strerror(errno));
                    continue;
                }

                size += fstat.st_size;
            }
            else
            {
                log_e(LOG_MID, "file name '%s/%s' is too long, it's ignored", path, dent->d_name);
            }
        }
        else if (dent->d_type == DT_DIR && strcmp(dent->d_name, ".") != 0 &&
                 strcmp(dent->d_name, "..") != 0)
        {
            if (strlen(path) + strlen(dent->d_name) + 1 < sizeof(fullpath))
            {
                memset(fullpath, 0, sizeof(fullpath));
                strcpy(fullpath, path);
                strcat(fullpath, "/");
                strcat(fullpath, dent->d_name);
                size += dir_get_size(fullpath);
            }
            else
            {
                log_e(LOG_MID, "subpath '%s/%s' is too long, it's ignored", path, dent->d_name);
            }
        }
    }

    closedir(dir);
    return size;
}

/*****************************************************************************
 function:     dir_update_start
 description:  start to update  the files in the directory
 input:        const char *dst_dir, the dir name;
 output:       none
 return:       0 indicates sucess;
               others indicates failed
 *****************************************************************************/
int dir_update_start(const char *dst_dir)
{
    int ret;
    char status_file[COM_APP_MAX_PATH_LEN] = {0};

    memset(status_file, 0, sizeof(status_file));

    sprintf(status_file, "%s/%s", dst_dir, COM_APP_STATUS_FILE);

    ret = file_write_atomic(status_file, (unsigned char *)"bad",
                            strlen("bad"), S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret != 0)
    {
        log_e(LOG_MID, "write bad failed, file:%s, ret:0x%08x",
              status_file, ret);
    }

    return ret;
}

/*****************************************************************************
 function:     dir_update_commit
 description:  commmit the changes in the directory
 input:        const char *dst_dir, the dir name;
 output:       none
 return:       0 indicates sucess;
               others indicates failed
 *****************************************************************************/
int dir_update_commit(const char *dst_dir)
{
    int ret;
    char status_file[COM_APP_MAX_PATH_LEN] = {0};

    memset(status_file, 0, sizeof(status_file));

    sprintf(status_file, "%s/%s", dst_dir, COM_APP_STATUS_FILE);

    ret = file_write_atomic(status_file, (unsigned char *)"good",
                            strlen("good"), S_IRUSR | S_IWUSR | S_IXUSR);

    if (ret != 0)
    {
        log_e(LOG_MID, "write good failed, file:%s, ret:0x%08x",
              status_file, ret);
    }

    return ret;
}

/****************************************************************
function:     dir_get_status
description:  check the completeness of the directory
input:        const char *dir, directory path
output:       none
return:       true indicates the directory is good;
              false indicates bad
*****************************************************************/
bool dir_get_status(const char *dir)
{
    int ret = 0 ;
    unsigned int size;
    char data[16], status_file[COM_APP_MAX_PATH_LEN];

    /* if the directory not exist */
    if (!dir_is_dir(dir))
    {
        return false;
    }

    sprintf(status_file, "%s/%s", dir, COM_APP_STATUS_FILE);

    if (!file_exists(status_file))
    {
        return false;
    }

    memset(data, 0, sizeof(data));
    size = sizeof(data) - 1;  /* last char must be '\0' */

    ret = file_read(status_file, (unsigned char *)data, &size);

    if (ret != 0)
    {
        return false;
    }

    if (0 != strstr(data, "good"))
    {
        return true;
    }

    return false;
}

/****************************************************************
function:     dir_copy
description:  copy images from one directory to other directory
input:        const char *src_dir, source directory;
              const char *dst_dir, destination directory;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int dir_copy(const char *src_dir, const char *dst_dir)
{
    int ret;

    if( !dir_exists(src_dir) )
    {
        return 0;
    }

    if( dir_is_empty(src_dir) )
    {
        return 0;    
    }
    
    /* create dst directory */
    ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        return ret;
    }

    /* remove the files and directory in dst_dir */
    ret = dir_remove_path(dst_dir);

    if (ret != 0)
    {
        log_e(LOG_MID, "remove data dir failed,path:%s, ret:0x%08x",
              dst_dir, ret);
        return ret;
    }

    /* create dst data directory */
    ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        return ret;
    }

    ret = dir_update_start(dst_dir);

    if (ret != 0)
    {
        log_e(LOG_MID, "dir statuts start failed, path:%s, ret:0x%08x", dst_dir, ret);
        return ret;
    }

    /* copy the file and directory from src_dir to dst_dir */
    ret = dir_copy_no_st(src_dir, dst_dir);

    if (ret != 0)
    {
        log_e(LOG_MID, "copy images failed, src path:%s,dst path:%s, ret:0x%08x",
              src_dir, src_dir, ret);
        return ret;
    }
	
	ret = dir_update_commit(dst_dir);
	
	if (ret != 0)
    {
        log_e(LOG_MID, "dir statuts commit failed, path:%s, ret:0x%08x", dst_dir, ret);
        return ret;
    }

    return 0;
}


