#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>
#include <dirent.h>
#include "log_file_mgr.h"
#include "diag.h"
#include "com_app_def.h"
#include "file.h"
#include "dir.h"

int log_disk_check(int mode)
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
        ret = LOG_DISK_OK;
    }
    else if (DIAG_EMMC_FULL == ret)
    {
        ret = LOG_DISK_FULL;
    }
    else
    {
        ret = LOG_DISK_ERROR;
    }

    return ret;
}


int log_dir_check(void)
{
    int ret;
    // check sd card or emmc dir
    ret = log_disk_check(1);

    if (LOG_DISK_OK != ret)
    {
        return ret;
    }

    if (!dir_exists(COM_LOG_DIR))
    {
        ret = dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);

        if (ret != 0)
        {
            log_e(LOG_DEV, "make LOG path error(%d):%s", ret, COM_LOG_DIR);
            return LOG_DISK_ERROR;
        }
    }

    return LOG_DISK_OK;
}


/* find the oldest file in the specified folder */
char *log_find_oldfile(char const *path)
{
    #if 0
    assert(path);
    #else

    if (NULL == path)
    {
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

    if ((dir = opendir(path)) == NULL)
    {
        log_e(LOG_DEV, "Open dir error:%s, path;%s", strerror(errno), path);
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

        if (2 != sscanf(file->d_name, "log_%d_%d", &reboot, &tmpIndex))  
        {
            snprintf(old_file, sizeof(old_file), "%s%s", path, file->d_name);  //删除名称不符文件
            ret = old_file;
            closedir(dir);
            return ret;
        }

        y = reboot * 10000 + tmpIndex;

        if( y > log_file_cur_get_num() )  //删除比当前文件号大的文件
        {
            snprintf(old_file, sizeof(old_file), "%s%s", path, file->d_name);
            ret = old_file;
            closedir(dir);
            return ret;
        }
        
        if (y < x)
        {
            x = y;
            snprintf(old_file, sizeof(old_file), "%s%s", path, file->d_name);
            ret = old_file;
        }
       

    }

    closedir(dir);

    return ret;
}

void log_do_diskfull(void)
{
    struct statfs diskfs;
    char *oldfile = NULL;
    unsigned long long block = 0;
    unsigned long long total = 0;
    unsigned long long avail = 0;  

    if ( !path_exists(COM_LOG_DIR) &&
        dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false) != 0)
    {
        log_e(LOG_DEV, "make log directory(%s) failed", COM_LOG_DIR);
        return ;
    }

    do
    {
        statfs(COM_SDCARD_DIR, &diskfs);
        block = diskfs.f_bsize;
        total = diskfs.f_blocks * block;
        avail = diskfs.f_bavail * block;
        log_i(LOG_DEV, "total:%llu B;avail:%llu B", total, avail);

        if ((avail >> 20) <= LOG_DIR_DISK_SIZE)
        {
            oldfile = log_find_oldfile(COM_LOG_DIR);
            if(oldfile == NULL)  //无文件
            {
                break;
            }

            log_o(LOG_DEV, "the disk of %s is full,so delete the early log file:%s\r\n", COM_SDCARD_DIR,oldfile);
            file_delete(oldfile);
            sync();
        }
        else
        {
            break;
        }
    }
    while (1);
}

void log_dir_do_full(void)
{
    char *oldfile = NULL;
    unsigned long long folder_size = 0; 

    if ( !path_exists(COM_LOG_DIR) &&
        dir_make_path(COM_LOG_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false) != 0)
    {
        log_e(LOG_DEV, "make log directory(%s) failed", COM_LOG_DIR);
        return ;
    }

    do
    {
        folder_size = (unsigned long long)dir_get_size(COM_LOG_DIR);

        if (folder_size > LOG_DIR_MAX_SIZE)
        {
            oldfile = log_find_oldfile(COM_LOG_DIR);
        }
        else
        {
            break;
        }

        log_o(LOG_DEV, "the size of log folder is larger than %dM, so delete the early log file:%s",(LOG_DIR_MAX_SIZE/1024)/1024, oldfile);
        file_delete(oldfile);
        sync();
    }
    while (1);
}

void log_file_do_full(void)
{
    int logfile_size = 0;    
   
    logfile_size = log_file_get_size();

    if (logfile_size >= LOG_FILE_MAX_SIZE)
    {
        log_o(LOG_DEV, "the size of current log file is larger than %dM, will create a new one", (LOG_FILE_MAX_SIZE/1024)/1024);
        log_file_cur_close();
        log_flie_index_add();
    }

}



