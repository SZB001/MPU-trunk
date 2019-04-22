/**
 * @Title: dsu_iwdfile.h
 * @author yuzhimin
 * @date Nov 21, 2017
 * @version V1.0
 */
#ifndef __DSU_IWDFILE_H__
#define __DSU_IWDFILE_H__

#include "dsu_comm.h"

#define IWD_FAULT_CH_MAX                256
#define IWD_LOGCAN_UNDEFINE             ((short)0xFEFE)

enum
{
    IWD_CONTINUE_REC = 0,
    IWD_REWIND_TMP_FILE,
    IWD_CLOSE_IWD_FILE,
};

/* iwd status structure */
typedef struct
{
    unsigned char   enable;
    /*
    stat: 0: 停止记录(tmp和iwd), 并创建新tmp,
          1: 移动tmp写指针到iwd头结尾处,
          2: 记录到iwd
    */
    unsigned char   stat;
    RTCTIME         cantag;
    short           countdown;      // 记录的倒计时时间S
    short           logtime;        // 配置文件 故障前后时间单位S
    int             fault_flag;     // 标记dbc中E01,E03的标记位
    long            offset;         // iwd头信息结尾距文件头的偏移
    pthread_mutex_t mutex;          // mutex for stat;
} IWD_ATTR;


typedef struct _iwd_fault_info_t
{
    int siglist[IWD_FAULT_CH_MAX];
    int cnt;
    struct _iwd_fault_info_t *next;
} IWD_FAULT_INFO_T;



int iwd_file_init(void);
int iwd_file_append(unsigned char *data, unsigned int len);
void iwd_attr_init(DSU_CFG_T *cfg);
void iwd_fault_ch_happen(int id, unsigned int uptime);
void iwd_fault_list_setting(IWD_FAULT_INFO_T *faultinf);
int iwd_fault_dbc_surfix(IWD_FAULT_INFO_T *faultinf, int sigid, const char *sfx);

#endif /*__IWDFILE_H__ */
