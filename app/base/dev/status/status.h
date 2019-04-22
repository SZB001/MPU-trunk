#ifndef __STATUS_H__
#define __STATUS_H__
#include "dev_api.h"

#define ST_ITEM_NAME_LEN         (32)
#define ST_ITEM_MAX_MEMBER_CNT   (6)
#define ST_ITEM_BUF_LEN          (1024)
#define ST_MAX_REG_ITEM_NUM      (32)
#define ST_MAX_STATUS_LEN        (32)

typedef struct ST_REG_ITEM
{
    ST_DEF_ITEM_ID id;
    st_on_changed onchanged;
} ST_REG_ITEM;

typedef struct ST_REG_TBL
{
    unsigned short used_num;
    ST_REG_ITEM sttbl[ST_MAX_REG_ITEM_NUM];
} ST_REG_TBL;

typedef enum ST_DATA_TYPE
{
    ST_DATA_UINT = 0,
    ST_DATA_INT,
    ST_DATA_USHORT,
    ST_DATA_UCHAR,
    ST_DATA_STRING,
    ST_DATA_DOUBLE,
    ST_DATA_INVALID = 0xff,
} ST_DATA_TYPE;

typedef struct ST_ITEM_TYPE
{
    char name[ST_ITEM_NAME_LEN];
    ST_DATA_TYPE type;
    unsigned int len;
} ST_ITEM_TYPE;

typedef struct ST_ITEM_INFO
{
    unsigned short itemid;
    unsigned int len;
    unsigned int offset;
    unsigned int is_valid;
    ST_ITEM_TYPE member[ST_ITEM_MAX_MEMBER_CNT];
} ST_ITEM_INFO;

#define ST_TABLE_BEGIN()            ST_ITEM_INFO st_table[] = {
#define ST_TABLE_END()              };

#define ST_ITEM_BEGIN()             {
#define ST_ITEM_END()               },
#define ST_ITEM_ID(id)              (id), 0, 0, 0,

#define ST_MEMBER_BEGIN()           {
#define ST_MEMBER_END()             {"", ST_DATA_INVALID, 0}},
#define ST_MEMBER(name, type, len)  {(name),(type), (len)},

int st_init(void);
int st_dump(int argc, const char **argv);

#endif

