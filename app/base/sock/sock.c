/*
********************************************************************************
file:         socket.c
description:  the source file of socket management
date:         2017/11/10
author        liuzhenrong
********************************************************************************
*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "init.h"
#include "log.h"
#include "timer.h"
#include "list.h"
#include "shell_api.h"
#include "sock_api.h"
#include "nm_api.h"

/* maximum socket can be openned */
#define SOCK_MAX_SOCKET     10
/* maximum sync-list length for sync-tcp socket */
#define SOCK_MAX_SYNC       40
/* muximum url length */
#define SOCK_MAX_URLLEN     256
/* send timeout value, in millisecond */
#define SOCK_SEND_TIMEOUT   3000
/* connect timeout value, in second */
#define SOCK_CONN_TIMEOUT   10

#define SOCK_ACK_TIMEOUT    15000

#define sock_lock(sock)     pthread_mutex_lock(&(sock)->mtx)
#define sock_trylock(sock)  pthread_mutex_trylock(&(sock)->mtx)
#define sock_unlock(sock)   pthread_mutex_unlock(&(sock)->mtx)

/* sync-list entity for sync-tcp socket */
typedef struct
{
    int len;
    void (*sync)(void);
    list_t link;
    list_t *owner;
} sync_t;

/* socket management entity */
typedef struct _sock_t
{
    /* can't be cleaned up attributes */
    const char *name;
    int  type;
    int  stat;
    int  apn_type;
    pthread_mutex_t mtx;
    uint64_t chcktime;
    uint64_t linktime;
    uint64_t sendtime;
    int connect_failtimes;//Liu Binkui added for GEELY

    /* can be cleaned up attributes */
    int  sockfd;
    char url[SOCK_MAX_URLLEN];
    int  port;
    int  sendsz;
    sync_t sync_mem[SOCK_MAX_SYNC];
    list_t sync_free;
    list_t sync_back;
    list_t sync_used;
    pthread_t tid;
    struct _sock_t *server;
    int clients;
} sock_t;

static sock_t sock_tbl[SOCK_MAX_SOCKET];


int sock_status(int sid)
{
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    return sock->stat;
}

/**
    Liu Binkui ���ӣ�������ȡsocket����ʧ�ܴ���
*/
int sock_reconnect_times(int sid)
{
    int tmpint = 0;
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    tmpint = sock->connect_failtimes;

    return tmpint;
}

/**
    Liu Binkui ���ӣ��������socket����ʧ�ܴ���
*/
int sock_reconnect_times_clear(int sid)
{
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    sock_lock(sock);
    sock->connect_failtimes = 0;
    sock_unlock(sock);

    return 0;
}



