#ifndef __SOCK_API_H__
#define __SOCK_API_H__

typedef enum
{
    SOCK_STAT_UNUSED,
    SOCK_STAT_CLOSED,
    SOCK_STAT_OPENED,
    SOCK_STAT_INPROG,
} sock_stat_t;

typedef enum
{
    SOCK_TYPE_INVAL,
    SOCK_TYPE_TCP,
    SOCK_TYPE_UDP,
    SOCK_TYPE_SYNCTCP,
    SOCK_TYPE_MAX,
} sock_type_t;

extern int sock_init(INIT_PHASE phase);
extern int sock_create(const char *name, int type);
extern int sock_delete(int sid);
extern int sock_open(int type, int sid, const char *url, uint16_t port);
extern int sock_listen_port(int sid, uint16_t port);
extern int sock_close(int sid);
extern int sock_send(int sid, uint8_t *buf, int len, void (*sync)(void));
extern int sock_recv(int sid, uint8_t *buf, int len);
extern  int sock_recv_check(void);
extern int sock_status(int sid);
extern int sock_error(int sid);
extern int sock_sync(int sid);
extern int sock_bufsz(int sid);
extern const char *sock_strstat(int stat);

extern int sock_reconnect_times(int sid);
extern int sock_reconnect_times_clear(int sid);


#endif

