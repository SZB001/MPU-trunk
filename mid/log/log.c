/****************************************************************
file:         log.c
description:  the source file of log implementation
date:         2016/9/28
author        liuzhongwen
****************************************************************/
#include <stdio.h>
#include <memory.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include "init.h"
#include "log.h"
#include "log_file_mgr.h"
#include "file.h"

typedef struct
{
    int  level;
    char name[32];
} log_info_t;

log_info_t log_cfg_table[LOG_MAX] =
{
    { LOG_ERROR, "MAIN"     },
    { LOG_ERROR, "CFG"      },
    { LOG_ERROR, "DEV"      },
    { LOG_ERROR, "SHELL"    },
    { LOG_ERROR, "NM"       },
    { LOG_ERROR, "AT"       },
    { LOG_ERROR, "TCOM"     },
    { LOG_ERROR, "MID"      },
    { LOG_ERROR, "APPL"     },
    { LOG_ERROR, "SCOM"     },
    { LOG_ERROR, "PM"       },
    { LOG_ERROR, "GPS"      },
    { LOG_ERROR, "CAN"      },
    { LOG_ERROR, "UDS"      },
    { LOG_ERROR, "SOCK"     },
    { LOG_ERROR, "ASSIST"   },
    { LOG_ERROR, "GB32960"  },
    { LOG_ERROR, "DSU"      },
    { LOG_ERROR, "FCT"      },
    { LOG_ERROR, "FTP"      },
    { LOG_ERROR, "FOTON"    },
	{ LOG_ERROR, "FOTA"     },
    { LOG_ERROR, "FOTONHU"  },
    { LOG_ERROR, "AUTO" 	}
};

/*********************************************
function:     log_set_level
description:  LOG_ID id, id
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*********************************************/
int log_set_level(LOG_ID id, int level)
{
    if (id >= LOG_MAX)
    {
        return -1;
    }

    if ((level > LOG_DEBUG) || (level < LOG_ERROR))
    {
        return -1;
    }

    log_cfg_table[id].level = level;

    return 0;
}

/*********************************************
function:     log_get_level
description:  get log level
input:        LOG_ID id, id
output:       none
return:       none
*********************************************/
int log_get_level(LOG_ID id)
{
    return log_cfg_table[id].level;
}


/****************************************************************
 function:     log_get_id_by_name
 description:  get id by module name
 input:        unsigned char *name, module name;
 output:       none
 return:       0 indicates success;
 others indicates failed
 ****************************************************************/
int log_get_id(const char *name)
{
    int i;

    for (i = 0; i < LOG_MAX && strcmp(name, log_cfg_table[i].name) != 0; i++)
    {
    }

    return i < LOG_MAX ? i : -1;
}

/*********************************************
function:     log_get_name
description:  get log level
input:        LOG_ID id, id
output:       none
return:       log name addr
*********************************************/
char *log_get_name(LOG_ID id)
{
    return log_cfg_table[id].name;
}


#define LOG_BUF_SIZE    1024
#define offset(t, m)    ((int) &((t *)0)->m)

typedef struct
{
    int size;
    int colr;
    char buff[LOG_BUF_SIZE];
} __attribute__((packed)) log_msg_t ;

static char log_buf[64 * 1024];
static unsigned int log_get, log_put;
static pthread_mutex_t log_mtx1;
static pthread_mutex_t log_mtx2;
static pthread_mutex_t log_mtx3;
static int log_save;
static int log_file_index;
static FILE *log_file;
static const char *log_path;

void log_init(void)
{
    pthread_mutex_init(&log_mtx1, NULL);
    pthread_mutex_init(&log_mtx2, NULL);
    pthread_mutex_init(&log_mtx3, NULL);
}

void log_save_ctrl(int index, const char *path)
{
    assert(path != NULL);

    pthread_mutex_lock(&log_mtx3);
    log_save = index;
    log_path = path;

    if ((log_file != NULL) && (!log_save))
    {
        fclose(log_file);
        log_file = NULL;
    }

    pthread_mutex_unlock(&log_mtx3);
}

