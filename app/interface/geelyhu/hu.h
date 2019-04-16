#ifndef __HU_H__
#define __HU_H__
#include "../support/protocol.h"

#define HU_ERR_CHECKSUM         0x01
#define HU_ERR_UNKNOWN          0x02
#define HU_ERR_DENIED           0x03
#define HU_ERR_TIMEOUT          0x04
#define HU_ERR_RESET            0x05

#define HU_CMD_TYPE_NONE        0
#define HU_CMD_TYPE_PERIOD      1
#define HU_CMD_TYPE_NEEDACK     2
#define HU_CMD_TYPE_RANDOM      3

#define HU_CMD_ACKERROR         0x01

#define HU_CMD_STATION_INFO     0x10
#define HU_CMD_SIGNAL_LEVEL     0x40
#define HU_CMD_XCALL            0x50
#define HU_CMD_HU_STATUS        0x51
#define HU_CMD_XCALL_CTL        0x52
#define HU_CMD_MUTE             0x53
#define HU_CMD_XCALL_QUERY      0x54
#define HU_CMD_GPS_INFO         0x70
#define HU_CMD_UPG_STATUS       0x80
#define HU_CMD_TBOX_INFO        0x90
#define HU_CMD_DATA_FLOW        0xa0
#define HU_CMD_NET_STATUS       0xb0

#define HU_CMD_FOTA_DLD_REQUEST     0x41
#define HU_CMD_FOTA_DLD_PROCESS     0x42
#define HU_CMD_FOTA_DLD_FINISH      0x43
#define HU_CMD_FOTA_DLD_CONFIRM     0x44
#define HU_CMD_FOTA_DLD_RESTART     0x46
#define HU_CMD_FOTA_UPD_REQUEST     0x47
#define HU_CMD_FOTA_UPD_PROCESS     0x49
#define HU_CMD_FOTA_UPD_FINISH      0x4A
#define HU_CMD_FOTA_UPD_WARN        0x4B
#define HU_CMD_FOTA_ROLLUPD_FINISH  0x4C


#define HU_MAX_SEND_LEN         512
#define HU_MAX_CMD_NUM          32

typedef union
{
    uint8_t raw[HU_MAX_SEND_LEN];
    struct
    {
        uint8_t dle;
        uint8_t som;
        uint8_t dom;
        uint8_t cmd;        
        uint8_t len_m;
        uint8_t len;
        uint8_t dat[HU_MAX_SEND_LEN - 9];
        uint8_t rsv1[3];
    };

    struct
    {
        uint8_t  rsv2[3];
        uint8_t  cshead[0];
    };
} hu_pack_t;

typedef struct
{
    int  cmd;
    int  uptype;
    int  dwtype;
    int  (*send_proc)(hu_pack_t *pack);
    int  (*recv_proc)(hu_pack_t *pack);
    void (*done_proc)(int error);
    uint32_t period;
    uint32_t timeout;
    uint64_t sendtm;    
} hu_cmd_t;

typedef struct
{
    int callevt;
    union
    {
        char callnum[32];
        int  callsta;
    };
} hu_xcall_t;

extern void hu_xcall_event_proc(hu_xcall_t *call);
extern int hu_xcall_start(int type);
extern int hu_xcall_hangup(void);
extern int hu_xcall_init(int phase);
extern int hu_common_init(int phase);
extern int hu_register_cmd(hu_cmd_t *cmd);
extern int hu_fota_init(int phase);


#define CALL_ERR_DENIED         1
#define CALL_ERR_ONLINE         2
#define CALL_ERR_OFFLINE        3        


#define CALL_EVENT_CALLIN       1
#define CALL_EVENT_CALLFAIL     2
#define CALL_EVENT_ONLINE       3
#define CALL_EVENT_HUNGUP       4

/*
 * for cb
 *  event - call event (see above)
 *  par   - parameter
 *      CALL_EVENT_CALLIN   - char*, call in number
 *      CALL_EVENT_CALLFAIL - NULL
 *      CALL_EVENT_ONLINE   - NULL
 *      CALL_EVENT_HUNGUP   - int*, 0: hung up in self side, 1: hung up in the other side
 */        
static inline int at_register_callevent_cb(int (*cb)(int event, void *par))
{
    return 0;
}
/*
 * return: 0 - success
 *         not 0 - fail
 */
static inline int at_make_call(const char* num)
{
    return 0;
}
/*
 * return: 0 - success
 *         not 0 - fail (already hung up)
 */
static inline int at_hang_call(void)
{
    return 0;
}

static inline int at_pick_call(void)
{
    return 0;
}





#endif
