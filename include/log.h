/****************************************************************
 file:         log.h
 description:  the header file of log api definition
 date:         2016/9/28
 author        liuzhongwen
 ****************************************************************/

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

/* log level definition */
enum LOG_LEVEL_TYPE
{
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG
};

typedef enum LOG_ID
{
    LOG_MAIN,
    LOG_CFG,
    LOG_DEV,
    LOG_SHELL,
    LOG_NM,
    LOG_AT,
    LOG_TCOM,
    LOG_MID,
    LOG_APPL,
    LOG_SCOM,
    LOG_PM,
    LOG_GPS,
    LOG_CAN,
    LOG_UDS,
    LOG_SOCK,
    LOG_ASSIST,
    LOG_GB32960,
    LOG_DSU,
    LOG_FCT,
    LOG_FTP,
    LOG_FOTON,
    LOG_FOTA,
    LOG_FOTONHU,
    LOG_AUTO,
	LOG_HOZON,
	LOG_SOCK_PROXY,
    LOG_MAX,
} LOG_ID;

enum
{
    LOG_COLOR_BLACK,
    LOG_COLOR_RED,
    LOG_COLOR_GREEN,
    LOG_COLOR_PURPLE,
    LOG_COLOR_BLUE,
};

#if 1
#define log_e(id, ...) \
    do\
    {\
        assert(id < LOG_MAX);\
        log_msg(id, LOG_ERROR, __FUNCTION__, __VA_ARGS__);\
    } while (0)
#else
#define log_e(id, ...)\
    do\
    {\
        assert(id < LOG_MAX);\
        printf("\033[47;31m %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
        printf(__VA_ARGS__);\
        printf("\033[0m\r\n");\
        if(save_log)\
        {\
            fprintf(sdsave, " %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
            fprintf(sdsave, __VA_ARGS__);\
            fprintf(sdsave, "\r\n");\
        }\
    } while(0)
#endif

#if 1
#define log_w(id, ...) \
    do\
    {\
        assert(id < LOG_MAX);\
        log_msg(id, LOG_WARNING, __FUNCTION__, __VA_ARGS__);\
    } while (0)
#else
#define log_w(id, ...)\
    do\
    {\
        assert(id < LOG_MAX);\
        if (log_cfg_table[id].level >= LOG_WARNING)\
        {\
            printf(" %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
            printf(__VA_ARGS__);\
            printf("\r\n");\
            if(save_log)\
            {\
                fprintf(sdsave, " %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
                fprintf(sdsave, __VA_ARGS__);\
                fprintf(sdsave, "\r\n");\
            }\
        }\
    } while(0)
#endif

#if 1
#define log_i(id, ...) \
    do\
    {\
        assert(id < LOG_MAX);\
        log_msg(id, LOG_INFO, __FUNCTION__, __VA_ARGS__);\
    } while (0)
#else
#define log_i(id, ...)\
    do\
    {\
        assert(id < LOG_MAX);\
        if(log_cfg_table[id].level >= LOG_INFO)\
        {\
            printf(" %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
            printf(__VA_ARGS__);\
            printf("\r\n");\
            if(save_log)\
            {\
                fprintf(sdsave, " %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
                fprintf(sdsave, __VA_ARGS__);\
                fprintf(sdsave, "\r\n");\
            }\
        }\
    } while(0)
#endif

#if 1
#define log_o(id, ...) \
    do\
    {\
        assert(id < LOG_MAX);\
        log_msg(id, -1, __FUNCTION__, __VA_ARGS__);\
    } while (0)
#else
#define log_o(id, ...)\
    do\
    {\
        assert(id < LOG_MAX);                   \
        printf(" %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
        printf(__VA_ARGS__);\
        printf("\r\n");\
        if(save_log)\
        {\
            fprintf(sdsave, " %s>%s()", log_cfg_table[id].name, __FUNCTION__);\
            fprintf(sdsave, __VA_ARGS__);\
            fprintf(sdsave, "\r\n");\
        }\
    } while(0)
#endif

#define log_buf_dump(id, tip, buf, len)\
    do\
    {\
        assert(id < LOG_MAX);\
        log_dump(id, LOG_INFO, LOG_COLOR_BLACK, tip, "******************************", buf, len);\
    } while (0)


extern void dumphex(int id, int lvl, unsigned char *buf, int len, const char *funcname, int line);

// only for debug print,don't save to the log file.
#define log_hex_dump(id, buf, len)\
    do\
    {\
        assert(id < LOG_MAX);\
        dumphex(id, LOG_INFO, buf, len, __FUNCTION__,__LINE__);\
    } while(0)


extern int log_set_level(LOG_ID id, int level);
extern int log_get_level(LOG_ID id);
extern char *log_get_name(LOG_ID id);
extern int log_get_id(const char *name);
extern void log_level_show(void);
extern void log_init(void);
extern void log_msg(int id, int lvl, const char *func, const char *fmt, ...);
extern int log_raw(int colr, const char *fmt, va_list args);
extern void log_dump(int id, int lvl, int colr, const char *title, const char *cover,
                     const uint8_t *data, int len);
extern void log_save_ctrl(int index, const char *path);
extern void log_sync(void);
extern int log_file_get_size(void);
extern void log_file_cur_close(void);
extern void log_flie_index_add(void);
extern int log_file_cur_get_num(void);


#endif