void log_sync(void)
{
    pthread_mutex_lock(&log_mtx3);

    if (log_file)
    {
        fflush(log_file);
    }

    pthread_mutex_unlock(&log_mtx3);
}

static FILE *log_file_open(void)
{
    char fname[64];
    FILE *logfile;
    unsigned char file_new_flag=0;
    char logmsg[100]="";
    unsigned char len;

    if (snprintf(fname, 64, "%s%s%d_%d", log_path, "log_", log_save,log_file_index) >= 64)
    {
        return NULL;
    }    

    if( !file_exists(fname) )
    {
        file_new_flag = 1;
	    len = sprintf(logmsg,"Create a new log file:%s\r\n",fname);	    
	}
	
    logfile = fopen(fname, "a+");
    
    if(logfile == NULL)
    {
        return NULL;
    }

    if(file_new_flag)
    {
        file_new_flag=0;

        if(!file_exists(fname))
        {                        
            fclose(logfile);
            return NULL;
        }
        else
        {
            printf(logmsg);
            if (fwrite(logmsg, 1, len, logfile) < len)
            {
                fclose(logfile);
                return NULL;
            }
            else
            {
                fflush(logfile);
            }
        }
    }

    return logfile;
    
}

int log_file_get_size(void)
{
    char fname[100]={0};
    snprintf(fname, 64, "%s%s%d_%d", log_path, "log_", log_save,log_file_index);
    return file_size(fname);
}

void log_file_cur_close(void)
{
    pthread_mutex_lock(&log_mtx3);
    if( log_file != NULL )
    {
        fclose(log_file);   
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mtx3);
}

void log_flie_index_add(void)
{
    pthread_mutex_lock(&log_mtx3);
    log_file_index++;
    pthread_mutex_unlock(&log_mtx3);
}
int log_file_cur_get_num(void)
{
    return  (log_save * 10000 + log_file_index);
}

static void log_flush(void)
{
    char fname[100]={0};
    const char *colr[] =
    {
        "\033[0m",
        "\033[47;31m",
        "\033[47;32m",
        "\033[47;35m",
        "\033[47;34m",
    };

    while (log_get != log_put)
    {
        if (sizeof(log_buf) - log_get < sizeof(log_msg_t))
        {
            log_get = 0;
        }
        else
        {
            log_msg_t *msg = (log_msg_t *)(log_buf + log_get);

            log_get += msg->size + offset(log_msg_t, buff) + 1;
            printf("%s%s", colr[msg->colr], msg->buff);
            pthread_mutex_lock(&log_mtx3);

            if (log_save)
            {
                if (log_file != NULL || (log_file = log_file_open()) != NULL)
                {
                    snprintf(fname, 64, "%s%s%d_%d", log_path, "log_", log_save,log_file_index);
                    if(!file_exists(fname))
                    {                        
                        fclose(log_file);
                        log_file = NULL;
                    }
                    else
                    {
                        if (fwrite(msg->buff, 1, msg->size, log_file) < msg->size)
                        {
                            fclose(log_file);
                            log_file = NULL;
                        }
                        else
                        {
                            fflush(log_file);
                        }
                    }
                }
            }
            else if (log_file != NULL)
            {
                fclose(log_file);
                log_file = NULL;
            }

            pthread_mutex_unlock(&log_mtx3);
        }
    }
}

static int log_push_data(uint8_t color, const uint8_t *data, int len)
{
    log_msg_t *msg;
    int size, out, cnt, i;

    if (sizeof(log_buf) - log_put < sizeof(log_msg_t))
    {
        if (log_get < sizeof(log_msg_t))
        {
            log_flush();
        }

        log_put = 0;
    }

    msg = (log_msg_t *)(log_buf + log_put);
    msg->colr = color;

    for (out = 0, size = 0; len > 0; len -= cnt, out += cnt)
    {
        cnt = len > 20 ? 20 : len;

        if (cnt * 3 + 2 >= LOG_BUF_SIZE - size)
        {
            break;
        }

        for (i = 0; i < cnt; i++)
        {
            size += sprintf(msg->buff + size, " %02X", data[out + i]);
        }

        size += sprintf(msg->buff + size, "\r\n");
    }

    msg->size = size;
    log_put += msg->size + offset(log_msg_t, buff) + 1;

    return out;
}