int sock_error(int sid)
{
    sock_t *sock = sock_tbl + sid;
    int error;
    socklen_t len = sizeof(error);

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    if (sock->stat == SOCK_STAT_UNUSED)
    {
        return -1;
    }

    if (getsockopt(sock->sockfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0)
    {
        return -1;
    }

    return error;
}

int sock_sync(int sid)
{
    int unacksz;
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    if (sock->stat != SOCK_STAT_OPENED)
    {
        return -1;
    }

    if (sock->type != SOCK_TYPE_SYNCTCP)
    {
        return 0;
    }

    if (ioctl(sock->sockfd, SIOCOUTQ, &unacksz) < 0)
    {
        return -1;
    }

    if (!list_empty(&sock->sync_used))
    {
        sync_t *sync = list_entry(sock->sync_used.next, sync_t, link);

        if (sync->len <= sock->sendsz - unacksz)
        {
            sock->sendsz -= sync->len;
            list_delete(&sync->link);
            list_insert_before(sync->owner, &sync->link);

            if (sync->sync)
            {
                sync->sync();
            }
        }
    }

    return 0;
}

int sock_bufsz(int sid)
{
    sock_t *sock = sock_tbl + sid;
    int buffsz = 0, unacksz = 0;
    socklen_t len = sizeof(buffsz);

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    if (sock->stat != SOCK_STAT_OPENED)
    {
        return -1;
    }

    if (getsockopt(sock->sockfd, SOL_SOCKET, SO_SNDBUF, &buffsz, &len) < 0)
    {
        return -1;
    }

    if (ioctl(sock->sockfd, SIOCOUTQ, &unacksz) < 0)
    {
        return -1;
    }

    return buffsz - unacksz;
}


int sock_send(int sid, uint8_t *buf, int len, void (*sync)(void))
{
    int res, wrl = 0;
    sock_t *sock = sock_tbl + sid;
    uint64_t timeout = tm_get_time() + SOCK_SEND_TIMEOUT;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);
    assert(buf != NULL);

    if (sock->stat != SOCK_STAT_OPENED)
    {
        return -1;
    }

    if (sock->type == SOCK_TYPE_SYNCTCP &&
        list_empty(&sock->sync_free) && (sync || list_empty(&sock->sync_back)))
    {
        return (tm_get_time() - sock->sendtime > SOCK_ACK_TIMEOUT) ? -1 : 0;
    }

    while (len - wrl > 0 && tm_get_time() < timeout)
    {
        res = send(sock->sockfd, buf + wrl, len - wrl, MSG_DONTWAIT);

        if (res <= 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                return -1;
            }
        }
        else
        {
            wrl += res;
        }
    }

    if (wrl != len)
    {
        return -1;
    }

    if (sock->type == SOCK_TYPE_SYNCTCP)
    {
        list_t *node = sync ? NULL : list_get_first(&sock->sync_back);

        if (node == NULL)
        {
            node = list_get_first(&sock->sync_free);
        }

        sock->sendsz += len;
        list_entry(node, sync_t, link)->len  = len;
        list_entry(node, sync_t, link)->sync = sync;
        list_insert_before(&sock->sync_used, node);
    }

    sock->sendtime = tm_get_time();
    return len;
}

int sock_recv(int sid, uint8_t *buf, int len)
{
    int res;
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);
    assert(buf != NULL);

    if (sock->stat != SOCK_STAT_OPENED)
    {
		log_e(LOG_SOCK_PROXY, "socket state != open");
        return -1;
    }

    res = recv(sock->sockfd, buf, len, MSG_DONTWAIT);

    if (res == 0)
    {
		log_e(LOG_SOCK_PROXY, "res == 0");
        res = -1;
    }
    else if (res < 0 && (errno == EINTR || errno == EAGAIN))
    {
        res = 0;
    }

    return res;
}

int sock_recv_check(void)
{
    int i, unread;

    for (i = 0; i < SOCK_MAX_SOCKET; i++)
    {
        if (sock_trylock(sock_tbl + i) != 0)
        {
            continue;
        }

        if (sock_tbl[i].stat == SOCK_STAT_OPENED &&
            ioctl(sock_tbl[i].sockfd, SIOCINQ, &unread) == 0 && unread > 0)
        {
            sock_unlock(sock_tbl + i);
            return i;
        }

        sock_unlock(sock_tbl + i);
    }

    return -1;
}


static int sock_resolve(const char *url, uint32_t *addr)
{
    struct hostent *he = gethostbyname(url);

    if (he == NULL || he->h_addrtype != AF_INET || he->h_length < sizeof(*addr))
    {
        return -1;
    }

    memcpy(addr, he->h_addr, sizeof(*addr));
    return 0;
}

static void *sock_finish(sock_t *sock, int fd, int stat)
{
    sock_lock(sock);
    sock->stat = stat;

    if (stat == SOCK_STAT_OPENED)
    {
        sock->chcktime = tm_get_time();
        sock->sendtime = 0;
        sock->sockfd = fd;
        sock->connect_failtimes = 0;//Liu Binkui added for geely
    }
    else if (fd > 0)
    {
        close(fd);
    }

    if (stat == SOCK_STAT_CLOSED)
    {
        sock->connect_failtimes++;//Liu Binkui added for geely
    }

    sock->tid = 0;
    sock_unlock(sock);
    return NULL;
}

