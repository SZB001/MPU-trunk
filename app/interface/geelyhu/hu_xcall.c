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
#include "dev_api.h"
#include "at.h"
#include "at_api.h"
#include "shell_api.h"
#include "hu.h"


typedef enum
{
    HU_XCALL_TYPE_NONE,
    HU_XCALL_TYPE_RESERVED,
    HU_XCALL_TYPE_BCALL,
    HU_XCALL_TYPE_ECALL,
} HU_XCALL_TYPE;

typedef enum
{
    HU_XCALL_COMM_CALLSTART,
    HU_XCALL_COMM_INCOMING,
    HU_XCALL_COMM_RECALL,
    HU_XCALL_COMM_FAILED,
    HU_XCALL_COMM_CALLING,
    HU_XCALL_COMM_ONLINE,
    HU_XCALL_COMM_DATASEND,
    HU_XCALL_COMM_CALLEND,
    HU_XCALL_COMM_IDLE,
    HU_XCALL_COMM_MUTEON,
    HU_XCALL_COMM_MUTEOFF,
    HU_XCALL_COMM_MAX,
} HU_XCALL_COMM_STATE;

typedef enum
{
    HU_XCALL_STATE_IDLE,
    HU_XCALL_STATE_ONLINE,
    HU_XCALL_STATE_CALLING,
    HU_XCALL_STATE_HANGING,
} HU_XCALL_STATE;

typedef enum
{
    HU_XCALL_SERV_NONE,
    HU_XCALL_SERV_CALL,
    HU_XCALL_SERV_HANGUP,
} HU_XCALL_REQ;

static unsigned comm_wait;
static unsigned comm_stat;
static unsigned call_type;
static unsigned call_stat;
static unsigned call_cancel;
static unsigned call_retry;
static unsigned call_request;
static unsigned call_service;
static pthread_mutex_t call_mtx;
static char     call_num[4][32] =
{
    "",
    "",
    "",
    "",
};

static unsigned comm_next[] =
{
    HU_XCALL_COMM_MUTEON,       /* after sending call-start */
    HU_XCALL_COMM_IDLE,         /* after sending incomming */
    HU_XCALL_COMM_CALLING,      /* after sending re-call */
    HU_XCALL_COMM_IDLE,         /* after sending failed */
    HU_XCALL_COMM_IDLE,         /* after sending calling */
    HU_XCALL_COMM_IDLE,         /* after sending connected */
    HU_XCALL_COMM_CALLSTART,    /* after sending data-sending */
    HU_XCALL_COMM_MUTEOFF,      /* after sending call-end */
    HU_XCALL_COMM_DATASEND,     /* starting with this */
    HU_XCALL_COMM_CALLING,      /* after sending mute-on */
    HU_XCALL_COMM_IDLE,         /* after sending mute-off */
};