static void log_push_text(uint8_t color, const char *fmt, ...)
{
    log_msg_t *msg;
    va_list args;
    int size;

    if (sizeof(log_buf) - log_put < sizeof(log_msg_t))
    {
        if (log_get < sizeof(log_msg_t))
        {
            log_flush();
        }

        log_put = 0;
    }

    msg  = (log_msg_t *)(log_buf + log_put);
    msg->colr = color;
    va_start(args, fmt);
    size = vsnprintf(msg->buff, LOG_BUF_SIZE, fmt, args);
    va_end(args);
    msg->size = size < LOG_BUF_SIZE ? size : LOG_BUF_SIZE - 1;
    log_put += msg->size + offset(log_msg_t, buff) + 1;
}

int log_raw(int color, const char *fmt, va_list args)
{
    log_msg_t *msg;
    int size;

    pthread_mutex_lock(&log_mtx1);
    pthread_mutex_lock(&log_mtx2);
    pthread_mutex_unlock(&log_mtx1);

    if (sizeof(log_buf) - log_put < sizeof(log_msg_t))
    {
        if (log_get < sizeof(log_msg_t))
        {
            log_flush();
        }

        log_put = 0;
    }

    msg  = (log_msg_t *)(log_buf + log_put);
    msg->colr = color;

    if ((size = vsnprintf(msg->buff, LOG_BUF_SIZE, fmt, args)) >= LOG_BUF_SIZE)
    {
        size = LOG_BUF_SIZE;
    }

    msg->size = size;
    log_put += msg->size + offset(log_msg_t, buff) + 1;

    if (pthread_mutex_trylock(&log_mtx1) == 0)
    {
        pthread_mutex_unlock(&log_mtx1);
        log_flush();
    }

    pthread_mutex_unlock(&log_mtx2);
    return size;
}
#if 0
int log_raw(int color, const char *fmt, ...)
{
    log_msg_t *msg;
    va_list args;
    int size;

    pthread_mutex_lock(&log_mtx1);
    pthread_mutex_lock(&log_mtx2);
    pthread_mutex_unlock(&log_mtx1);

    if (sizeof(log_buf) - log_put < sizeof(log_msg_t))
    {
        if (log_get < sizeof(log_msg_t))
        {
            log_flush();
        }

        log_put = 0;
    }

    msg  = (log_msg_t *)(log_buf + log_put);
    msg->colr = color;
    va_start(args, fmt);
    size = vsnprintf(msg->buff, LOG_BUF_SIZE, fmt, args);
    va_end(args);
    msg->size = size < LOG_BUF_SIZE ? size : LOG_BUF_SIZE - 1;
    log_put += msg->size + offset(log_msg_t, buff) + 1;

    if (pthread_mutex_trylock(&log_mtx1) == 0)
    {
        pthread_mutex_unlock(&log_mtx1);
        log_flush();
    }

    pthread_mutex_unlock(&log_mtx2);
}
#endif
void log_msg(int id, int lv, const char *func, const char *fmt, ...)
{
    log_msg_t *msg;
    int size;

    if (log_cfg_table[id].level < lv)
    {
        return;
    }

    pthread_mutex_lock(&log_mtx1);
    pthread_mutex_lock(&log_mtx2);
    pthread_mutex_unlock(&log_mtx1);

    if (sizeof(log_buf) - log_put < sizeof(log_msg_t))
    {
        if (log_get < sizeof(log_msg_t))
        {
            log_flush();
        }

        log_put = 0;
    }

    msg  = (log_msg_t *)(log_buf + log_put);
    size = snprintf(msg->buff, LOG_BUF_SIZE, " %s>%s()", log_cfg_table[id].name, func);
    msg->colr = lv == LOG_ERROR ? LOG_COLOR_RED : LOG_COLOR_BLACK;

    if (size < LOG_BUF_SIZE - 1)
    {
        va_list args;

        va_start(args, fmt);
        size += vsnprintf(msg->buff + size, LOG_BUF_SIZE - size, fmt, args);
        va_end(args);
    }

    size += snprintf(msg->buff + size, LOG_BUF_SIZE - size, "\r\n");

    msg->size = size < LOG_BUF_SIZE - 1 ? size : LOG_BUF_SIZE - 1;
    log_put += msg->size + offset(log_msg_t, buff) + 1;

    if (pthread_mutex_trylock(&log_mtx1) == 0)
    {
        pthread_mutex_unlock(&log_mtx1);
        log_flush();
    }

    pthread_mutex_unlock(&log_mtx2);

}