static void *sock_open_proc(sock_t *sock)
{
    int res, sockfd;
    struct sockaddr_in addr;
    uint32_t s_addr;
    uint8_t *ip = (uint8_t *)&s_addr;

    log_i(LOG_SOCK, "socket(%s) is starting to resolve url: \"%s\"", sock->name, sock->url);

    if(sock_resolve(sock->url, &s_addr))
    {
        log_e(LOG_SOCK, "socket(%s) resolve url fail", sock->name);
        return sock_finish(sock, -1, SOCK_STAT_CLOSED);
    }

    log_i(LOG_SOCK, "socket(%s) url resolved: %u.%u.%u.%u", sock->name, ip[0], ip[1], ip[2], ip[3]);
    log_i(LOG_SOCK, "socket(%s) is starting to connect with: \"%s\":%u", sock->name, sock->url,
          sock->port);

    if ((sockfd = socket(AF_INET, sock->type == SOCK_TYPE_UDP ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0)
    {
        log_e(LOG_SOCK, "socket(%s) create socket fail: %s", sock->name, strerror(errno));
        return sock_finish(sock, sockfd, SOCK_STAT_CLOSED);
    }

    nm_set_net(sockfd, sock->apn_type);

    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)(sock->port));
    addr.sin_addr.s_addr = s_addr;

    res = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

    if (res == 0)
    {
        log_i(LOG_SOCK, "socket(%s) is connected", sock->name);
        return sock_finish(sock, sockfd, SOCK_STAT_OPENED);
    }

    if (errno == EINPROGRESS)
    {
        fd_set set;
        struct timeval tv;
        int error = 0;
        socklen_t errlen = sizeof(error);

        log_i(LOG_SOCK, "socket(%s) is waitting for connection", sock->name);

        FD_ZERO(&set);
        FD_SET(sockfd, &set);

        tv.tv_sec  = 10;
        tv.tv_usec = 0;

        switch (select(sockfd + 1, NULL, &set, NULL, &tv))
        {
            case -1:
                log_e(LOG_SOCK, "socket(%s) select fail: %s", sock->name, strerror(errno));
                break;

            case 0:
                log_e(LOG_SOCK, "socket(%s) select fail: time out", sock->name);
                break;

            default:
                if (!FD_ISSET(sockfd, &set))
                {
                    log_e(LOG_SOCK, "socket(%s) select fail: socket not be set", sock->name);
                }
                else if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &errlen) < 0)
                {
                    log_e(LOG_SOCK, "socket(%s) getsockopt fail: %s", sock->name, strerror(errno));
                }
                else if (error)
                {
                    log_e(LOG_SOCK, "socket(%s) connect fail: %s", sock->name, strerror(error));
                }
                else
                {
                    log_i(LOG_SOCK, "socket(%s) is connected", sock->name);
                    return sock_finish(sock, sockfd, SOCK_STAT_OPENED);
                }

                break;
        }
    }
    else
    {
        log_e(LOG_SOCK, "socket(%s) connect fail: %s", sock->name, strerror(errno));
    }

    return sock_finish(sock, sockfd, SOCK_STAT_CLOSED);
}

static void sock_cleanup(sock_t *sock, const char *url, uint16_t port)
{
    strncpy(sock->url, url, SOCK_MAX_URLLEN - 1);
    sock->url[SOCK_MAX_URLLEN - 1] = 0;
    sock->port    = port;
    sock->tid     = 0;
    sock->server  = NULL;
    sock->clients = 0;
    sock->sockfd  = -1;

    if (sock->type == SOCK_TYPE_SYNCTCP)
    {
        int i, back;

        sock->sendsz = 0;
        list_init(&sock->sync_free);
        list_init(&sock->sync_back);
        list_init(&sock->sync_used);

        back = SOCK_MAX_SYNC / 10;

        for (i = 0; i < back; i++)
        {
            sock->sync_mem[i].owner = &sock->sync_back;
            list_insert_before(&sock->sync_back, &sock->sync_mem[i].link);
        }

        for (; i < SOCK_MAX_SYNC; i++)
        {
            sock->sync_mem[i].owner = &sock->sync_free;
            list_insert_before(&sock->sync_free, &sock->sync_mem[i].link);
        }
    }
}


