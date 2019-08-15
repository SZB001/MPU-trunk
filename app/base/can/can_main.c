#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "com_app_def.h"
#include "init.h"
#include "list.h"
#include "tcom_api.h"
#include "timer.h"
#include "can_api.h"
#include "can_main.h"
#include "can_dbc.h"
#include "shell_api.h"
#include "scom_api.h"
#include "pm_api.h"
#include "cfg_api.h"
#include "scom_msg_def.h"
#include "can_adp.h"

#define CAN_MAX_CALLBACK    16
#define CAN_BUS_TIMEOUT     10000 //10000   10S->120S for foton
#define CAN_MAX_RAWSIZE     10

typedef enum
{
    CAN_MSG_TIMEOUT = MPU_MID_CAN,
    CAN_MSG_TAGEMU,
    CAN_MSG_SETDBC,
    CAN_MSG_SET_BAUD,
    CAN_MSG_SET_DEFAULT_BAUD,
    CAN_MSG_STAT,
    CAN_MSG_SYNC_BAUD,
    CAN_MSG_SYNC_DEFAULT_BAUD,
    CAN_MSG_STOP,
    CAN_MSG_RUN,
} CAN_MSG_TYPE;

typedef union
{
    uint8_t  rawdat[TCOM_MAX_MSG_LEN];
    uint64_t ustime;
    char     dbcpath[256];
    short    setup[2];
    short    baudlist[CAN_MAX_PORT];
} can_msg_t;

typedef struct
{
    int comfd;
    int canfd;
    int sleep;
    int active;
    int rawpos[CAN_MAX_PORT];
    CAN_MSG rawmsg[CAN_MAX_PORT][CAN_MAX_RAWSIZE];
    timer_t tm;
    timer_t tagemu;
    uint32_t uptime;
    int stop;
} can_stat_t;

static can_cb_t can_cb_lst[CAN_MAX_CALLBACK];
static short can_baud[CAN_MAX_PORT];
static short can_default_baud[CAN_MAX_PORT];
static int can_sleep;
static uint32_t can_basetime;
static uint32_t can_uptime[CAN_MAX_PORT];
static pthread_mutex_t can_time_mutex;

uint32_t can_sum = 0;

short  get_can2_config_baud(void)
{
    return can_baud[1];
}

static void can_do_callback(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    int i;

    dbc_can_callback(event, arg1, arg2);

    for (i = 0; i < CAN_MAX_CALLBACK && can_cb_lst[i]; i++)
    {
		if(can_cb_lst[i] != NULL)
		{
			can_cb_lst[i](event, arg1, arg2);
		}
    }
}

static void can_do_dumpsta(can_stat_t *state)
{
    int i, j, pos, no;

    shellprintf(" CAN status: %s\r\n",
                state->stop ? "disabled" : (state->active ? "receiving" : "no data"));

    for (i = 0; i < CAN_MAX_PORT; i++)
    {
        #if 0
        if (can_baud[i] < 0)
        {
            shellprintf(" CAN %d: N/A\r\n", i + 1);
        }
        else
        {
            shellprintf(" CAN %d: %uk\r\n", i + 1, can_baud[i]);
        }
        #endif

        pos = state->rawpos[i];

        for (no = 0, j = 0; j < CAN_MAX_RAWSIZE; j++, pos = (pos + 1) % CAN_MAX_RAWSIZE)
        {
            CAN_MSG *msg = &state->rawmsg[i][pos];

            if (msg->msgid != 0)
            {
                unsigned char m;
                
                shellprintf("  %2d  0x%08X %c:", ++no, msg->msgid, msg->exten ? 'E' : 'S');  

                for(m=0; m<msg->len; m++)
                {
                    if((m%8) == 7)
                    {
                        shellprintf("%2x ", msg->Data[m]);
                        shellprintf("\r\n");
                    }
                    else if(m && ((m%8)==0))
                    {
                        shellprintf("%21x ", msg->Data[m]);
                    }
                    else
                    {
                        shellprintf("%2x ", msg->Data[m]);
                    }
                }

                if(msg->len%8)
                {
                    shellprintf("\r\n");
                }
            }
        }
    }
 
  shellprintf(" --------------------------------dataover--------------------------------\r\n");
}

