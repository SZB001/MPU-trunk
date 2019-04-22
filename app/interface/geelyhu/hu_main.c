#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "hu.h"

#define HU_DUMP_TITLE       "Geely HU"

#define CHN_UART            0
#define CHN_CAN             1
#define CHN_IP              2
#define CHN_BT              3

#define DEV_MCU             0
#define DEV_MPU             1
#define DEV_HU              2
#define DEV_PC              3

#define SORC(s)             ((s) << 2)
#define DEST(d)             ((d) << 5)

typedef enum
{
    HU_MSG_NETWORK = MPU_MID_GEELYHU,
    HU_MSG_XCALL,
    HU_MSG_LINKST,
} HU_MSG_TYPE;

typedef union
{
    /* for HU_MSG_NETWORK */
    int network;
    hu_xcall_t xcall;
} hu_msg_t;

typedef struct
{
    int socket;
    int network;
    hu_cmd_t *wait;
    uint64_t hbtime;
} hu_stat_t;

static hu_cmd_t hu_cmd_tbl[HU_MAX_CMD_NUM];

static void hu_reset(hu_stat_t *state)
{
    if (state->wait && state->wait->done_proc)
    {
        state->wait->done_proc(HU_ERR_RESET);
    }

    state->wait = NULL;
    state->hbtime = 0;
    sock_close(state->socket);
}

static hu_cmd_t *hu_find_command(int cmd)
{
    hu_cmd_t *pcmd;

    for (pcmd = hu_cmd_tbl; pcmd < ARRAY_LMT(hu_cmd_tbl) && pcmd->cmd != cmd; pcmd++)
    {
    }

    return pcmd < ARRAY_LMT(hu_cmd_tbl) ? pcmd : NULL;
}

static int hu_checksum(uint8_t *data, int len)
{
    uint8_t cs = 0;

    while (len--)
    {
        cs ^= data[len];
    }

    return cs;
}

#if 0
static int hu_decode(uint8_t *data, int size)
{
    int si, di, conv = 0;
    uint8_t trans[3] = {0, 0x0d, 0x0e};

    for (si = 0, di = 0; si < size; si++)
    {
        if (conv)
        {
            if (data[si] == 0x01 || data[si] == 0x02)
            {
                conv = 0;
                data[di++] = trans[data[si]];
            }
            else
            {
                log_e(LOG_FOTONHU, "unexpected byte 0x%02x after 0x0e", data[si]);
                break;
            }
        }
        else if (data[si] == 0x0e)
        {
            conv = 1;
        }
        else
        {
            data[di++] = data[si];
        }
    }

    return conv ? 0 : di;
}

static int hu_encode(uint8_t *src, int srcsz, uint8_t *dst, int dstsz)
{
    int si, di;

    for (si = 0, di = 0; si < srcsz && di < dstsz; si++, di++)
    {
        int next;

        if (src[si] == 0x0d)
        {            
            dst[di++] = 0x0e;
            next = 0x01;
        }
        else if (src[si] == 0x0e)
        {
            dst[di++] = 0x0e;
            next = 0x02;
        }
        else
        {
            next = src[si];
        }

        if (di >= dstsz)
        {
            log_e(LOG_FOTONHU, "output buffer over flow");
            break;
        }

        dst[di] = next;
    }

    log_e(LOG_FOTONHU, "encode, isz=%d, osz=%d ", srcsz, di);
    return si < srcsz ? 0 : di;
}
#endif

static int hu_send_pack(hu_stat_t *state, hu_pack_t *pack)
{
    pack->dle = 0x0d;
    pack->som = 0x05;
    pack->dom = CHN_IP | SORC(DEV_MPU) | DEST(DEV_HU);    
    pack->len_m = 0;
    pack->dat[pack->len] = hu_checksum(pack->cshead, pack->len + 3);
    pack->dat[pack->len + 1] = 0x0d;
    pack->dat[pack->len + 2] = 0x0a;
    //txlen = hu_encode(pack->raw + 2, pack->len + 5, txbuf + 2, HU_MAX_SEND_LEN - 4);
    protocol_dump(LOG_FOTONHU, HU_DUMP_TITLE, pack->raw, pack->len + 9, 1);
    return sock_send(state->socket, pack->raw, pack->len + 9, 0);
}