static sock_t *sock_getsvr(uint16_t port, int type)
{
    int i;

    for (i = 0; i < SOCK_MAX_SOCKET; i++)
    {
        sock_lock(sock_tbl + i);

        if (sock_tbl[i].stat != SOCK_STAT_UNUSED &&
            sock_tbl[i].server == sock_tbl + i &&
            sock_tbl[i].port == port &&
            sock_tbl[i].type == type)
        {
            sock_tbl[i].clients++;
            sock_unlock(sock_tbl + i);
            log_i(LOG_SOCK, "server socket(@%hu) is found", port);
            return sock_tbl + i;
        }

        sock_unlock(sock_tbl + i);
    }

    for (i = 0; i < SOCK_MAX_SOCKET; i++)
    {
        sock_lock(sock_tbl + i);

        if (sock_tbl[i].stat == SOCK_STAT_UNUSED)
        {
            struct sockaddr_in addr;
            int opt = 1;
            int sockfd = socket(AF_INET, type == SOCK_TYPE_UDP ? SOCK_DGRAM : SOCK_STREAM, 0);

            if (sockfd < 0)
            {
                sock_unlock(sock_tbl + i);
                log_e(LOG_SOCK, "server socket(@%hu) create fail: %s", port, strerror(errno));
                return NULL;
            }

            addr.sin_family = AF_INET;
            addr.sin_port   = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);


            setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt));

            if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
            {
                sock_unlock(sock_tbl + i);
                close(sockfd);
                log_e(LOG_SOCK, "server socket(@%hu) bind fail: %s", port, strerror(errno));
                return NULL;
            }

            sock_tbl[i].name = type == SOCK_TYPE_UDP ? "udp server" : "tcp server";
            sock_tbl[i].type = type;
            sock_tbl[i].stat = SOCK_STAT_OPENED;
            sock_tbl[i].linktime = 0;
            sock_tbl[i].chcktime = tm_get_time();
            sock_cleanup(sock_tbl + i, inet_ntoa(addr.sin_addr), port);

            sock_tbl[i].sockfd  = sockfd;
            sock_tbl[i].server  = sock_tbl + i;
            sock_tbl[i].clients = 1;
            sock_unlock(sock_tbl + i);
            log_i(LOG_SOCK, "server socket(@%hu) is created", port);
            return sock_tbl + i;
        }

        sock_unlock(sock_tbl + i);
    }

    return NULL;
}

static void sock_putsvr(sock_t *sock)
{
    sock_lock(sock);

    if (sock->stat != SOCK_STAT_UNUSED &&
        sock->server == sock && --sock->clients <= 0)
    {
        log_i(LOG_SOCK, "server socket(@%hu) is deleted", sock->port);

        if (sock->sockfd > 0)
        {
            close(sock->sockfd);
        }

        sock->stat = SOCK_STAT_UNUSED;
    }

    sock_unlock(sock);
}


int sock_create(const char *name, int type)
{
    int i;

    assert(type > 0 && type < SOCK_TYPE_MAX);

    for (i = 0; i < SOCK_MAX_SOCKET; i++)
    {
        if (sock_trylock(sock_tbl + i) != 0)
        {
            continue;
        }

        if (sock_tbl[i].stat == SOCK_STAT_UNUSED)
        {
            sock_tbl[i].tid    = 0;
            sock_tbl[i].name   = name ? name : "unnamed";
            sock_tbl[i].type   = type;
            sock_tbl[i].stat   = SOCK_STAT_CLOSED;
            sock_tbl[i].linktime = 0;

            log_i(LOG_SOCK, "socket(%s) is created", sock_tbl[i].name);
            sock_unlock(sock_tbl + i);
            return i;
        }

        sock_unlock(sock_tbl + i);
    }

    return -1;
}

int sock_delete(int sid)
{
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    sock_lock(sock);

    if (sock->stat != SOCK_STAT_UNUSED && sock->server != sock)
    {
        if (sock->tid != 0)// && EBUSY == pthread_tryjoin_np(sock->tid, NULL))
        {
            pthread_cancel(sock->tid);
            sock->tid = 0;
            sleep(1);
        }

        if (sock->sockfd > 0)
        {
            close(sock->sockfd);
        }

        if (sock->server)
        {
            sock_putsvr(sock->server);
        }

        log_i(LOG_SOCK, "socket(%s) is deleted", sock->name);
        sock_tbl[sid].stat = SOCK_STAT_UNUSED;
    }

    sock_unlock(sock_tbl + sid);

    return 0;
}

