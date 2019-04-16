
/****************************************************************
file:         cfg_para.h
description:  the header file of configuration parameter manager definition
date:         2016/9/25
author        liuzhongwen
****************************************************************/

#ifndef __CFG_PARA_H__
#define __CFG_PARA_H__

#include "cfg_api.h"

#define CFG_PARA_MAX_MEMBER_CNT   (6)
#define CFG_MAX_PARA_LEN          (512)
#define CFG_PARA_NAME_LEN         (32)
#define CFG_PARA_BUF_LEN          (5*1024)

#define CFG_MAX_CFG_CB_NUM        (8)

typedef enum CFG_DATA_TYPE
{
    CFG_DATA_UINT = 0,
    CFG_DATA_USHORT,
    CFG_DATA_UCHAR,
    CFG_DATA_STRING,
    CFG_DATA_DOUBLE,
    CFG_DATA_INVALID = 0xff,
} CFG_DATA_TYPE;

typedef enum CFG_RES_USED_ENUM
{
    CFG_RES_UNUSED = 0,
    CFG_RES_USED,
} CFG_RES_USED_ENUM;

typedef enum CFG_SET_TYPE
{
    CFG_SET_UNSILENT = 0,
    CFG_SET_SILENT   = 1,
} CFG_SET_TYPE;


typedef struct CFG_ITEM_TYPE
{
    char name[CFG_PARA_NAME_LEN];
    CFG_DATA_TYPE type;
    unsigned int len;
} CFG_ITEM_TYPE;

typedef struct CFG_ITEM_INFO
{
    unsigned short itemid;
    unsigned int len;
    unsigned int offset;
    CFG_ITEM_TYPE member[CFG_PARA_MAX_MEMBER_CNT];
} CFG_ITEM_INFO;


#define CFG_TABLE_BEGIN()      CFG_ITEM_INFO cfg_table[] = {
#define CFG_TABLE_END()              };

#define CFG_ITEM_BEGIN()             {
#define CFG_ITEM_END()               },
#define CFG_ITEM_ID(id)              (id), 0, 0,

#define CFG_MEMBER_BEGIN()           {
#define CFG_MEMBER_END()             {"", CFG_DATA_INVALID, 0}},
#define CFG_MEMBER(name, type, len)  {(name),(type), (len)},

int cfg_init_para(void);
int cfg_save_para(void);
int cfg_restore_para(void);
int cfg_dump_para(int argc, const char **argv);
int cfg_set_default(int argc, const char **argv);

#endif
