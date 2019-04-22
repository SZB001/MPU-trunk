/**
 * @Title: dsu_main.h
 * @author yuzhimin
 * @date Nov 21, 2017
 * @version V1.0
 */
#ifndef __DSU_MAIN_H__
#define __DSU_MAIN_H__

#include "dsu_comm.h"
#include "dsu_file.h"
#include "dsu_inxfile.h"
#include "dsu_iwdfile.h"
#include "dsu_iwdzfile.h"

extern unsigned int canbus_stat;
extern DSU_CFG_T dsu_cfg;
extern DSU_FILE inx_file;
extern INX_ATTR inx_attr;
extern DSU_FILE iwd_file;
extern IWD_ATTR iwd_attr;
extern IWD_FAULT_INFO_T *iwd_fault_inf;
extern IWD_FAULT_INFO_T  iwd_fault_infmem[2];
extern DSU_FILE iwdz_file;
extern IWDZ_ATTR iwdz_attr;
extern int tmp_max_len[3];
extern pthread_mutex_t dsu_mutex;

void dsu_get_cantag_time(RTCTIME *time);
unsigned int dsu_get_cantag_cnt(void);
int dsu_shell_init(void);

#define DSU_LOCK()   do{pthread_mutex_lock(&dsu_mutex);}while(0)
#define DSU_UNLOCK() do{pthread_mutex_unlock(&dsu_mutex);}while(0)


#endif /* __DSU_MAIN_H__ */