static int hu_recv_pack(hu_stat_t *state, hu_pack_t **pack)
{
    static int opos = 0, ipos = 0, vpos = 0, step = 0;			/*vpos for recv length;opos and ipos for buf;step for head and tail*/
    static uint8_t ibuf[HU_MAX_SEND_LEN], obuf[HU_MAX_SEND_LEN];/*ibuf for recv pack;obuf for return pack*/
    static uint8_t dict[4] = {0x0d, 0x05, 0x0d, 0x0a};
    int retv = 0;
    hu_pack_t *rpak = NULL;
    
#if 1 /*subpackage,deal each pack for static variable*/
    do
    {   
        while (step < 4 && opos < HU_MAX_SEND_LEN && ipos < vpos)
        {
            if (ibuf[ipos] == dict[step])
            {
                step++;
            }
            else if (step == 0)
            {
                ipos++;
            }
            else if (step != 2)
            {
                step = opos = 0;
            }
            
            if (step)
            {
                obuf[opos++] = ibuf[ipos++];
            }
        }

        if (step >= 4)
        {
            retv = opos;
            step = opos = 0;            
            rpak = (hu_pack_t*)obuf;
            protocol_dump(LOG_FOTONHU, HU_DUMP_TITLE, obuf, retv, 0);
        }
        else if (opos >= HU_MAX_SEND_LEN)
        {
            step = opos = 0;
        }
        else if (ipos >= vpos)
        {
            ipos = vpos = 0;

            if ((retv = sock_recv(state->socket, ibuf + vpos, HU_MAX_SEND_LEN - vpos)) < 0)
            {
                opos = step = 0;
            }
            else
            {
                vpos += retv;
            }
        }     
    } while (!rpak && ipos < vpos);
#else  /*single pack*/
    if ((rxlen = sock_recv(state->socket, obuf, HU_MAX_SEND_LEN)) > 0)
    {
        protocol_dump(LOG_FOTONHU, HU_DUMP_TITLE, obuf, rxlen, 0);
        *pack = (hu_pack_t *)obuf;
    }
    ret = rxlen;
#endif
    *pack = rpak;
    return retv;
}

static int hu_send_error(hu_stat_t *state, int error)
{
    hu_pack_t pack;

    pack.cmd    = error ? HU_CMD_ACKERROR : 0;
    pack.len    = error ? 1 : 0;
    pack.dat[0] = error;

	protocol_dump(LOG_FOTONHU, HU_DUMP_TITLE, pack.raw, pack.len + 9, 1);
    return hu_send_pack(state, &pack);
}

static int hu_check_recv(hu_stat_t *state)
{
    hu_pack_t *pack;
    hu_cmd_t  *pcmd;
    int ret = 0;

    if ((ret = hu_recv_pack(state, &pack)) > 0)
    {
        if (pack->cmd == 0x2a)
        {
            if ((state->hbtime = tm_get_time()) == 0)
            {
                state->hbtime = 1;
            }
            return hu_send_pack(state, pack);
        }
        
        if ((pcmd = hu_find_command(pack->cmd)) == NULL)
        {
            log_e(LOG_FOTONHU, "command 0x%02x is unsupported", pack->cmd);
            //ret = hu_send_error(state, 0x02);
        }
        #if 1
        else if (pack->dat[pack->len] != hu_checksum(pack->cshead, pack->len + 3))
        #else
        else if (pack->dat[pack->len] != hu_checksum(pack->cshead, pack->len + 2))
        #endif
        {
            log_e(LOG_FOTONHU, "packet checksum error");

            if (pcmd->dwtype == HU_CMD_TYPE_NEEDACK)
            {
                ret = hu_send_error(state, HU_ERR_CHECKSUM);
            }
        }
        else if (pcmd->dwtype == 0 && state->wait)
        {
            int done = 0, err;

            if (pcmd->cmd == HU_CMD_ACKERROR)
            {
                done = 1;
                err  = pack->dat[0];

                log_e(LOG_FOTONHU, "error, command 0x%02x return: 0x%02x", state->wait->cmd, err);
            }
            else if (pcmd->cmd == state->wait->cmd)
            {
                done = 1;
                err  = pcmd->recv_proc ? pcmd->recv_proc(pack) : 0;

                log_e(LOG_FOTONHU, "command 0x%02x return: 0x%02x", pcmd->cmd, err);
            }

            if (done)
            {
                if (state->wait->done_proc)
                {
                    state->wait->done_proc(err);
                }
                state->wait = NULL;
            }
        }
        else if (pcmd->dwtype != 0)
        {
            int err = pcmd->recv_proc ? pcmd->recv_proc(pack) : 0;

            if (pcmd->dwtype == HU_CMD_TYPE_NEEDACK)
            {
                if (!err)
                {
                    err = pcmd->send_proc == NULL ? HU_ERR_DENIED : pcmd->send_proc(pack);
                }

                if (!err)
                {
                    ret = hu_send_pack(state, pack);
                }
                else
                {
                    ret = hu_send_error(state, err);
                }
            }
        }
    }

    return ret;
}