static void can_do_timeout(can_stat_t *state)
{
    if (state->active)
    {
        state->active = 0;
        memset(state->rawmsg, 0, sizeof(state->rawmsg));
        log_e(LOG_CAN, "can bus time out");
    }

    if (!state->sleep)
    {
        can_do_callback(CAN_EVENT_TIMEOUT, state->uptime, 0);
    }
}

static void can_do_sleep(can_stat_t *state)
{
    if (!state->sleep)
    {
        if (state->active)
        {
            tm_stop(state->tm);
            state->active = 0;
            memset(state->rawmsg, 0, sizeof(state->rawmsg));
        }

        if (state->canfd >= 0)
        {
            close(state->canfd);
            state->canfd = -1;
        }

        can_do_callback(CAN_EVENT_SLEEP, 0, 0);
        state->sleep = 1;
        can_sleep = 1;
        log_e(LOG_CAN, "can is sleeping");
    }
}

static void can_do_wakeup(can_stat_t *state)
{
    if (state->sleep)
    {
        can_do_callback(CAN_EVENT_WAKEUP, 0, 0);
        state->sleep = 0;
        can_sleep = 0;
        log_e(LOG_CAN, "can is waken up");
    }
}


static int can_putraw(can_stat_t *state, CAN_MSG *msglst, int msgcnt)
{
    int rawcnt;
    CAN_MSG *msg;
    uint32_t uptime = tm_get_time();

    for (msg = msglst, rawcnt = 0; msg < msglst + msgcnt; msg++)
    {
        if (msg->type == 'C')
        {
            if (msg->port > CAN_MAX_PORT)
            {
                log_e(LOG_CAN, "can port error, port=%d, len=%d", msg->port, msg->len);
                continue;
            }

            memcpy(state->rawmsg[RPORT(msg->port)] + state->rawpos[RPORT(msg->port)],
                   msg, sizeof(CAN_MSG));
            state->rawpos[RPORT(msg->port)] = (state->rawpos[RPORT(msg->port)] + 1) % CAN_MAX_RAWSIZE;
            rawcnt++;
			can_sum++;
            can_uptime[RPORT(msg->port)] = uptime;
        }
    }

    return rawcnt;
}

static void can_do_datain(can_stat_t *state, CAN_MSG *msglst, int msgcnt)
{
    CAN_MSG *msg, *first;

    if (!state->stop && can_putraw(state, msglst, msgcnt) > 0)
    {
        tm_start(state->tm, CAN_BUS_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);

        if (!state->active)
        {
            state->active = 1;
            log_e(LOG_CAN, "can bus active");
            can_do_callback(CAN_EVENT_ACTIVE, 0, 0);
        }
    }

    for (msg = msglst, first = msglst; msg < msglst + msgcnt; msg++)
    {
        if (msg->type == 'T')
        {
            pthread_mutex_lock(&can_time_mutex);
            can_basetime = msg->data32[0];
            pthread_mutex_unlock(&can_time_mutex);
            state->uptime = msg->uptime;

            if (state->stop)
            {
                can_do_callback(CAN_EVENT_DATAIN, (uint32_t)msg, 1);
            }
            else
            {
                can_do_callback(CAN_EVENT_DATAIN, (uint32_t)first, msg - first + 1);
            }

            first = msg + 1;
        }
    }

    if (!state->stop && first != msg)
    {
        state->uptime = msg[-1].uptime;
        can_do_callback(CAN_EVENT_DATAIN, (uint32_t)first, msg - first);
    }
}

