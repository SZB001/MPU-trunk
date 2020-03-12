#ifndef __CAN_API_H__
#define __CAN_API_H__
#include <stdint.h>
#include "com_app_def.h"
#include "list.h"
#include "init.h"
#include "timer.h"


typedef struct
{
    uint32_t uptime;
    uint32_t miscUptime;
    union
    {
        uint32_t MsgID; /* CAN Message ID (11-bit or 29-bit) */
        struct
        {
            uint32_t msgid : 31;
            uint32_t exten : 1;
        };
    };
    
    unsigned int  type  : 8;   /* can message or can Tag */
    unsigned int  len   : 7;   /* data length */
    unsigned int  brs   : 1;   /* bit rate switch */
    unsigned int  esi   : 1;   /* error state indicator */
    unsigned int  canFD : 1;   /* canFD flag */
    unsigned int  isRTR : 1;   /* RTR flag */
    unsigned int  isEID : 1;   /* EID flag */
    unsigned int  port  : 3;   /* can port */
    unsigned int  isRx  : 1;   /* tx or rx flag */ 
    unsigned int  res   : 8;
    union
    {
        uint32_t data32[2];
        uint8_t  Data[64];
    };
} __attribute__((packed)) CAN_MSG;

typedef struct
{
    unsigned int : 13;
    unsigned int PAD : 8;
    unsigned int DLC : 7;
    unsigned int BRS : 1;
    unsigned int canFD:1;
    unsigned int isRTR : 1;
    unsigned int isEID : 1;
    unsigned int MsgID;     /* CAN Message ID (11-bit or 29-bit) */
    unsigned char Data[64]; /* CAN Message Data Bytes 0-63 */
}CAN_SEND_MSG;

typedef struct
{
    unsigned char rtype; /* see also CAN_REPORT_TYPE */
    unsigned char port; /* CAN PORT */
    unsigned int val; /* the unit of interval is ms, the minimum precision is 50ms */
    unsigned int times; /* send times, 0: send until the stop */
    CAN_SEND_MSG msg;
}CAN_REPORT_CFG;


/* can call back event */
typedef enum
{
    CAN_EVENT_DATAIN = 1,   //p1:message buf(CAN_MSG*), p2:message count
    CAN_EVENT_TIMEOUT,      //p1:uptime
    CAN_EVENT_ACTIVE,       //no parameter
    CAN_EVENT_SLEEP,
    CAN_EVENT_WAKEUP
} CAN_EVENT_TYPE;

/* can dbc call back event */
typedef enum
{
    DBC_EVENT_SURFIX = 1,   //p1:signal id, p2:surfix(char*), r:surfix len
    DBC_EVENT_DEFINE,       //p1:dbc BA string(char*)
    DBC_EVENT_RELOAD,       //p1:dbc name(char*)
    DBC_EVENT_FINISHED,     //p1:0/1
    DBC_EVENT_UPDATE,       //p1:signal id, p2:uptime
    DBC_EVENT_RESET,         //p1:uptime
    DBC_EVENT_RCVED,        //p1:signal id, p2:uptime
} DBC_EVENT_TYPE;

/* dbc constant defination */
#define DBC_SIGNAME_LEN     64
#define DBC_ECUNAME_LEN     16
#define DBC_SIGUNIT_LEN     64
#define DBC_SUFFIX_LEN      16

/* can call back function format */
typedef int (*can_cb_t)(uint32_t, uint32_t, uint32_t);
/* dbc signal format */
typedef struct
{
    char     name[DBC_SIGNAME_LEN];
    char     unit[DBC_SIGUNIT_LEN];
    char     ecu[DBC_ECUNAME_LEN];
    char     suffix[DBC_SUFFIX_LEN];
    char     order;
    char     sign;
    char     start;
    char     size;
    double   factor;
    double   offset;
    double   min;
    double   max;
    double   init;
    double   lastv;
    double   value;
    uint32_t flags;
    uint32_t usrflags;
    uint32_t port;
    uint32_t midx;
    uint32_t mi[4];
    double   mv[4];
    list_t link;
} dbc_sig_t;

extern int can_init(INIT_PHASE phase);
extern int can_run(void);
extern int can_set_baud(int port, short baud);
extern short can_get_baud(int port);
extern uint32_t can_get_uptime(int port);
extern int can_set_default_baud(int port, short baud);
extern int can_set_dbc(const char *fpath);
extern int can_sync_baud(short *baudlist);
extern int can_sync_default_baud(short *baudlist);
extern int can_get_time(uint32_t uptime, RTCTIME *time);
extern int can_time_ready(void);
extern int can_register_callback(can_cb_t cb);
extern int dbc_register_callback(can_cb_t cb);
/*
This function is used to request a flag bit for caller, it's return a bit-mask
which indicate the position of this bit. This flag bit can be used for marking
some signal-attributes and has no conflict with the other caller. There are 32
bits can be requested in limit.
*/
extern int dbc_request_flag(void);
/*
These two functions just get a copy from dbc structure, it's calling-safe.
*/
extern int dbc_is_load(void);
extern int dbc_copy_signal_from_id(uint32_t id, dbc_sig_t *sig);
extern int dbc_copy_signal_from_name(const char *name, dbc_sig_t *sig);
/*
These two functions are used to ensure the consistency of dbc structure(not
signal's value), because it's maybe different around being changed, such as
reloading.
*/
extern void dbc_lock(void);
extern void dbc_unlock(void);
/*
These functions below could not ensure the consistency of dbc structure around
calling out of dbc-callback, because of the dbc structure maybe changed at the
same time. Call dbc_lock() first before them to make sure that the dbc structure
will not be changed unexpectedly. It's no fault when calling in can-callback or
dbc-callback.
*/
extern int dbc_get_md5(uint8_t *md5);
extern char *dbc_get_fpath(char *fpath, int size);
extern int dbc_get_signal_cnt(void);
extern int dbc_get_signal_id(const char *name);
extern int dbc_get_signal_port(int id);
extern char *dbc_get_signal_name(int id, char *name, int size);
extern char *dbc_get_signal_unit(int id, char *unit, int size);
extern char *dbc_get_signal_ecu(int id, char *ecu, int size);
extern double dbc_get_signal_value(int id);
extern double dbc_get_signal_lastval(int id);
/*
These two functions must be used carefully, the value and last value(others will
not) of the signal pointed by the return pointer may be changed unexpectedly.
So they must be called in the can-callback or dbc-callback to avoid this
disadvantage.
*/
extern const dbc_sig_t *dbc_get_signal_from_id(int id);
extern const dbc_sig_t *dbc_get_signal_from_name(const char *name);
/*
These functions are used to set, clear or test the flags of a signal. The flags
mask must consist of the return values from dbc_request_flag() function. Any
undetermined flags mask passed to dbc_set_signal_flag() or dbc_clr_signal_flag()
function may couse a unexpected result. No matter with dbc_test_signal_flag()
function.
*/
extern void dbc_set_signal_flag(int id, int flags);
extern void dbc_clr_signal_flag(int id, int flags);
extern int dbc_test_signal_flag(int id, int flags, int and);


/*set default ?default baud*/
void can_baud_reset(void);
void can_auto_baud_rs(void);
short can_get_auto_baud(void);

short get_can2_config_baud(void);

#endif