static hu_pack_t pack_temp;
static int wait_huconut = 0;
static int hu_check_send(hu_stat_t *state)
{
    hu_cmd_t *pcmd;
    hu_pack_t pack;
    int ret = 0;
    int need_ack = 0;

    for (pcmd = hu_cmd_tbl; pcmd < ARRAY_LMT(hu_cmd_tbl) && !ret; pcmd++)
    {
        if (pcmd->send_proc != NULL)
        {
            int send = 0;
            need_ack = 0;

            switch (pcmd->uptype)
            {
                case HU_CMD_TYPE_PERIOD:
                    send = (tm_get_time() - pcmd->sendtm >= pcmd->period) ? 1 : 0;
                    break;

                case HU_CMD_TYPE_NEEDACK:
                    send = state->wait ? 0 : 1;
                    need_ack = 1;
                    break;

                case HU_CMD_TYPE_RANDOM:
                    send = 1;

                default:
                    break;
            }

            if (send && pcmd->send_proc(&pack) == 0)
            {
                pack.cmd = pcmd->cmd;
                ret  = hu_send_pack(state, &pack);
                pcmd->sendtm = (ret > 0) ? tm_get_time() : 0;
                state->wait = (ret > 0 && pcmd->uptype == HU_CMD_TYPE_NEEDACK) ? pcmd : state->wait;
                if(need_ack)
                {
                    memset(&pack_temp, 0, sizeof(hu_pack_t));
                    memcpy(&pack_temp, &pack, sizeof(hu_pack_t));
                    wait_huconut = 1;
                   /* if((pack.cmd == HU_CMD_FOTA_DLD_FINISH)||(pack.cmd == HU_CMD_FOTA_UPD_FINISH))
                    {
                      ;//  send_hu_finish_flag = 1;
                    }*/
                }
            }
        }
    }

    return ret;
}
 
static int hu_check_wait(hu_stat_t *state)
{
    hu_pack_t pack;
    int ret = 0;

    if (state->wait &&
        tm_get_time() - state->wait->sendtm >= state->wait->timeout)
    {
        log_e(LOG_FOTONHU, "command %02x timeout", state->wait->cmd);
        if (wait_huconut != 3)
        {
            memcpy(&pack, &pack_temp, sizeof(hu_pack_t));
            ret  = hu_send_pack(state, &pack);
            state->wait->sendtm = (ret > 0) ? tm_get_time() : 0;
            if((pack.cmd == HU_CMD_FOTA_DLD_FINISH)||(pack.cmd == HU_CMD_FOTA_UPD_FINISH))
            {
              //  send_hu_finish_flag = 1;
                wait_huconut = 0;
                log_e(LOG_FOTONHU, "download and upgrade finish need wait done ack!");
            } 
            else
            {
                wait_huconut++;
            }
            
            return 0;
        }

        if (state->wait->done_proc)
        {
            state->wait->done_proc(HU_ERR_TIMEOUT);
        }
     
        state->wait = NULL;
        wait_huconut = 0;
        memset(&pack_temp, 0, sizeof(hu_pack_t));
    }

    if (state->hbtime && tm_get_time() - state->hbtime >= 6000)
    {
        log_e(LOG_FOTONHU, "heart beat timeout");
        return -1;
    }

    return 0;
}


static int hu_check_socket(hu_stat_t *state)
{
    int ret = -1;

    if (sock_status(state->socket) == SOCK_STAT_CLOSED)
    {
        static uint64_t time = 0;

        if (time == 0 || tm_get_time() - time > 5000)
        {
            time = tm_get_time();
            log_e(LOG_FOTONHU, "start to listen clients @%d", 28888);

            if ((ret = sock_listen_port(state->socket, 28888)) != 0)
            {
                log_e(LOG_FOTONHU, "listen clients @%d fail, retry later %dms", 28888, 5000);
                ret = -1;
            }
            else if ((state->hbtime = tm_get_time()) == 0)
            {
                state->hbtime = 1;
            }
        }
    }
    else if (sock_status(state->socket) == SOCK_STAT_OPENED)
    {
        if ((ret = sock_error(state->socket)) != 0)
        {
            log_e(LOG_FOTONHU, "socket error: %d", ret);
            ret = -1;
        }
    }

    return ret;
}

