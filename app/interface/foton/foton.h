#ifndef __FOTON_H__
#define __FOTON_H__

typedef enum
{
    FT_MSG_SOCKET = MPU_MID_FOTON,
    FT_MSG_CANON,
    FT_MSG_CANOFF,
    FT_MSG_TIMER,
    FT_MSG_SUSPEND,
    FT_MSG_RESUME,
    FT_MSG_ERRON,
    FT_MSG_ERROFF,
    FT_MSG_CONFIG,
    FT_MSG_NETWORK,
    FT_MSG_STATUS,
    FT_MSG_FTP,
    FT_MSG_WAKEUP_TIMEOUT,
} FT_MSG_TYPE;

typedef enum
{
    FT_UPG_UNKNOW = 0,
    FT_UPG_PKG,
    FT_UPG_FW,
} FT_UPG_TYPE;

typedef struct
{
    uint8_t  data[1024];
    uint32_t len;
    uint32_t seq;
    uint32_t type;
    list_t *list;
    list_t  link;
} ft_pack_t;

extern void ft_data_put_back(ft_pack_t *rpt);
extern void ft_data_put_send(ft_pack_t *rpt);
extern void ft_data_ack_pack(void);
extern void ft_data_flush_sending(void);
extern void ft_data_flush_realtm(void);
extern void ft_data_clear_report(void);
extern void ft_data_clear_error(void);
extern int ft_data_init(INIT_PHASE phase);
extern ft_pack_t *ft_data_get_pack(void);
extern void ft_data_emergence(int set);
extern int ft_data_nosending(void);
extern int ft_data_noreport(void);
extern void ft_data_set_intv(uint16_t intv);
extern int ft_data_get_intv(void);
extern void ft_data_set_pendflag(int flag);
extern int ft_cache_save_all(void);

#endif