void hu_xcall_event_proc(hu_xcall_t *call)
{
    switch (call->callevt)
    {
        case CALL_EVENT_CALLIN:
            break;

        case CALL_EVENT_ONLINE:
            if (call_stat == HU_XCALL_STATE_CALLING)
            {
                call_stat = HU_XCALL_STATE_ONLINE;

                /*
                if communication state is not IDLE, that means current message
                is not done, so we do nothing to wait it done, else we send a
                ONLINE message.
                */
                if (comm_stat == HU_XCALL_COMM_IDLE)
                {
                    comm_stat = HU_XCALL_COMM_ONLINE;
                }
            }

            /*
            if call state is not CALLING, that means we trying to cancel it
            somewhere and a HUNGUP evnet should be sent later, so we just
            ignore this event.
            */
            break;

        case CALL_EVENT_CALLFAIL:
            if (call_stat == HU_XCALL_STATE_CALLING)
            {
                call_stat = HU_XCALL_STATE_IDLE;

                /*
                if communication state is not IDLE, that means current message
                is not done, so we do nothing to wait it done, else we send a
                FAILED or RECALL message.
                */
                if (comm_stat == HU_XCALL_COMM_IDLE)
                {
                    comm_stat = call_retry ? HU_XCALL_COMM_FAILED : HU_XCALL_COMM_RECALL;
                }
            }
            /*
            if call state is not HANGING, that means we trying to cancel it
            somewhere, this event indicates the canceling is done, whatever by
            ourself or otheres, we just need send a CALL-END message
            */
            else if (call_stat == HU_XCALL_STATE_HANGING)
            {
                call_stat = HU_XCALL_STATE_IDLE;
                comm_stat = call_cancel ? HU_XCALL_COMM_IDLE : HU_XCALL_COMM_CALLEND;
                call_cancel = 0;
            }
            /*
            other cases should never be happened
            */
            else
            {
                log_e(LOG_FOTONHU, "BIG BUG: incorrect call state when get a CALLFAIL event");
            }

            break;

        case CALL_EVENT_HUNGUP:

            /*
            this event means the call is hungup by ourself or other side when
            call is online(not call state is ONLINE), it should be happened
            when call state is ONLINE(hungup by other side) or HANGING(hungup
            by ourself), we need send a CALL-END message
            */
            if (call_stat == HU_XCALL_STATE_ONLINE ||
                call_stat == HU_XCALL_STATE_HANGING)
            {
                call_stat = HU_XCALL_STATE_IDLE;
                comm_stat = call_cancel ? HU_XCALL_COMM_IDLE : HU_XCALL_COMM_CALLEND;
                call_cancel = 0;
            }
            /*
            other cases should never be happened
            */
            else
            {
                log_e(LOG_FOTONHU, "BIG BUG: incorrect call state when get a HUNGUP event");
            }

            break;

        default:
            break;
    }
}


static int hu_xcall_do_service(int service)
{
    if (service == HU_XCALL_SERV_HANGUP)
    {
        switch (comm_stat)
        {
            case HU_XCALL_COMM_DATASEND:
            case HU_XCALL_COMM_CALLSTART:
            case HU_XCALL_COMM_MUTEON:
            case HU_XCALL_COMM_RECALL:
                comm_stat = HU_XCALL_COMM_CALLEND;
                break;

            case HU_XCALL_COMM_CALLING:
            case HU_XCALL_COMM_ONLINE:
                at_hang_call();
                comm_stat = HU_XCALL_COMM_IDLE;
                call_stat = HU_XCALL_STATE_HANGING;
                break;

            case HU_XCALL_COMM_CALLEND:
                comm_stat = HU_XCALL_COMM_MUTEOFF;
                break;

            case HU_XCALL_COMM_FAILED:
            case HU_XCALL_COMM_MUTEOFF:
            case HU_XCALL_COMM_IDLE:
                comm_stat = HU_XCALL_COMM_IDLE;
                service = HU_XCALL_SERV_NONE;
                break;
        }
    }
    else if (service == HU_XCALL_SERV_CALL)
    {
        switch (comm_stat)
        {
            case HU_XCALL_COMM_IDLE:
                call_retry = 0;
                comm_stat = HU_XCALL_COMM_DATASEND;
                break;

            case HU_XCALL_COMM_RECALL:
                call_retry++;

            case HU_XCALL_COMM_MUTEON:
                at_make_call(call_num[call_type]);
                call_stat = HU_XCALL_STATE_CALLING;

            case HU_XCALL_COMM_DATASEND:
            case HU_XCALL_COMM_CALLSTART:
            case HU_XCALL_COMM_ONLINE:
            case HU_XCALL_COMM_CALLEND:
                comm_stat = comm_next[comm_stat];
                break;

            case HU_XCALL_COMM_CALLING:

                /* we are online, send a ONLINE message */
                if (call_stat == HU_XCALL_STATE_ONLINE)
                {
                    comm_stat = HU_XCALL_COMM_ONLINE;
                }
                /* the calling is failed, send a FAILED or RECALL message */
                else if (call_stat == HU_XCALL_STATE_IDLE)
                {
                    comm_stat = call_retry ? HU_XCALL_COMM_FAILED : HU_XCALL_COMM_RECALL;
                }
                /* it's still calling, just wait a moment for event */
                else if (call_stat == HU_XCALL_STATE_CALLING)
                {
                    comm_stat = HU_XCALL_COMM_IDLE;
                }
                /* otherwise should never be happened */
                else
                {
                    log_e(LOG_FOTONHU, "BIG BUG: incorrect call state when CALLING message is done");
                }

                break;

            case HU_XCALL_COMM_FAILED:
            case HU_XCALL_COMM_MUTEOFF:
                comm_stat = HU_XCALL_COMM_IDLE;
                service = HU_XCALL_SERV_NONE;
                break;
        }
    }

    return service;
}

