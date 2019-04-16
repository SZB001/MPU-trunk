/**
 * @Title: dsu_file.h
 * @author yuzhimin
 * @date Nov 8, 2017
 * @version V1.0
 */
#ifndef __DSU_FILE_H__
#define __DSU_FILE_H__

#include "dsu_comm.h"

typedef int (*Process)(void);

typedef struct _dsu_file_t
{
    FILE *fp;
    unsigned char *fbuf;        // raw data buffer
    unsigned int size;
    unsigned int used;
    unsigned char type;                  // see DSU_DATA_TYPE
    volatile unsigned char stat;         // see DSU_FILE_STAT,need mutex protection
    RTCTIME c_time;                      // time at file creation.
    char name[128];
    Process create;
    Process write;
    Process sync;
    Process close;
} DSU_FILE;

enum DSU_DATA_TYPE
{
    RAW_DATA = 0,
    ZLIB_DATA,
};

enum DSU_FILE_STAT
{
    FILE_ST_NONE = 0,
    FILE_ST_CREATING,
    FILE_ST_OPEN,
};

enum DSU_FILE_OPT
{
    FILE_OPT_CREATE = 0,
    FILE_OPT_WRITE,
    FILE_OPT_SYNC,
    FILE_OPT_CLOSE,
};

void dsu_compress_init(void);
int dsu_set_opt(DSU_FILE *file, DSU_FILE_TYPE type, unsigned char opt);
unsigned char dsu_get_stat(DSU_FILE *file);
int dsu_dir_check(void);
int dsu_file_init(DSU_FILE *file, unsigned char *buffer, unsigned int size, unsigned char type);
int dsu_fcreate(DSU_FILE *file, unsigned char type, const char *path, const char *mode);
int dsu_change_fb_type(DSU_FILE *file, unsigned char type);
int dsu_fb_used(DSU_FILE *file);
int dsu_write_fb(DSU_FILE *file, unsigned char *data, unsigned int len);      // write to fbuf
int dsu_fsync(DSU_FILE *file);
int dsu_fclose(DSU_FILE *file);
char *dsu_find_oldfile(char const *path, char **exlist, unsigned int ex_cnt);
int _dsu_fwrite(FILE *fp, unsigned char *buf, unsigned int len);
int dsu_disk_check(int mode); // mode 1:check now; mode 0:check result from device status
#endif /* __DSU_FILE_H__ */