static int sock_listen_on(sock_t *sock)
{
    int ret = -1;

    sock_lock(sock);

    if (sock->stat != SOCK_STAT_OPENED)
    {
        log_e(LOG_SOCK, "server socket(@%hu) is busy", sock->port);
    }
    else
    {
        sock->stat = SOCK_STAT_INPROG;
        sock_unlock(sock);

        if ((ret = listen(sock->sockfd, 1)) < 0)
        {
            log_e(LOG_SOCK, "server socket(@%hu) listen fail: %s", sock->port, strerror(errno));
        }

        sock_lock(sock);
        sock->stat = SOCK_STAT_OPENED;
    }

    sock_unlock(sock);
    return ret;
}

int sock_listen_port(int sid, uint16_t port)
{
    sock_t *sock = sock_tbl + sid, *server;
    int sockfd, type;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    const char *name;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    sock_lock(sock);

    if (sock->stat == SOCK_STAT_UNUSED)
    {
        log_e(LOG_SOCK, "socket %d is not used", sid);
        sock_unlock(sock);
        return -1;
    }

    if (sock->stat != SOCK_STAT_CLOSED || sock->server == sock)
    {
        log_e(LOG_SOCK, "socket(%s) is busy", sock->name);
        sock_unlock(sock);
        return -1;
    }

    name = sock->name;
    type = sock->type;
    sock_unlock(sock);

    if ((server = sock_getsvr(port, type)) == NULL)
    {
        log_e(LOG_SOCK, "socket(%s) no server socket(@%hu)", name, port);
        return -1;
    }

    if (sock_listen_on(server) < 0)
    {
        log_e(LOG_SOCK, "socket(%s) listen server socket(@%hu) fail", name, port);
        sock_putsvr(server);
        return -1;
    }

    if ((sockfd = accept(server->sockfd, (struct sockaddr *)&addr, &addrlen)) < 0)
    {
        log_e(LOG_SOCK, "socket(%s) accept server socket(@%hu) fail: %s",
              name, port, strerror(errno));
        sock_putsvr(server);
        return -1;
    }

    sock_lock(sock);

    if (sock->stat != SOCK_STAT_CLOSED)
    {
        log_e(LOG_SOCK, "socket(%s) modified unexpectedly", name);
        close(sockfd);
        sock_putsvr(server);
        sock_unlock(sock);
        return -1;
    }

    sock_cleanup(sock, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    sock->sockfd = sockfd;
    sock->server = server;
    sock->chcktime = tm_get_time();
    sock->stat = SOCK_STAT_OPENED;
    log_i(LOG_SOCK, "socket(%s) is connected", sock->name);
    sock_unlock(sock);
    return 0;
}

int sock_open(int type, int sid, const char *url, uint16_t port)
{
    sock_t *sock = sock_tbl + sid;
    int ret = -1;

    sock->apn_type = type;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);
    assert(url != NULL);

    sock_lock(sock);

    if (sock->stat == SOCK_STAT_UNUSED)
    {
        log_e(LOG_SOCK, "socket(id:%d) is not used", sid);
    }
    else if (sock->stat != SOCK_STAT_CLOSED)
    {
        log_e(LOG_SOCK, "socket(%s) has been connected or in progress", sock->name);
    }
    else
    {
        pthread_attr_t ta;

        sock_cleanup(sock, url, port);
        pthread_attr_init(&ta);
        pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&sock_tbl[sid].tid, &ta, (void *)sock_open_proc, sock_tbl + sid) < 0)
        {
            sock->stat = SOCK_STAT_CLOSED;
            log_e(LOG_SOCK, "socket(%s) start handler fail: %s", sock->name, strerror(errno));
        }
        else
        {
            sock->stat = SOCK_STAT_INPROG;
            ret = 0;
        }
    }

    sock_unlock(sock);
    return ret;
}