static int hu_xcall_do_cancel(void)
{
    switch (comm_stat)
    {
        case HU_XCALL_COMM_CALLING:
        case HU_XCALL_COMM_ONLINE:
            at_hang_call();
            call_stat = HU_XCALL_STATE_HANGING;
            call_cancel = 1;
            break;

        case HU_XCALL_COMM_DATASEND:
        case HU_XCALL_COMM_CALLSTART:
        case HU_XCALL_COMM_MUTEON:
        case HU_XCALL_COMM_RECALL:
        case HU_XCALL_COMM_FAILED:
        case HU_XCALL_COMM_CALLEND:
        case HU_XCALL_COMM_MUTEOFF:
        case HU_XCALL_COMM_IDLE:
            break;
    }

    comm_stat = HU_XCALL_COMM_IDLE;

    return call_cancel;
}

int hu_xcall_start(int type)
{
    if (call_request != HU_XCALL_SERV_NONE ||
        call_service == HU_XCALL_SERV_CALL)
    {
        return HU_ERR_DENIED;
    }

    call_type = type;

    if (call_service == HU_XCALL_SERV_NONE)
    {
        pthread_mutex_lock(&call_mtx);
        call_service = hu_xcall_do_service(HU_XCALL_SERV_CALL);
        pthread_mutex_unlock(&call_mtx);
    }
    else
    {
        call_request = HU_XCALL_SERV_CALL;
    }

    return 0;
}

int hu_xcall_hangup(void)
{
    if (call_request != HU_XCALL_SERV_NONE ||
        call_service != HU_XCALL_SERV_CALL)
    {
        return HU_ERR_DENIED;
    }

    call_request = HU_XCALL_SERV_HANGUP;
    return 0;
}

static void hu_xcall_done(int error)
{
    comm_wait = 0;

    pthread_mutex_lock(&call_mtx);

    if (!error)
    {
        if (call_request == HU_XCALL_SERV_HANGUP ||
            hu_xcall_do_service(call_service) == HU_XCALL_SERV_NONE)
        {
            call_service = hu_xcall_do_service(call_request);
            call_request = HU_XCALL_SERV_NONE;
        }
    }
    else
    {
        if (hu_xcall_do_cancel() == 0)
        {
            call_service = hu_xcall_do_service(call_request);
            call_request = HU_XCALL_SERV_NONE;
        }
    }

    pthread_mutex_unlock(&call_mtx);
}

static int hu_xcall_send(hu_pack_t *pack)
{
    if (comm_wait)
    {
        return HU_ERR_DENIED;
    }
    else if (comm_stat < HU_XCALL_COMM_IDLE)
    {
        if (sizeof(pack->dat) < 3)
        {
            return HU_ERR_DENIED;
        }

        pack->len = 3;
        pack->dat[0] = call_type;
        pack->dat[1] = comm_stat;
        pack->dat[2] = 0;
    }
    else if (comm_stat > HU_XCALL_COMM_IDLE)
    {
        if (sizeof(pack->dat) < 1)
        {
            return HU_ERR_DENIED;
        }

        pack->len = 1;
        pack->dat[0] = comm_stat == HU_XCALL_COMM_MUTEON ? 0 : 1;
    }
    else
    {
        return HU_ERR_DENIED;
    }

    comm_wait = 1;
    return 0;
}

static int hu_xcall_send_ctl(hu_pack_t *pack)
{
    pack->len = 0;
    return 0;
}

