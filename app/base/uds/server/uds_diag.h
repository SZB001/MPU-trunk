#ifndef __UDS_DIAG_H__
#define __UDS_DIAG_H__

#include "uds_diag_item_def.h"
#include <pthread.h>


typedef int (*diag_get_did_value)(unsigned char *did, unsigned int len);
typedef int (*diag_set_did_value)(unsigned char *did, unsigned int len);

typedef int (*diag_fun)(void);

#define DIAG_MAX_DID_CNT           (17)     //FT
#define DIAG_MAX_ITEM_LEN          (128)
#define DIAG_DID_NAME_LEN          (32)
#define DIAG_ITEM_BUF_LEN          (1024)

#define DID_INVALID                (0xFFFF)

typedef enum DIAG_DATA_TYPE
{
    DIAG_DATA_UINT = 0,
    DIAG_DATA_USHORT,
    DIAG_DATA_UCHAR,
    DIAG_DATA_STRING,
    DIAG_DATA_DOUBLE,
    DIAG_DATA_TIME,
    DIAG_DATA_INVALID = 0xff,
} DIAG_DATA_TYPE;

typedef struct DIAG_ITEM_DID
{
    char name[DIAG_DID_NAME_LEN];
    unsigned short      id;
    DIAG_DATA_TYPE      type;
    unsigned int        len;
    unsigned char       level;
    diag_get_did_value  get;
    diag_set_did_value  set;
} DIAG_ITEM_DID;

typedef struct
{
    unsigned char uds_diag_item_buf[DIAG_ITEM_BUF_LEN];
    pthread_mutex_t uds_diag_item_buf_mtx;
} UDS_DIAG_ITEM_BUF_T;

typedef struct DIAG_ITEM_INFO
{
    unsigned short itemid;
    char           dtc[7];   /* include '\0' */
    char           *name;
    diag_fun       fun;
    unsigned int   thsd;     /* threshold */
    unsigned int   len;
    unsigned int   offset;
    unsigned int   counter;
    unsigned int   *confirmed;    /* freeze frame is valid when the confirmed flag is true, */
    unsigned short freeze[DIAG_MAX_DID_CNT]; /* the did index which is contained in the freeze frame*/
    pthread_mutex_t diag_item_info_mtx;
} DIAG_ITEM_INFO;

#pragma pack(1)
typedef struct
{
    unsigned char  sec;    /* Second value - [0,59] */
    unsigned char  min;    /* Minute value - [0,59] */
    unsigned char  hour;   /* Hour value - [0,23] */
    unsigned char  mday;   /* Day of the month value - [1,31] */
    unsigned char  mon;    /* Month value - [1,12] */
    unsigned char  year;   /* Year value - [0,4095] */
} DIAG_RTCTIME;
#pragma pack(0)

#define DIAG_TABLE_BEGIN()              DIAG_ITEM_INFO diag_table[] = {
#define DIAG_TABLE_END()                };

#define DIAG_ITEM_BEGIN()               {
#define DIAG_ITEM_END()                 },
#define DIAG_ITEM_ID(id)                (id),
#define DIAG_ITEM_DTC(dtc)              (dtc),
#define DIAG_ITEM_NAME(name)            (name),
#define DIAG_ITEM_FUN(fun)              (fun),
#define DIAG_ITEM_THSD(thsd)            (thsd),0,0,0,NULL,

#define DIAG_FREEZE_BEGIN()             {
#define DIAG_FREEZE_END()               DID_INVALID },
#define DIAG_DID(id)                    (id),

#define DIAG_DID_TABLE_BEGIN()          DIAG_ITEM_DID diag_did_table[] = {
#define DIAG_DID_TABLE_END()            };
#define DIAG_DID_ATTR(name, id, type, len, level, get, set)  {(name), (id), (type), (len), (level), (get), (set)},



/*==========================================================*/
int uds_diag_init(void);
void uds_diag_all_devices(void);
void uds_diag_devices(int startno, int endno);


unsigned int uds_diag_get_dtc_num(void);
unsigned int uds_diag_get_cfm_dtc_num(void);
unsigned int uds_diag_get_uncfm_dtc_num(void);
unsigned int uds_diag_is_cfm_dtc_valid(DIAG_DEF_ITEM_ID id);
unsigned int uds_diag_is_uncfm_dtc_valid(DIAG_DEF_ITEM_ID id);
unsigned char *uds_diag_get_dtc(DIAG_DEF_ITEM_ID id);
unsigned char *uds_diag_get_dtc_name(DIAG_DEF_ITEM_ID id);
int uds_diag_get_did_value(unsigned short did, unsigned char *value, unsigned int *len);
int uds_diag_set_did_value(unsigned short did, unsigned char *value, unsigned int len);
int uds_diag_get_freeze(DIAG_DEF_ITEM_ID id, unsigned char *freeze, unsigned int *len,
                        unsigned int *did_num);
void uds_diag_dtc_clear(void);
int is_uds_diag_set_did_invalue(unsigned short did, unsigned char *value, unsigned int len);

#endif