int sock_close(int sid)
{
    sock_t *sock = sock_tbl + sid;

    assert(sid >= 0 && sid < SOCK_MAX_SOCKET);

    sock_lock(sock);

    if (sock->stat == SOCK_STAT_OPENED && sock->server != sock)
    {
        if (sock->tid != 0)// && EBUSY == pthread_tryjoin_np(sock->tid, NULL))
        {
            pthread_cancel(sock->tid);
            sock->tid = 0;
            sleep(1);
        }

        close(sock->sockfd);
        sock->tid = 0;
        sock->linktime += tm_get_time() - sock->chcktime;
        sock->stat = SOCK_STAT_CLOSED;
        sock->sockfd = -1;

        if (sock->server)
        {
            sock_putsvr(sock->server);
            sock->server = NULL;
        }

        log_i(LOG_SOCK, "socket(%s) is closed", sock->name);
    }

    sock_unlock(sock);

    return 0;
}

const char *sock_strstat(int stat)
{
    static const char *strstat[] =
    {
        "unused",
        "closed",
        "openned",
        "in progress",
    };

    assert(stat >= 0 && stat <= 3);
    return strstat[stat];
}

static int sock_test(int argc, const char **argv)
{
    int i, port, cnt, level, type, sock;

    if (argc != 4 || 1 != sscanf(argv[1], "%d", &port) ||
        1 != sscanf(argv[2], "%d", &type) || 1 != sscanf(argv[3], "%d", &cnt))
    {
        shellprintf(" usage: sock_test <url> <port> <type> <times>\r\n");
        return -1;
    }

    level = log_get_level(LOG_SOCK);
    log_set_level(LOG_SOCK, LOG_DEBUG);
    sock = sock_create("TEST", type ? SOCK_TYPE_TCP : SOCK_TYPE_UDP);

    for (i = 0; i < cnt; i++)
    {
        shellprintf("  socket test <%d/%d>: %s %s %d\r\n", i + 1, cnt, type ? "TCP" : "UDP",
                    argv[0], port);
        sock_open(NM_PUBLIC_NET, sock, argv[0], port);
        sock_close(sock);
    }

    log_set_level(LOG_SOCK, level);
    sock_delete(sock);

    return 0;
}


static int sock_list(int argc, const char **argv)
{
    int i;
    sock_t *sock;
    static const char *strtype[] =
    {
        "invalid",
        "tcp",
        "udp",
        "sync-tcp",
        "none"
    };


    shellprintf(" socket list\r\n");

    for (i = 0, sock = sock_tbl; i < SOCK_MAX_SOCKET; i++, sock++)
    {
        sock_lock(sock);
        shellprintf("  %02d, %s", i, sock_strstat(sock->stat));

        if (sock->stat != SOCK_STAT_UNUSED)
        {
            uint32_t percent;
            uint64_t time = tm_get_time();

            shellprintf(", %s, %s", sock->name, strtype[sock->type]);

            if (sock->stat != SOCK_STAT_CLOSED)
            {
                shellprintf(", %s : %u", sock->url, sock->port);
            }

            if (sock->server != sock)
            {
                if (sock->stat == SOCK_STAT_OPENED)
                {
                    sock->linktime += time - sock->chcktime;
                    sock->chcktime  = time;
                }

                percent = (sock->linktime * 10000 + time / 2) / time;
                shellprintf(", %llu/%llu, %u.%u%%", sock->linktime, time, percent / 100, percent % 100);
            }
        }

        sock_unlock(sock);
        shellprintf("\r\n");
    }

    return 0;
}

int sock_init(INIT_PHASE phase)
{
    int i, ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            for (i = 0; i < SOCK_MAX_SOCKET; i++)
            {
                sock_tbl[i].stat = SOCK_STAT_UNUSED;
                pthread_mutex_init(&sock_tbl[i].mtx, NULL);
            }

            break;

        case INIT_PHASE_OUTSIDE:

            ret |= shell_cmd_register_ex("sockls", "info", sock_list, "display socket list");
            ret |= shell_cmd_register_ex("socktst", NULL, sock_test, "test socket");

        default:
            break;
    }

    return ret;
}