static void can_do_receive(can_stat_t *state)
{
    static CAN_MSG msglst[4096];
    int len;

    if (state->canfd < 0)
    {
        state->canfd = open("/dev/spichn_0", O_RDWR | O_NONBLOCK | O_NOCTTY);

        if (state->canfd < 0)
        {
            log_e(LOG_CAN, "open spi can channel fail: %s", strerror(errno));
            return;
        }

        tm_start(state->tm, CAN_BUS_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);
    }

    if ((len = read(state->canfd, msglst, sizeof(msglst))) > 0)
    {
        can_do_datain(state, msglst, len / sizeof(CAN_MSG));
    }
}

int can_do_send(unsigned char port, CAN_SEND_MSG *msg)
{
    unsigned char temp[1+sizeof(CAN_SEND_MSG)];

    temp[0] = port;
    memcpy(&temp[1],msg,sizeof(CAN_SEND_MSG));

    return scom_tl_send_frame(SCOM_TL_CMD_CAN_SEND, SCOM_TL_SINGLE_FRAME, 0,
                                               (uint8_t *)temp, sizeof(CAN_SEND_MSG)+1);
}

static void can_do_change_dbc(can_stat_t *state, const char *fpath)
{
    short baud[CAN_MAX_PORT], i;

    for (i = 0; i < CAN_MAX_PORT; i++)
    {
        baud[i] = -1;
    }

    if (dbc_load_file(fpath, baud))
    {
        log_e(LOG_CAN, "change dbc file fail");
    }
    else
    {
        for (i = 0; i < CAN_MAX_PORT; i++)
        {
            if (baud[i] >= 0)
            {
                can_baud[i] = baud[i];

                if (can_default_baud[i] != baud[i])
                {
                    can_default_baud[i] = baud[i];
                    can_set_default_baud(i, can_default_baud[i]);
                }
            }
            else
            {
                can_baud[i] = -1;

                if (can_default_baud[i] != 0)
                {
                    can_default_baud[i] = 0;
                    can_set_default_baud(i, can_default_baud[i]);
                }
            }
        }
    }
}

static int can_shell_setdbc(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: cansetdbc <file path>\r\n");
        return -1;
    }

    if (can_set_dbc(argv[0]))
    {
        shellprintf(" error: set dbc fail\r\n");
        return -2;
    }

    shellprintf(" setdbc ok\r\n");
    sleep(1);

    return 0;
}

