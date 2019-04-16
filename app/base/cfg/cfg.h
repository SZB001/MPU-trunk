
/****************************************************************
file:         cfg.h
description:  the header file of configuration manager definition
date:         2016/9/25
author        liuzhongwen
****************************************************************/

#ifndef __CFG_H__
#define __CFG_H__

#include "cfg_api.h"

#define CFG_MAX_REG_ITEM_NUM    (128)

typedef struct CFG_REG_ITEM
{
    CFG_PARA_ITEM_ID id;
    on_changed onchanged;
} CFG_REG_ITEM;

typedef struct CFG_REG_TBL
{
    unsigned short used_num;
    CFG_REG_ITEM   cfgtbl[CFG_MAX_REG_ITEM_NUM];
} CFG_REG_TBL;

#endif