void log_dump(int id, int lvl, int color, const char *title, const char *cover, const uint8_t *data,
              int len)
{
    int out;
    unsigned int tlen;

    if (log_cfg_table[id].level < lvl || len <= 0)
    {
        return;
    }

    pthread_mutex_lock(&log_mtx1);
    pthread_mutex_lock(&log_mtx2);
    pthread_mutex_unlock(&log_mtx1);

    tlen   = (strlen(title) + 1) / 2;

    log_push_text(color, " %-.*s%-*s%.*s\r\n", 29 - tlen, cover, tlen * 2, title, 30 - tlen, cover);

    for (out = 0; len > 0; len -= out, data += out)
    {
        out = log_push_data(color, data, len);
    }

    log_push_text(color, " ----------------------------END----------------------------\r\n");

    if (pthread_mutex_trylock(&log_mtx1) == 0)
    {
        pthread_mutex_unlock(&log_mtx1);
        log_flush();
    }

    pthread_mutex_unlock(&log_mtx2);
}


void log_level_show(void)
{
    int i;
    const char *lvlstr[] =
    {
        "ERROR",
        "WARNING",
        "INFORMATION",
        "DEBUG",
    };

    for (i = 0; i < LOG_MAX; i++)
    {
        log_push_text(LOG_COLOR_BLUE, " task %02d %-8s: %s\r\n", i, log_get_name(i),
                      lvlstr[log_get_level(i)]);
    }

    pthread_mutex_lock(&log_mtx1);
    log_flush();
    pthread_mutex_unlock(&log_mtx1);
}

#if 1
void dumphex(int id, int lvl, unsigned char *buf, int len, const char *funcname, int line)
{

    if (log_cfg_table[id].level < lvl || len <= 0)
    {
        return;
    }

    char membuf[1024] = {0};
    unsigned char *ptr = (unsigned char *) buf;
    char *ibuf = membuf;
    int i, j;
    int len1, len2;

    printf("\r\nDUMPHEX: func %s, line %d, len %d\r\n", funcname, line, len);
    len1 = len / 16 * 16;
    len2 = len % 16;
    ibuf[0] = 0;

#define BIN2CHAR(ch) (((ch) > ' ' && (ch) <= '~') ? (ch) : '.')

    for (i = 0; i < len1; i += 16)
    {
        ibuf = membuf;
        sprintf(ibuf, "0x%08X  ", (int) buf + i);
        ibuf += 12;

        for (j = 0; j < 16; j++)
        {
            sprintf(ibuf, "%02X ", ptr[j]);
            ibuf += 3;
        }

        sprintf(ibuf, "  ");
        ibuf += 2;

        for (j = 0; j < 16; j++)
        {
            sprintf(ibuf, "%c", BIN2CHAR(ptr[j]));
            ibuf++;
        }

        sprintf(ibuf, "\r\n");
        printf("%s", membuf);
        ptr += 16;
    }

    if (len2 == 0)
    {
        return;
    }

    ibuf = membuf;
    sprintf(ibuf, "0x%08X  ", (int) buf + ((len >> 4) << 4));
    ibuf += 12;

    for (i = 0; i < len2; i++)
    {
        sprintf(ibuf, "%02X ", ptr[i]);
        ibuf += 3;
    }

    for (i = len2; i < 16; i++)
    {
        sprintf(ibuf, "   ");
        ibuf += 3;
    }

    sprintf(ibuf, "  ");
    ibuf += 2;

    for (i = 0; i < len2; i++)
    {
        sprintf(ibuf, "%c", BIN2CHAR(ptr[i]));
        ibuf += 1;
    }

    sprintf(ibuf, "\r\n");

    printf("%s", membuf);
}
#endif