static int can_shell_setbaud(int argc, const char **argv)
{
    uint32_t port;
    uint32_t baud;

    if (argc != 2 || sscanf(argv[0], "%u", &port) != 1 ||
        sscanf(argv[1], "%u", &baud) != 1)
    {
        shellprintf(" usage: cansetbuad <port> <baud>\r\n");
        return -1;
    }

    if (port == 0 || port > CAN_MAX_PORT)
    {
        shellprintf(" error: port %u is not supported\r\n", port);
        return -1;
    }

    if (can_set_baud(port - 1, baud))
    {
        shellprintf(" error: set baudrate fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_set_default_baud(int argc, const char **argv)
{
    uint32_t port;
    uint32_t baud;

    if (argc != 2 || sscanf(argv[0], "%u", &port) != 1 ||
        sscanf(argv[1], "%u", &baud) != 1)
    {
        shellprintf(" usage: cansetdfbuad <port> <baud>\r\n");
        return -1;
    }

    if (port == 0 || port > CAN_MAX_PORT)
    {
        shellprintf(" error: port %u is not supported\r\n", port);
        return -1;
    }

    if (can_set_default_baud(port - 1, baud))
    {
        shellprintf(" error: set default baudrate fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_sethcan(int argc, const char **argv)
{
    uint32_t baud;

    if (argc != 1 || sscanf(argv[0], "%u", &baud) != 1)
    {
        shellprintf(" usage: canseth <baud>\r\n");
        return -1;
    }

    if (1 > CAN_MAX_PORT)
    {
        shellprintf(" error: port 1 is not supported\r\n");
        return -1;
    }

    if (can_set_baud(0, baud))
    {
        shellprintf(" error: set baudrate fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_setmcan(int argc, const char **argv)
{
    uint32_t baud;

    if (argc != 1 || sscanf(argv[0], "%u", &baud) != 1)
    {
        shellprintf(" usage: cansetm <baud>\r\n");
        return -1;
    }

    if (2 > CAN_MAX_PORT)
    {
        shellprintf(" error: port 2 is not supported\r\n");
        return -1;
    }

    if (can_set_baud(1, baud))
    {
        shellprintf(" error: set baudrate fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_setlcan(int argc, const char **argv)
{
    uint32_t baud;

    if (argc != 1 || sscanf(argv[0], "%u", &baud) != 1)
    {
        shellprintf(" usage: cansetl <baud>\r\n");
        return -1;
    }

    if (3 > CAN_MAX_PORT)
    {
        shellprintf(" error: port 3 is not supported\r\n");
        return -1;
    }

    if (can_set_baud(2, baud))
    {
        shellprintf(" error: set baudrate fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_canstat(int argc, const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_STAT;
    msg.msglen   = 0;
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, NULL) != 0)
    {
        shellprintf(" error: send tcom message fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_cansend(int argc, const char **argv)
{
    unsigned int op;
    typedef struct
    {
        unsigned char rtype; /* see also CAN_REPORT_TYPE */
        unsigned char ftype; /* see also CAN_FRAME_TYPE */
        unsigned char port;
        unsigned char dlc;
        unsigned int id;
        unsigned char data[8];
        unsigned int val;
        unsigned int times;
    } CAN_REPORT_CFG;

    CAN_REPORT_CFG cfg;
    memset(&cfg, 0, sizeof(cfg));

    if (argc != 1 || sscanf(argv[0], "%u", &op) != 1)
    {
        shellprintf(" usage: cansend <op>\r\n");
        shellprintf(" op: 0 for stop; 1 for start\r\n");
        return -1;
    }

    cfg.ftype = 0;
    cfg.rtype = op;
    cfg.port = 0;
    cfg.dlc = 8;
    cfg.id = 0x101;
    cfg.times = 1;
    cfg.val = 1000;
    memcpy(cfg.data, "12345678", sizeof(cfg.data));

    return scom_tl_send_frame(SCOM_TL_CMD_CAN_SEND_CFG, SCOM_TL_SINGLE_FRAME, 0,
                              (unsigned char *) &cfg, sizeof(cfg));

}


static int can_shell_canstop(int argc, const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_STOP;
    msg.msglen   = 0;
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, NULL) != 0)
    {
        shellprintf(" error: send tcom message fail\r\n");
        return -2;
    }

    return 0;
}

static int can_shell_canrun(int argc, const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_RUN;
    msg.msglen   = 0;
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, NULL) != 0)
    {
        shellprintf(" error: send tcom message fail\r\n");
        return -2;
    }

    return 0;
}

static int can_sleep_handler(PM_EVT_ID id)
{
    return can_sleep;
}

int can_scom_msg_proc(unsigned char *msg, unsigned int len)
{
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;

    if (len < sizeof(SCOM_TL_MSG_HDR))
    {
        log_e(LOG_CAN, "invalid message,len:%u", len);
        return -1;
    }

    if (((tl_hdr->msg_type & 0xf0) >> 4) != SCOM_TL_CHN_CAN)
    {
        log_e(LOG_CAN, "invalid message,msgtype:%u, fct:%u", tl_hdr->msg_type, SCOM_TL_CHN_CAN);
        return -1;
    }

    switch (tl_hdr->msg_type)
    {
        case SCOM_TL_CMD_CAN_BAUD:
            can_sync_baud((short *)&msg[sizeof(SCOM_TL_MSG_HDR)]);
            break;

        case SCOM_TL_CMD_CAN_DEFAULT_BAUD:
            can_sync_default_baud((short *)&msg[sizeof(SCOM_TL_MSG_HDR)]);
            break;

        case SCOM_TL_CMD_CAN_AUTO_BAUD:
            can_sync_autobaud_from_mcu((short *)&msg[sizeof(SCOM_TL_MSG_HDR)]);
            break;

        case SCOM_TL_CMD_CAN_FILER_CANID:
            dbc_canid_check(tl_hdr->fno, (unsigned char *)&msg[sizeof(SCOM_TL_MSG_HDR)]);
            break;

        default:
            log_e(LOG_CAN, "invalid message,msgtype:%u", tl_hdr->msg_type);
            break;
    }

    return 0;
}

int can_init(INIT_PHASE phase)
{
    int  ret = 0, i;
    short *pbaud = can_default_baud;
    uint32_t len;

    ret |= dbc_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(can_cb_lst, 0, sizeof(can_cb_lst));
            can_sleep = 0;
            can_basetime = 0xffffffff;
            pthread_mutex_init(&can_time_mutex, NULL);

            for (i = 0; i < CAN_MAX_PORT; i++)
            {
                can_baud[i] = (short) - 1;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:

            len = sizeof(short);
            ret |= cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_0, (void *)pbaud++, &len);
            len = sizeof(short);
            ret |= cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_1, (void *)pbaud++, &len);
            len = sizeof(short);
            ret |= cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_2, (void *)pbaud++, &len);
            len = sizeof(short);
            ret |= cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_3, (void *)pbaud++, &len);
            len = sizeof(short);
            ret |= cfg_get_para(CFG_ITEM_CAN_DEFAULT_BAUD_4, (void *)pbaud++, &len);

            ret |= can2_init_read_autobaud();

            ret |= pm_reg_handler(MPU_MID_CAN, can_sleep_handler);
            ret |= shell_cmd_register_ex("drdbc", "setdbc", can_shell_setdbc, "set dbc file path");
            ret |= shell_cmd_register_ex("cansetbaud", NULL, can_shell_setbaud, "set can baudrate");
            ret |= shell_cmd_register_ex("cansetdfbaud", NULL, can_shell_set_default_baud,
                                         "set default can baudrate");
            ret |= shell_cmd_register_ex("canseth", "sethcan", can_shell_sethcan, "set baudrate for can 1");
            ret |= shell_cmd_register_ex("cansetm", "setmcan", can_shell_setmcan, "set baudrate for can 2");
            ret |= shell_cmd_register_ex("cansetl", "setlcan", can_shell_setlcan, "set baudrate for can 3");
            ret |= shell_cmd_register_ex("cansta", "rawcan", can_shell_canstat, "show can status");
            ret |= shell_cmd_register_ex("cansend", NULL, can_shell_cansend, "send can data");
            ret |= shell_cmd_register_ex("canstop", NULL, can_shell_canstop, "disable can receiving");
            ret |= shell_cmd_register_ex("canrun", NULL, can_shell_canrun, "enable can receiving");

            ret |= scom_tl_reg_proc_fun(SCOM_TL_CHN_CAN, can_scom_msg_proc);

            if (ret != 0)
            {
                log_e(LOG_CAN, "reg scom proc failed, ret:0x%08x", ret);
                return ret;
            }

            break;

        default:
            break;
    }



    return ret;
}



static void *can_main(void)
{
    int res;
    can_stat_t state;

    prctl(PR_SET_NAME, "CAN");
    memset(&state, 0, sizeof(state));


    if ((state.comfd = tcom_get_read_fd(MPU_MID_CAN)) < 0)
    {
        log_e(LOG_CAN, "get module pipe fail, thread exit");
        return NULL;
    }

    if (tm_create(TIMER_REL, CAN_MSG_TIMEOUT, MPU_MID_CAN, &state.tm) ||
        tm_create(TIMER_REL, CAN_MSG_TAGEMU, MPU_MID_CAN, &state.tagemu))
    {
        log_e(LOG_CAN, "create timer fail, thread exit");
        return NULL;
    }

    state.canfd = -1;
    tm_start(state.tagemu, 10, TIMER_TIMEOUT_REL_PERIOD);

    while (1)
    {
        TCOM_MSG_HEADER msg;
        can_msg_t msgdata;
        fd_set set;

        FD_ZERO(&set);
        FD_SET(state.comfd, &set);

        res = select(state.comfd + 1, &set, NULL, NULL, NULL);

        if (res > 0)
        {
            if (FD_ISSET(state.comfd, &set) &&
                tcom_recv_msg(MPU_MID_CAN, &msg, msgdata.rawdat) == 0)
            {
                log_i(LOG_CAN, "get a message, sender=0x%04x, msgid=0x%08x, msglen=%u!",
                      msg.sender, msg.msgid, msg.msglen);
                log_buf_dump(LOG_CAN, "tcom message", msgdata.rawdat, msg.msglen);

                switch (msg.msgid)
                {
                    case CAN_MSG_SYNC_BAUD:
                        if (memcmp(can_baud, msgdata.baudlist, sizeof(can_baud)) != 0)
                        {
                            scom_tl_send_frame(SCOM_TL_CMD_CAN_BAUD, SCOM_TL_SINGLE_FRAME, 0,
                                               (uint8_t *)can_baud, sizeof(can_baud));
                        }

                        break;

                    case CAN_MSG_SYNC_DEFAULT_BAUD:
                        if (memcmp(can_default_baud, msgdata.baudlist, sizeof(can_default_baud)) != 0)
                        {
                            scom_tl_send_frame(SCOM_TL_CMD_CAN_DEFAULT_BAUD, SCOM_TL_SINGLE_FRAME, 0,
                                               (uint8_t *)can_default_baud, sizeof(can_default_baud));
                        }

                        break;

                    case CAN_MSG_SET_BAUD:
                        can_baud[msgdata.setup[0]] = msgdata.setup[1];
                        break;

                    case CAN_MSG_SET_DEFAULT_BAUD:
                        can_default_baud[msgdata.setup[0]] = msgdata.setup[1];
                        cfg_set_para(CFG_ITEM_CAN_DEFAULT_BAUD_0 + msgdata.setup[0],
                                     msgdata.setup + 1, sizeof(msgdata.setup[1]));
                        break;

                    case PM_MSG_SLEEP:
                    case PM_MSG_EMERGENCY:
                    case PM_MSG_OFF:
                        can_do_sleep(&state);
                        break;

                    case PM_MSG_RUNNING:
                        can_do_wakeup(&state);
                        break;

                    case CAN_MSG_TIMEOUT:
                        can_do_timeout(&state);
                        break;

                    case CAN_MSG_SETDBC:
                        can_do_change_dbc(&state, msgdata.dbcpath);
                        break;

                    case CAN_MSG_STAT:
                        can_do_dumpsta(&state);
                        break;

                    case CAN_MSG_STOP:
                        state.stop = 1;
                        break;

                    case CAN_MSG_RUN:
                        state.stop = 0;
                        break;

                    case MPU_MID_MID_PWDG:
                        pwdg_feed(MPU_MID_CAN);
                        break;

                    default:
                        break;

                }
            }

            if (!state.sleep)
            {
                can_do_receive(&state);
            }

        }
        else if (res < 0 && EINTR != errno) /* timeout */
        {
            log_e(LOG_CAN, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }
    }

    return NULL;
}

int can_run(void)
{
    int ret;
    pthread_t can_tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&can_tid, &ta, (void *)can_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}

int can_register_callback(can_cb_t cb)
{
    int i = 0;

    while (i < CAN_MAX_CALLBACK && can_cb_lst[i] != NULL)
    {
        i++;
    }

    if (i >= CAN_MAX_CALLBACK)
    {
        log_e(LOG_CAN, "no space for new callback");
        return -1;
    }

    can_cb_lst[i] = cb;
    return 0;
}

int can_set_dbc(const char *fpath)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_SETDBC;
    msg.msglen   = strlen(fpath) + 1;
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, fpath) != 0)
    {
        log_e(LOG_CAN, "send tcom message fail");
        return -2;
    }

    return 0;
}

int can_get_time(uint32_t uptime, RTCTIME *time)
{
    struct tm *ltime;
    uint32_t lbase;

    pthread_mutex_lock(&can_time_mutex);
    lbase = can_basetime;
    pthread_mutex_unlock(&can_time_mutex);

    if (lbase == 0xffffffff)
    {
        return tm_get_abstime(time);
    }

    lbase += uptime / 100;
    ltime = localtime((time_t *)&lbase);
    time->year = ltime->tm_year + 1900;
    time->mon  = ltime->tm_mon + 1;
    time->mday = ltime->tm_mday;
    time->hour = ltime->tm_hour;
    time->min  = ltime->tm_min;
    time->sec  = ltime->tm_sec;

    return 0;
}

int can_time_ready(void)
{
    uint32_t lbase;

    pthread_mutex_lock(&can_time_mutex);
    lbase = can_basetime;
    pthread_mutex_unlock(&can_time_mutex);

    return lbase != 0xffffffff;
}

int can_sync_baud(short *baudlist)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_SYNC_BAUD;
    msg.msglen   = CAN_MAX_PORT * sizeof(short);
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, baudlist) != 0)
    {
        log_e(LOG_CAN, "send tcom message fail");
        return -2;
    }

    return 0;
}

int can_sync_default_baud(short *baudlist)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = CAN_MSG_SYNC_DEFAULT_BAUD;
    msg.msglen   = CAN_MAX_PORT * sizeof(short);
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, baudlist) != 0)
    {
        log_e(LOG_CAN, "send tcom message fail");
        return -2;
    }

    return 0;
}


