#ifndef __GB32960_H__
#define __GB32960_H__


#define GB32960_SHARE_LINK 1 //定义该宏，说明有其他服务需要跟gb32960共用数据收/发链路
#if GB32960_SHARE_LINK
/***********Щㄧ?***********/
typedef void (*gb32960_RcvCallback)(void);

typedef enum
{
    GB_HZ_USER_DEFONED = 0,//合众
	GB_MAX_OBJ,
} GB_RCV_OBJ;


typedef struct
{
    char  startChar[2];
    gb32960_RcvCallback rcvCallback;
} gb_rcvCb_t;
#endif


typedef enum
{
    GB_MSG_SOCKET = MPU_MID_GB32960,
    GB_MSG_CANON,
    GB_MSG_CANOFF,
    GB_MSG_TIMER,
    GB_MSG_SUSPEND,
    GB_MSG_RESUME,
    GB_MSG_ERRON,
    GB_MSG_ERROFF,
    GB_MSG_CONFIG,
    GB_MSG_NETWORK,
    GB_MSG_STATUS,
} GB_MSG_TYPE;

typedef struct
{
    uint8_t  data[1024];
    uint32_t len;
    uint32_t seq;
    uint32_t type;
    list_t *list;
    list_t  link;
} gb_pack_t;

extern void gb_data_put_back(gb_pack_t *rpt);
extern void gb_data_put_send(gb_pack_t *rpt);
extern void gb_data_ack_pack(void);
extern void gb_data_flush_sending(void);
extern void gb_data_flush_realtm(void);
extern void gb_data_clear_report(void);
extern void gb_data_clear_error(void);
extern int gb_data_init(INIT_PHASE phase);
extern gb_pack_t *gb_data_get_pack(void);
extern void gb_data_emergence(int set);
extern int gb_data_nosending(void);
extern int gb_data_noreport(void);
extern void gb_data_set_intv(uint16_t intv);
extern int gb_data_get_intv(void);
extern void gb_data_set_pendflag(int flag);

#endif