static int hu_callevent_cb(int event, void *par)
{
    hu_xcall_t xcall;
    TCOM_MSG_HEADER msg;


    msg.msgid    = HU_MSG_XCALL;
    msg.sender   = MPU_MID_GEELYHU;
    msg.receiver = MPU_MID_GEELYHU;
    msg.msglen   = sizeof(xcall.callevt);
    xcall.callevt = event;

    if (event == CALL_EVENT_CALLIN)
    {
        strncpy(xcall.callnum, par, sizeof(xcall.callnum) - 1);
        xcall.callnum[sizeof(xcall.callnum) - 1] = 0;
        msg.msglen += strlen(xcall.callnum) + 1;
    }
    else if (event == CALL_EVENT_HUNGUP)
    {
        xcall.callsta = *(int *)par;
        msg.msglen += sizeof(xcall.callsta);
    }

    return tcom_send_msg(&msg, &xcall);
}


static void *hu_main(void)
{
    int tcomfd;
    hu_stat_t state;

    log_o(LOG_FOTONHU, "Foton HU thread running");
    prctl(PR_SET_NAME, "Foton HU");

    if ((tcomfd = tcom_get_read_fd(MPU_MID_GEELYHU)) < 0)
    {
        log_e(LOG_FOTONHU, "get tcom pipe fail, thread exit");
        return NULL;
    }

    //memset(&state, 0, sizeof(hu_stat_t));
    state.network = 1;
    state.wait = NULL;

    if ((state.socket = sock_create("Foton HU", SOCK_TYPE_TCP)) < 0)
    {
        log_e(LOG_FOTONHU, "create socket fail, thread exit");
        return NULL;
    }

    while (1)
    {
        TCOM_MSG_HEADER msg;
        hu_msg_t msgdata;
        int res;
        int ret[4] = {0, 0, 0, 0};

        res = protocol_wait_msg(MPU_MID_GEELYHU, tcomfd, &msg, &msgdata, 200);

        if (res < 0)
        {
            log_e(LOG_FOTONHU, "unexpectedly error: %s, thread exit", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            /*
            case HU_MSG_NETWORK:
                log_i(LOG_FOTONHU, "get NETWORK message: %d", msgdata.network);

                if (state.network && !msgdata.network)
                {
                    hu_reset(&state);
                }

                state.network = msgdata.network;
                break;
            */
            case HU_MSG_XCALL:
                log_i(LOG_FOTONHU, "get XCALL message: %d", msgdata.xcall.callevt);
                hu_xcall_event_proc(&msgdata.xcall);
                break;
        }

        if ((ret[0] = hu_check_socket(&state)) < 0 ||
            (ret[1] = hu_check_recv(&state)) < 0 ||
            (ret[2] = hu_check_wait(&state)) < 0 ||
            (ret[3] = hu_check_send(&state)) < 0)
        {
            static uint64_t tick = 0;

            hu_reset(&state);

            if (tm_get_time() >= tick + 3000)
            {
                tick = tm_get_time();
                log_i(LOG_FOTONHU, "network is unusable: %d, %d, %d, %d", ret[0], ret[1], ret[2], ret[3]);
            }
        }
    }

    sock_delete(state.socket);
    return NULL;
}

int hu_register_cmd(hu_cmd_t *cmd)
{
    hu_cmd_t *tmp;

    for (tmp = hu_cmd_tbl; tmp < ARRAY_LMT(hu_cmd_tbl); tmp++)
    {
        if (tmp->cmd == 0)
        {
            memcpy(tmp, cmd , sizeof(*tmp));
            return 0;
        }
    }

    return -1;
}

int hu_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(hu_cmd_tbl, 0, sizeof(hu_cmd_tbl));
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= at_register_callevent_cb(hu_callevent_cb);
            break;
    }

    //ret |= hu_common_init(phase);
    //ret |= hu_xcall_init(phase);
    ret |= hu_fota_init(phase);
    return ret;
}

int hu_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)hu_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_FOTONHU, "create thread fail: %s", strerror(errno));
        return ret;
    }

    return 0;
}
