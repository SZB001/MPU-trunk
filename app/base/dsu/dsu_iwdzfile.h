/**
 * @Title: dsu_iwdzfile.h
 * @author yuzhimin
 * @date Nov 9, 2017
 * @version V1.0
 */
#ifndef __DSU_IWDZFILE_H__
#define __DSU_IWDZFILE_H__

#include "dsu_comm.h"

/* iwdz status structure */
typedef struct
{
    unsigned char enable;
    unsigned char hourfile;
    unsigned char reserve[2];
} IWDZ_ATTR;

int iwdz_file_init(void);
int iwdz_file_append(unsigned int type, unsigned char *data, unsigned int len);
void iwdz_attr_init(DSU_CFG_T *cfg);

#endif /* __DSU_IWDZFILE_H__ */

