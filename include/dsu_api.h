/**
 * @Title: dsu_api.h
 * @author yuzhimin
 * @date Nov 26, 2017
 * @version V1.0
 */
#ifndef __DSU_API_H__
#define __DSU_API_H__

#include "init.h"

int dsu_init(INIT_PHASE phase);
int dsu_run(void);
int dsu_suspend_record(void);
void dsu_resume_record(void);

unsigned int dsu_get_opened_filelist(char **list);


#endif /* __DSU_API_H__ */