static int hu_xcall_recv_ctl(hu_pack_t *pack)
{
    if (pack->len != 2)
    {
        return HU_ERR_CHECKSUM;
    }

    if (pack->dat[0] == 0)
    {
        return hu_xcall_hangup();
    }

    return hu_xcall_start(pack->dat[1]);
}

static int hu_xcall_send_query(hu_pack_t *pack)
{
    int stat;

    if (sizeof(pack->dat) < 3)
    {
        return HU_ERR_DENIED;
    }

    if (comm_stat == HU_XCALL_COMM_MUTEON)
    {
        stat = HU_XCALL_COMM_CALLSTART;
    }
    else if (comm_stat == HU_XCALL_COMM_MUTEOFF)
    {
        stat = HU_XCALL_COMM_CALLEND;
    }
    else
    {
        stat = comm_stat;
    }

    pack->len = 3;
    pack->dat[0] = call_type;
    pack->dat[1] = stat;
    pack->dat[2] = 0;

    return 0;
}

static int hu_xcall_set_callnum(int argc, const char **argv)
{
    int type;

    if (argc != 2 || argv[0][1] != 0 || argv[0][0] != 'e' || argv[0][0] != 'b')
    {
        shellprintf(" usage: husetcallnum <type, 'e' or 'b'> <number>\r\n");
        return -1;
    }

    if (strlen(argv[1]) > 31)
    {
        shellprintf(" error: call number is too long(maximum 31)\r\n");
        return -2;
    }

    pthread_mutex_lock(&call_mtx);

    if (call_service != HU_XCALL_SERV_NONE)
    {
        pthread_mutex_unlock(&call_mtx);
        shellprintf(" error: xcall is in service, please try later\r\n");
        return -3;
    }

    if (argv[0][0] == 'b')
    {
        type = HU_XCALL_TYPE_BCALL;
    }
    else
    {
        type = HU_XCALL_TYPE_ECALL;
    }

    strncpy(call_num[type], argv[1], 31);
    call_num[type][31] = 0;
    pthread_mutex_unlock(&call_mtx);
    return 0;
}

static int hu_xcall_show_callnum(int argc, const char **argv)
{
    shellprintf(" B-call: %s\r\n", call_num[HU_XCALL_TYPE_BCALL]);
    shellprintf(" E-call: %s\r\n", call_num[HU_XCALL_TYPE_ECALL]);
    return 0;
}


static hu_cmd_t hu_cmd_xcall =
{
    .cmd     = HU_CMD_XCALL,
    .uptype  = HU_CMD_TYPE_NEEDACK,
    .timeout = 500,
    .send_proc = hu_xcall_send,
    .done_proc = hu_xcall_done,
};


static hu_cmd_t hu_cmd_mute =
{
    .cmd     = HU_CMD_MUTE,
    .uptype  = HU_CMD_TYPE_NEEDACK,
    .timeout = 500,
    .send_proc = hu_xcall_send,
    .done_proc = hu_xcall_done,
};

static hu_cmd_t hu_cmd_xcall_ctl =
{
    .cmd     = HU_CMD_XCALL_CTL,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_xcall_send_ctl,
    .recv_proc = hu_xcall_recv_ctl,
};

static hu_cmd_t hu_cmd_xcall_query =
{
    .cmd     = HU_CMD_XCALL_QUERY,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_xcall_send_query,
};

int hu_xcall_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            ret |= pthread_mutex_init(&call_mtx, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= hu_register_cmd(&hu_cmd_xcall);
            ret |= hu_register_cmd(&hu_cmd_mute);
            ret |= hu_register_cmd(&hu_cmd_xcall_ctl);
            ret |= hu_register_cmd(&hu_cmd_xcall_query);
            ret |= shell_cmd_register("husetcallnum", hu_xcall_set_callnum, "set Geely HU X-call number");
            ret |= shell_cmd_register("hucallnum", hu_xcall_show_callnum, "show Geely HU X-call number");
            break;
    }

    return ret;
}






