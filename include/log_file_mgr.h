#ifndef _LOG_FILR_MGR_H_
#define _LOG_FILR_MGR_H_

#define LOG_DIR_DISK_SIZE             (300)   // 300M

#define LOG_DIR_MAX_SIZE   (300*1024*1024)   //300M
#define LOG_FILE_MAX_SIZE  10*1024*1024    //10M
#define LOG_FILE_MAX_NUM   LOG_DIR_MAX_SIZE/LOG_FILE_MAX_SIZE   //30

/* disk status */
enum
{
    LOG_DISK_OK = 1,
    LOG_DISK_FULL,
    LOG_DISK_ERROR,
};


int log_disk_check(int mode);
int log_dir_check(void);
char *log_find_oldfile(char const *path);
void log_do_diskfull(void);
void log_dir_do_full(void);
void log_file_do_full(void);


#endif


