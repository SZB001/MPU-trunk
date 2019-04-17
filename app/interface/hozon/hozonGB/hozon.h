#ifndef __HOZON_H__
#define __HOZON_H__

typedef enum
{
    HZ_MSG_SOCKET = MPU_MID_FOTON,
    HZ_MSG_CANON,
    HZ_MSG_CANOFF,
    HZ_MSG_TIMER,
    HZ_MSG_SUSPEND,
    HZ_MSG_RESUME,
    HZ_MSG_ERRON,
    HZ_MSG_ERROFF,
    HZ_MSG_CONFIG,
    HZ_MSG_NETWORK,
    HZ_MSG_STATUS,
    HZ_MSG_FTP,
    HZ_MSG_WAKEUP_TIMEOUT,
} HZ_MSG_TYPE;

typedef enum
{
    HZ_UPG_UNKNOW = 0,
    HZ_UPG_PKG,
    HZ_UPG_FW,
} HZ_UPG_TYPE;

typedef struct
{
    uint8_t  data[1024];
    uint32_t len;
    uint32_t seq;
    uint32_t type;
    list_t *list;
    list_t  link;
} hz_pack_t;

extern void ht_data_put_back(ht_pack_t *rpt);
extern void ht_data_put_send(ht_pack_t *rpt);
extern void ht_data_ack_pack(void);
extern void ht_data_flush_sending(void);
extern void ht_data_flush_realtm(void);
extern void ht_data_clear_report(void);
extern void ht_data_clear_error(void);
extern int ht_data_init(INIT_PHASE phase);
extern ht_pack_t *ht_data_get_pack(void);
extern void ht_data_emergence(int set);
extern int ht_data_nosending(void);
extern int ht_data_noreport(void);
extern void ht_data_set_intv(uint16_t intv);
extern int ht_data_get_intv(void);
extern void ht_data_set_pendflag(int flag);
extern int ht_cache_save_all(void);

#endif