int can_set_baud(int port, short baud)
{
    short setup[2];
    TCOM_MSG_HEADER msg;

    if (port < 0 || port >= CAN_MAX_PORT || baud < 0)
    {
        return -1;
    }

    setup[0] = port;
    setup[1] = baud;

    msg.msgid    = CAN_MSG_SET_BAUD;
    msg.msglen   = sizeof(setup);
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, setup) != 0)
    {
        log_e(LOG_CAN, "send tcom message fail");
        return -2;
    }

    return 0;
}

short can_get_baud(int port)
{
    if (port < 0 || port >= CAN_MAX_PORT)
    {
        return -1;
    }

    return can_baud[port];
}

uint32_t can_get_uptime(int port)
{
    if (port < 0 || port >= CAN_MAX_PORT)
    {
        return 0;
    }

    return can_uptime[port];
}

int can_set_default_baud(int port, short baud)
{
    short setup[2];
    TCOM_MSG_HEADER msg;

    if (port < 0 || port >= CAN_MAX_PORT || baud < 0)
    {
        return -1;
    }

    setup[0] = port;
    setup[1] = baud;

    msg.msgid    = CAN_MSG_SET_DEFAULT_BAUD;
    msg.msglen   = sizeof(setup);
    msg.sender   = MPU_MID_CAN;
    msg.receiver = MPU_MID_CAN;

    if (tcom_send_msg(&msg, setup) != 0)
    {
        log_e(LOG_CAN, "send tcom message fail");
        return -2;
    }

    return 0;
}

void can_baud_reset(void)
{
    memset((void *)&can_default_baud, 0, sizeof(can_default_baud));
    memset((void *)&can_baud, 0xff, sizeof(can_baud));
}

