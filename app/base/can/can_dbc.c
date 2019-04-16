#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "com_app_def.h"
#include "init.h"
#include "list.h"
#include "can_api.h"
#include "can_main.h"
#include "can_dbc.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "md5.h"
#include "file.h"
#include "dir.h"
#include "uds_stack_def.h"
#include "scom_msg_def.h"
#include "scom_tl.h"
#include "scom_api.h"

#define DBC_KEYWORD_LEN     8
#define DBC_DUMP_PAGESZ     100

#define DBC_MAX_MESSAGE     256
#define DBC_MAX_SIGNAL      1024
#define DBC_MAX_LINESZ      4096
#define DBC_MAX_MATCHID     64
#define DBC_MAX_CALLBACK    16

#define DBC_FLAG_BCD        (1 << 0)

#define DBC_KEYWORD_MSG     "BO_"
#define DBC_KEYWORD_SIGNAL  "SG_"
#define DBC_KEYWORD_BA      "BA_"

#define DBC_LOCK_RELOAD()   do{/*log_e(LOG_CAN, "lock load");*/pthread_mutex_lock(&dbc_ldmtx);}while(0)
#define DBC_UNLOCK_RELOAD() do{/*log_e(LOG_CAN, "unlock load");*/pthread_mutex_unlock(&dbc_ldmtx);}while(0)
#define DBC_LOCK_UPDATE()   do{/*log_e(LOG_CAN, "lock update");*/pthread_mutex_lock(&dbc_upmtx);}while(0)
#define DBC_UNLOCK_UPDATE() do{/*log_e(LOG_CAN, "unlock update");*/pthread_mutex_unlock(&dbc_upmtx);}while(0)


typedef struct
{
    uint32_t id;
    char     ecu[DBC_ECUNAME_LEN];
    list_t   siglst;
} dbc_msg_t;

typedef struct _dbc_data_t
{
    char fpath[256];
    dbc_msg_t msglst[DBC_MAX_MESSAGE];
    dbc_sig_t siglst[DBC_MAX_SIGNAL];
    uint32_t  msgcnt;
    uint32_t  sigcnt;
    uint8_t   md5[16];
    struct _dbc_data_t *next;
} dbc_data_t;

#define MAX_ID_STD 96
#define MAX_ID_EX  20
typedef struct
{
    uint8_t sum_type[2][2];             /*00: CAN0 STD SUM; 01: CAN1 STD ; 10: CAN0 EX; 11: CAN1 EX*/
    uint16_t CANID_STD[MAX_ID_STD];     /*CAN0 CAN1 STD CANID*/
    uint32_t CANID_EX[MAX_ID_EX];       /*CAN0 CAN1 EX CANID*/
} __attribute__((packed)) canid_2mcu;

static char fmt_getkwd[8];
static char fmt_getmsg[32];
static char fmt_getsig[64];
static char fmt_getdef[32];

static dbc_data_t  dbc_mem[2];
static dbc_data_t *dbc_data;
static dbc_sig_t  *dbc_match_tbl[DBC_MAX_MATCHID];
static can_cb_t dbc_callback_lst[DBC_MAX_CALLBACK];
static pthread_mutex_t dbc_upmtx;
static pthread_mutex_t dbc_ldmtx;
static uint32_t  dbc_usrflags;

/*dbc canid to mcu*/
static canid_2mcu id2mcu;
static canid_2mcu *ptr_id2mcu;

static void dbc_match_tbl_init(void)
{
    memset(dbc_match_tbl, 0, sizeof(dbc_match_tbl));
}

static int dbc_match_tbl_set(uint32_t id, dbc_sig_t *sig)
{
    if (id == 0 || id > DBC_MAX_MATCHID)
    {
        log_e(LOG_CAN, "match index is out of range: %u", id);
        return -1;
    }

    if (dbc_match_tbl[id - 1])
    {
        log_e(LOG_CAN, "match index %u is repeated", id);
        return -1;
    }

    dbc_match_tbl[id - 1] = sig;
    return 0;
}

static int dbc_match_tbl_adj(dbc_data_t *dbc)
{
    int i;

    for (i = 0; i < dbc->sigcnt; i++)
    {
        int j;

        for (j = 0; j < 4 && dbc->siglst[i].mi[j]; j++)
        {
            dbc_sig_t *sig = dbc_match_tbl[dbc->siglst[i].mi[j] - 1];

            if (sig == NULL)
            {
                log_e(LOG_CAN, "match id signal %u is not existed", dbc->siglst[i].mi[j]);
                return -1;
            }

            dbc->siglst[i].mi[j] = sig - dbc->siglst + 1;
        }
    }

    return 0;
}


static int dbc_match_sig(dbc_data_t *dbc, dbc_sig_t *sig)
{
    int i, match;

    for (i = 0, match = 1; i < 4 && sig->mi[i] && match; i++)
    {
        if (sig->mv[i] != dbc->siglst[sig->mi[i] - 1].value)
        {
            match = 0;
        }
    }

    return match;
}

static void dbc_reset_signal(dbc_data_t *dbc)
{
    int i;

    for (i = 0; i < dbc->sigcnt; i++)
    {
        dbc->siglst[i].value = dbc->siglst[i].lastv = dbc->siglst[i].init;
    }
}

static dbc_msg_t *dbc_new_msg(dbc_data_t *dbc)
{
    dbc_msg_t *msg = NULL;

    if (dbc->msgcnt < DBC_MAX_MESSAGE)
    {
        msg = dbc->msglst + dbc->msgcnt++;
        list_init(&msg->siglst);
        msg->id = 0;
    }

    return msg;
}

static dbc_sig_t *dbc_new_sig(dbc_data_t *dbc)
{
    dbc_sig_t *sig = NULL;

    if (dbc->sigcnt < DBC_MAX_SIGNAL)
    {
        sig = dbc->siglst + dbc->sigcnt++;
        memset(sig, 0, sizeof(dbc_sig_t));
    }

    return sig;
}

static dbc_msg_t *dbc_find_msg(dbc_data_t *dbc, uint32_t id)
{
    int i;

    for (i = 0; i < dbc->msgcnt; i++)
    {
        if (dbc->msglst[i].id == id)
        {
            return dbc->msglst + i;
        }
    }

    return NULL;
}

static dbc_sig_t *dbc_find_sig(dbc_data_t *dbc, const char *name)
{
    int i;

    for (i = 0; i < dbc->sigcnt; i++)
    {
        if (strcmp(dbc->siglst[i].name, name) == 0)
        {
            return dbc->siglst + i;
        }
    }

    return NULL;
}


static dbc_msg_t *dbc_parse_msg(dbc_data_t *dbc, const char *line)
{
    char   ecu[DBC_ECUNAME_LEN];
    uint32_t id;
    dbc_msg_t *msg = NULL;

    if (sscanf(line, fmt_getmsg, &id, ecu) != 2)
    {
        log_e(LOG_CAN, "message format error");
        msg = NULL;
    }
    else if (id > 0x7ff && !(id & 0x80000000))
    {
        log_e(LOG_CAN, "illegal message id 0x%08x", id);
    }
    else if ((msg = dbc_find_msg(dbc, id)) == NULL &&
             (msg = dbc_new_msg(dbc)) == NULL)
    {
        log_e(LOG_CAN, "too many messages in DBC file");
    }
    else if (msg->id == 0)
    {
        msg->id = id;
        strcpy(msg->ecu, ecu);
    }

    return msg;
}

static int dbc_sig_overflow(char order, int start, int size)
{
    if (size <= 0 || size > 64 || start < 0 || start >= 64)
    {
        return 1;
    }

    if (order != '0' && order != '1')
    {
        return 1;
    }

    if (order == '1' && (start + size) > 64)
    {
        return 1;
    }

    if (order == '0' && ((start >> 3) * 16 + 7 - start + size) > 64)
    {
        return 1;
    }

    return 0;
}



static char *dbc_get_sig_suffix(char *name)
{
    char *sfx = strrchr(name, '_');

    if (sfx && sfx != name)
    {
        *sfx++ = 0;
    }
    else
    {
        sfx = NULL;
    }

    return sfx;
}

static int dbc_parse_sig_suffix(dbc_data_t *dbc, dbc_sig_t *sig, const char *sfx)
{
    const char *p = sfx;
    int mc = 0;

    if (sfx == NULL)
    {
        log_w(LOG_CAN, "just warnning: signal(%s) has no suffix", sig->name);
        return 0;
    }

    if (strlen(sfx) >= DBC_SUFFIX_LEN)
    {
        log_w(LOG_CAN, "just warnning: suffix is too long to display: %s", sfx);
    }

    strncpy(sig->suffix, sfx, DBC_SUFFIX_LEN - 1);
    sig->suffix[DBC_SUFFIX_LEN - 1] = 0;

    while (*p)
    {
        if (*p == '@')
        {
            if (sscanf(p, "@%u", &sig->port) != 1)
            {
                log_e(LOG_CAN, "suffix @x is incorrect: %s", p);
                break;
            }

            if (!sig->port || sig->port > CAN_MAX_PORT)
            {
                log_e(LOG_CAN, "can port number is out of range(1 - %d): %s", CAN_MAX_PORT, p);
                break;
            }

            sig->port--;
            p += 2;
        }
        else if (*p == 'B')
        {
            sig->flags |= DBC_FLAG_BCD;
            p += 1;
        }
        else if (*p == 'M')
        {
            if (mc >= 4)
            {
                log_e(LOG_CAN, "too many MxIx suffix: %s", p);
                break;
            }

            if (sscanf(p, "M%lfI%u", sig->mv + mc, sig->mi + mc) != 2)
            {
                log_e(LOG_CAN, "suffix MxIx is incorrect: %s", p);
                break;
            }

            if (sig->mi[mc] == 0 || sig->mi[mc] > DBC_MAX_MATCHID)
            {
                log_e(LOG_CAN, "match index is out of range: %s", p);
                break;
            }

            mc +=  1;

            while (*++p != 'I');

            while (isdigit(*++p));
        }
        else if (*p == 'I')
        {
            if (sscanf(p, "I%u", &sig->midx) != 1 ||
                dbc_match_tbl_set(sig->midx, sig) != 0)
            {
                log_e(LOG_CAN, "suffix IDx is incorrect: %s", p);
                break;
            }

            while (isdigit(*++p));
        }
        else
        {
            int i, ret = 0, lastret = 0;

            for (i = 0; i < DBC_MAX_CALLBACK && dbc_callback_lst[i]; i++)
            {
                ret = dbc_callback_lst[i](DBC_EVENT_SURFIX, sig - dbc->siglst + 1, (uint32_t)p);

                if (lastret > 0)
                {
                    if (ret > 0 && lastret != ret)
                    {
                        log_e(LOG_CAN, "suffix with different meaning");
                        break;
                    }
                }
                else
                {
                    lastret = ret;
                }
            }

            if (ret > 0 && lastret != ret)
            {
                log_e(LOG_CAN, "error");
                break;
            }

            p += ret > 0 ? ret : 1;
        }
    }

    return *p;
}



static dbc_sig_t *dbc_parse_sig(dbc_data_t *dbc, dbc_msg_t *msg, const char *line)
{
    char name[DBC_SIGNAME_LEN];
    char unit[DBC_SIGUNIT_LEN];
    char order, sign;
    int  start, size;
    double factor, offset, min, max;
    dbc_sig_t *sig = NULL;

    if (sscanf(line, fmt_getsig, name, &start, &size, &order, &sign, &factor,
               &offset, &min, &max, unit) != 10)
    {
        log_e(LOG_CAN, "signal format error");
    }
    else if (dbc_sig_overflow(order, start, size))
    {
        log_e(LOG_CAN, "signal bits is overflow");
    }
    else
    {
        char *sfx = dbc_get_sig_suffix(name);
        //char  pfx = dbc_get_sig_prefix(name);

        if (dbc_find_sig(dbc, name) != NULL)
        {
            log_e(LOG_CAN, "signal %s is repeated", name);
        }
        else if ((sig = dbc_new_sig(dbc)) == NULL)
        {
            log_e(LOG_CAN, "too many signals in DBC file");
        }
        else if (dbc_parse_sig_suffix(dbc, sig, sfx) != 0)
        {
            log_e(LOG_CAN, "signal suffix error: %s", sfx);
            sig = NULL;
        }
        else
        {
            if (unit[strlen(unit) - 1] != '"')
            {
                strcat(unit, "\"");
            }

            strcpy(sig->name, name);
            strcpy(sig->unit, unit);
            strcpy(sig->ecu, msg->ecu);
            sig->order  = order;
            sig->sign   = sign;
            sig->start  = start;
            sig->size   = size;
            sig->factor = factor;
            sig->offset = offset;
            sig->min    = min;
            sig->max    = max;
            sig->init   = 0;

            list_insert_before(&msg->siglst, &sig->link);
        }
    }

    return sig;
}

static int dbc_parse_def(dbc_data_t *dbc, const char *line, short *baud)
{
    static const uint32_t baudlst[6] =
    {
        50, 100, 125, 250, 500, 1000
    };

    unsigned short tmp, port;
    char name[DBC_SIGNAME_LEN];
    double defval;


    /* for old type baudrate configure */
    if (1 == sscanf(line, "BA_ \"HSBaudrate\" %hu", &tmp) && tmp < 6 && CAN_MAX_PORT > 0)
    {
        log_o(LOG_CAN, "set baud 0=%u\r\n", baudlst[tmp]);
        baud[0] = baudlst[tmp];
    }
    else if (1 == sscanf(line, "BA_ \"MSBaudrate\" %hu", &tmp) && tmp < 6 && CAN_MAX_PORT > 1)
    {
        baud[1] = baudlst[tmp];
    }
    else if (1 == sscanf(line, "BA_ \"LSBaudrate\" %hu", &tmp) && tmp < 6 && CAN_MAX_PORT > 2)
    {
        baud[2] = baudlst[tmp];
    }
    else if (2 == sscanf(line, "BA_ \"Baudrate%hu\" %hu", &port, &tmp) && port > 0
             && port <= CAN_MAX_PORT)
    {
        baud[port - 1] = tmp;
    }
    else if (2 == sscanf(line, fmt_getdef, name, &defval))
    {
        dbc_sig_t *sig = dbc_find_sig(dbc, name);

        if (sig)
        {
            sig->init = defval;
        }
        else
        {
            log_w(LOG_CAN, "just warnning: signal \"%s\" not found with default value declearation", name);
        }
    }
    else
    {
        int i;

        for (i = 0; i < DBC_MAX_CALLBACK && dbc_callback_lst[i]; i++)
        {
            dbc_callback_lst[i](DBC_EVENT_DEFINE, (uint32_t)line, 0);
        }

    }

    return 0;
}

int dbc_can_callback(uint32_t event, uint32_t par1, uint32_t par2)
{
    if (dbc_data == NULL)
    {
        return 0;
    }

    if (event == CAN_EVENT_DATAIN)
    {
        int i;
        CAN_MSG *canmsg;

        for (i = 0, canmsg = (CAN_MSG *)par1; i < par2; i++, canmsg++)
        {
            dbc_msg_t *msg;
            uint64_t rawbit, revbit;
            list_t *node;

            if (canmsg->type != 'C' || (msg = dbc_find_msg(dbc_data, canmsg->MsgID)) == NULL)
            {
                continue;
            }

            memcpy(&rawbit, canmsg->Data, 8);
            revbit = REV64(rawbit);

            for (node = msg->siglst.next; node != &msg->siglst; node = node->next)
            {
                int64_t  rawval;
                double   value;
                dbc_sig_t *sig = list_entry(node, dbc_sig_t, link);

                if (sig->port != RPORT(canmsg->port) || !dbc_match_sig(dbc_data, sig))
                {
                    continue;
                }

                if (sig->order == '1')
                {
                    rawval = (rawbit >> sig->start) & ((1ULL << sig->size) - 1);
                }
                else
                {
                    int start;

                    start  = 57 - (sig->start & ~0x7) + (sig->start & 0x7) - sig->size;
                    rawval = (revbit >> start) & ((1ULL << sig->size) - 1);
                }

                if (sig->sign == '-' && (rawval & (1ULL << (sig->size - 1))))
                {
                    rawval |= ((uint64_t) - 1) << sig->size;
                }

                value = (double)rawval * sig->factor + sig->offset;

                if (sig->max > sig->min && (value < sig->min || value > sig->max))
                {
                    value = sig->init;
                }

                if (value != sig->value)
                {
                    int j;

                    DBC_LOCK_UPDATE();
                    sig->lastv = sig->value;
                    sig->value = value;
                    DBC_UNLOCK_UPDATE();

                    for (j = 0; j < DBC_MAX_CALLBACK && dbc_callback_lst[j]; j++)
                    {
                        dbc_callback_lst[j](DBC_EVENT_UPDATE, sig - dbc_data->siglst + 1, canmsg->uptime);
                    }
                }
            }
        }
    }
    else if (event == CAN_EVENT_TIMEOUT || event == CAN_EVENT_SLEEP)
    {
        int i;

        DBC_LOCK_UPDATE();
        dbc_reset_signal(dbc_data);
        DBC_UNLOCK_UPDATE();

        for (i = 0; i < DBC_MAX_CALLBACK && dbc_callback_lst[i]; i++)
        {
            dbc_callback_lst[i](DBC_EVENT_RESET, par1, 0);
        }
    }

    return 0;
}

static void bubble_sort_int(uint32_t *arr, int len)
{
    int i, j;
    uint32_t tmp;

    if (len == 0)
    {
        return;
    }

    for (i = 0; i < len - 1; i++)
    {
        for (j = 0; j < len - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

static void bubble_sort_short(uint16_t *arr, int len)
{
    int i, j;
    uint16_t tmp;

    if (len == 0)
    {
        return;
    }

    for (i = 0; i < len - 1; i++)
    {
        for (j = 0; j < len - 1 - i; j++)
        {
            if (arr[j] > arr[j + 1])
            {
                tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }
}

void dbc_canid_check(unsigned char no, unsigned char *body)
{
    switch (no)
    {
        case 0:
            if (0 != memcmp((void *)&ptr_id2mcu->sum_type, body, sizeof(ptr_id2mcu->sum_type)))
            {
                log_e(LOG_CAN, "first ptr_id2mcu->sum_type to mcu error");

                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, (no + 0x10),
                                            (void *)&ptr_id2mcu->sum_type, sizeof(ptr_id2mcu->sum_type)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->sum_type to mcu error");

                }
                else
                {
                    log_o(LOG_CAN, "repeat send ptr_id2mcu->sum_type trans to mcu");
                }
            }

            break;

        case 1:
            if (0 != memcmp((void *)&ptr_id2mcu->CANID_STD, body, sizeof(ptr_id2mcu->CANID_STD)))
            {
                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, (no + 0x10),
                                            (void *)&ptr_id2mcu->CANID_STD, sizeof(ptr_id2mcu->CANID_STD)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->CANID_STD trans to mcu error");

                }
                else
                {
                    log_o(LOG_CAN, "repeat send ptr_id2mcu->CANID_STD trans to mcu");
                }
            }

            break;

        case 2:
            if (0 != memcmp((void *)&ptr_id2mcu->CANID_EX, body, sizeof(ptr_id2mcu->CANID_EX)))
            {

                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, (no + 0x10),
                                            (void *)&ptr_id2mcu->CANID_EX, sizeof(ptr_id2mcu->CANID_EX)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->CANID_EX trans to mcu error");
                }
                else
                {
                    log_o(LOG_CAN, "repeat send ptr_id2mcu->CANID_EX trans to mcu");
                }
            }

            break;

        default:
            break;
    }
}

void dbc_canid_to_mcu(void)
{
    int i = 0, j = 0;
    unsigned char flag_sig_port[2] = {0, 0};

    ptr_id2mcu = &id2mcu;
    memset((void *)&id2mcu, 0, sizeof(id2mcu));

    log_o(LOG_CAN, "dbc_data->msgcnt = %d", dbc_data->msgcnt);

    if (dbc_data == NULL)
    {
        log_e(LOG_CAN, "dbc cannot load");
        return;
    }

    if (dbc_data->msgcnt > MAX_ID_STD + MAX_ID_EX)
    {
        log_e(LOG_CAN, "dbc_data->msgcnt %d > %d(set)", dbc_data->msgcnt, (MAX_ID_STD + MAX_ID_EX));
        return;
    }

    for (i = 0; i < dbc_data->msgcnt; i++)
    {
        dbc_msg_t *msg = dbc_data->msglst + i;

        list_t *node;

        memset((void *)&flag_sig_port, 0, sizeof(flag_sig_port));

        for (node = msg->siglst.next; node != &msg->siglst; node = node->next)
        {
            dbc_sig_t *sig = list_entry(node, dbc_sig_t, link);
            flag_sig_port[sig->port] = 1;
        }

        for (j = 0; j < 2; j++)
        {
            if (flag_sig_port[j] == 1)
            {
                if (msg->id <= 0x7ff)
                {
                    if (j == 0)
                    {
                        ptr_id2mcu->CANID_STD[ptr_id2mcu->sum_type[0][0]++] = (uint16_t)msg->id;
                    }
                    else
                    {
                        ptr_id2mcu->sum_type[0][1]++;
                        ptr_id2mcu->CANID_STD[MAX_ID_STD - ptr_id2mcu->sum_type[0][1]] = (uint16_t)msg->id;

                    }
                }
                else
                {
                    if (j == 0)
                    {
                        ptr_id2mcu->CANID_EX[ptr_id2mcu->sum_type[1][0]++] = msg->id;
                    }
                    else
                    {
                        ptr_id2mcu->sum_type[1][1]++;
                        ptr_id2mcu->CANID_EX[MAX_ID_EX - ptr_id2mcu->sum_type[1][1]] = msg->id;

                    }
                }
            }
        }
    }

    if (UDS_CAN_PORT == 0)
    {
        ptr_id2mcu->CANID_STD[ptr_id2mcu->sum_type[0][0]++] = (uint16_t)UDS_CAN_ID_PHY;
        ptr_id2mcu->CANID_STD[ptr_id2mcu->sum_type[0][0]++] = (uint16_t)UDS_CAN_ID_FUN;
    }
    else if (UDS_CAN_PORT == 1)
    {
        ptr_id2mcu->sum_type[0][1]++;
        ptr_id2mcu->CANID_STD[MAX_ID_STD - ptr_id2mcu->sum_type[0][1]] = (uint16_t)UDS_CAN_ID_PHY;
        ptr_id2mcu->sum_type[0][1]++;
        ptr_id2mcu->CANID_STD[MAX_ID_STD - ptr_id2mcu->sum_type[0][1]] = (uint16_t)UDS_CAN_ID_FUN;
    }

    if (ptr_id2mcu->sum_type[0][0] > 0)
    {
        bubble_sort_short(&(ptr_id2mcu->CANID_STD[0]), ptr_id2mcu->sum_type[0][0]);
    }

    if (ptr_id2mcu->sum_type[0][1] > 0)
    {
        bubble_sort_short(&(ptr_id2mcu->CANID_STD[MAX_ID_STD - ptr_id2mcu->sum_type[0][1]]),
                          ptr_id2mcu->sum_type[0][1]);
    }

    if (ptr_id2mcu->sum_type[1][0] > 0)
    {
        bubble_sort_int(& (ptr_id2mcu->CANID_EX[0]), ptr_id2mcu->sum_type[1][0]);
    }


    if (ptr_id2mcu->sum_type[1][1] > 0)
    {
        bubble_sort_int(&(ptr_id2mcu->CANID_EX[MAX_ID_EX - ptr_id2mcu->sum_type[1][1]]),
                        ptr_id2mcu->sum_type[1][1]);
    }

    for (i = 0; i < ptr_id2mcu->sum_type[0][0]; i++)
    {
        log_i(LOG_CAN, "CAN0 ptr_id2mcu->CANID_STD[%d] = %08X", i, ptr_id2mcu->CANID_STD[i]);
    }

    for (i = MAX_ID_STD - ptr_id2mcu->sum_type[0][1]; i < MAX_ID_STD; i++)
    {
        log_i(LOG_CAN, "CAN1 ptr_id2mcu->CANID_STD[%d] = %08X", i, ptr_id2mcu->CANID_STD[i]);
    }

    for (i = 0; i < ptr_id2mcu->sum_type[1][0]; i++)
    {
        log_i(LOG_CAN, "CAN0 ptr_id2mcu->CANID_EX[%d] = %08X", i, ptr_id2mcu->CANID_EX[i]);
    }

    for (i = MAX_ID_EX - ptr_id2mcu->sum_type[1][1]; i < MAX_ID_EX; i++)
    {
        log_i(LOG_CAN, "CAN1 ptr_id2mcu->CANID_EX[%d] = %08X", i, ptr_id2mcu->CANID_EX[i]);
    }

    for (i = 0; i < 3; i++)
    {
        switch (i)
        {
            case 0:
                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, i,
                                            (void *)&ptr_id2mcu->sum_type, sizeof(ptr_id2mcu->sum_type)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->sum_type to mcu error");

                }

                break;

            case 1:
                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, i,
                                            (void *)&ptr_id2mcu->CANID_STD, sizeof(ptr_id2mcu->CANID_STD)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->CANID_STD trans to mcu error");

                }

                break;

            case 2:
                if (0 != scom_tl_send_frame(SCOM_TL_CMD_CAN_FILER_CANID, SCOM_TL_SINGLE_FRAME, i,
                                            (void *)&ptr_id2mcu->CANID_EX, sizeof(ptr_id2mcu->CANID_EX)))
                {
                    log_e(LOG_CAN, "ptr_id2mcu->CANID_EX trans to mcu error");

                }

                break;

            default:
                break;

        }

    }

}

static int dbc_shell_info(int argc, const char **argv)
{
    int i, mode = 0;
    static int offset = 0;

    if (argc == 1 && strcmp(argv[0], "/p") == 0)
    {
        mode = 'p';
    }
    else if (argc == 1 && strcmp(argv[0], "/t") == 0)
    {
        mode = 't';
    }
    else if (argc != 0)
    {
        shellprintf(" usage: dbcsigls [option]\r\n");
        shellprintf("    option: /p  - display in page\r\n");
        shellprintf("            /t  - display in tree\r\n");
        return 0;
    }

    shellprintf(" error: 111\r\n");
    DBC_LOCK_RELOAD();
    shellprintf(" error: 222\r\n");

    if (dbc_data == NULL)
    {
        shellprintf(" error: dbc file is not loaded\r\n");
        DBC_UNLOCK_RELOAD();
        return 0;
    }

    shellprintf(" File Name  : %s\r\n", dbc_data->fpath);
    shellprintf(" MD5 Code   : ");

    for (i = 0; i < 16; i++)
    {
        shellprintf("%02X", dbc_data->md5[i]);
    }

    shellprintf("\r\n");

    if (mode == 't')
    {
        for (i = 0; i < dbc_data->msgcnt; i++)
        {
            dbc_msg_t *msg = dbc_data->msglst + i;
            list_t *node;

            shellprintf(" CAN MSG ID : %lu(0x%-8X) ECU: %s\r\n", msg->id, msg->id, msg->ecu);
            shellprintf("  | name                                     port st sz o s        factor        offset           min           max unit     suffix\r\n");
            shellprintf("  | ---------------------------------------- ---- -- -- - - ------------- ------------- ------------- ------------- -------- ----------------\r\n");

            for (node = msg->siglst.next; node != &msg->siglst; node = node->next)
            {
                dbc_sig_t *sig = list_entry(node, dbc_sig_t, link);

                shellprintf("  |-%-40s CAN%1u %2u %2u %c %c %13.6lf %13.6lf %13.6lf %13.6lf %-8s %s\r\n",
                            sig->name, sig->port + 1, sig->start, sig->size, sig->order,
                            sig->sign, sig->factor, sig->offset, sig->min, sig->max,
                            sig->unit, sig->suffix);
            }

            shellprintf("\r\n");
        }
    }
    else
    {
        int start, end;

        if (mode == 'p')
        {
            start = offset;
            end   = MIN(start + DBC_DUMP_PAGESZ, dbc_data->sigcnt);
        }
        else
        {
            start = 0;
            end   = dbc_data->sigcnt;
        }

        shellprintf(" Signal List: %d to %d, total %d\r\n", start + 1, end, dbc_data->sigcnt);
        shellprintf("  indx name                                     port st sz o s        factor        offset           min           max unit     suffix\r\n");
        shellprintf("  ---- ---------------------------------------- ---- -- -- - - ------------- ------------- ------------- ------------- -------- ----------------\r\n");

        for (i = start; i < end; i++)
        {
            dbc_sig_t *sig = dbc_data->siglst + i;

            shellprintf("  %-4d %-40s CAN%1u %2u %2u %c %c %13.6lf %13.6lf %13.6lf %13.6lf %-8s %s\r\n",
                        i + 1,
                        sig->name, sig->port + 1, sig->start, sig->size, sig->order,
                        sig->sign, sig->factor, sig->offset, sig->min, sig->max,
                        sig->unit, sig->suffix);
        }

        if (mode == 'p')
        {
            offset = end % dbc_data->sigcnt;
        }
    }

	#if 0
    dbc_canid_to_mcu();
	#endif

    DBC_UNLOCK_RELOAD();
    return 0;
}

static int dbc_shell_sigls(int argc, const char **argv)
{
    int i, mode = 0;
    static int offset = 0;

    if (argc == 1 && strcmp(argv[0], "/p") == 0)
    {
        mode = 'p';
    }
    else if (argc == 1 && strcmp(argv[0], "/t") == 0)
    {
        mode = 't';
    }
    else if (argc != 0)
    {
        shellprintf(" usage: dbcsigls [option]\r\n");
        shellprintf("    option: /p  - list signal in page\r\n");
        shellprintf("            /t  - list signal in tree\r\n");
        return 0;
    }

    shellprintf(" error: 333\r\n");
    DBC_LOCK_RELOAD();
    shellprintf(" error: 444\r\n");

    if (dbc_data == NULL)
    {
        shellprintf(" error: dbc file is not loaded\r\n");
        DBC_UNLOCK_RELOAD();
        return 0;
    }

    DBC_LOCK_UPDATE();

    if (mode == 't')
    {
        for (i = 0; i < dbc_data->msgcnt; i++)
        {
            dbc_msg_t *msg = dbc_data->msglst + i;
            list_t *node;

            shellprintf(" CAN MSGID : %lu(0x%-8X) ECU: %s\r\n", msg->id, msg->id, msg->ecu);
            shellprintf("  | name                                     port             value        last value\n");
            shellprintf("  | ---------------------------------------- ---- ----------------- -----------------\n");

            for (node = msg->siglst.next; node != &msg->siglst; node = node->next)
            {
                dbc_sig_t *sig = list_entry(node, dbc_sig_t, link);

                shellprintf("  |-%-40s CAN%1u %17.6lf %17.6lf\n",
                            sig->name, sig->port + 1, sig->value, sig->lastv);
            }

            shellprintf("\r\n");
        }
    }
    else
    {
        int start, end;

        if (mode == 'p')
        {
            start = offset;
            end   = MIN(start + DBC_DUMP_PAGESZ, dbc_data->sigcnt);
        }
        else
        {
            start = 0;
            end   = dbc_data->sigcnt;
        }

        shellprintf(" Signal List: %d to %d, total %d\r\n", start + 1, end, dbc_data->sigcnt);
        shellprintf("  indx name                                     port             value        last value\r\n");
        shellprintf("  ---- ---------------------------------------- ---- ----------------- -----------------\r\n");

        for (i = start; i < end; i++)
        {
            dbc_sig_t *sig = dbc_data->siglst + i;

            shellprintf("  %-4d %-40s CAN%1u %17.6lf %17.6lf\r\n",
                        i + 1, sig->name, sig->port + 1, sig->value, sig->lastv);
        }

        if (mode == 'p')
        {
            offset = end % dbc_data->sigcnt;
        }
    }

    DBC_UNLOCK_UPDATE();
    DBC_UNLOCK_RELOAD();
    return 0;
}

static int dbc_shell_drdbcv(int argc, const char **argv)
{
    unsigned int i, num, temp;
    unsigned int Dbcvalueindex;

    if (argc != 1)
    {
        shellprintf(" error\r\n");
        return -1;
    }

    if (NULL == dbc_data)
    {
        shellprintf(" error: DBC file no load!\r\n");
        return -1;
    }

    if (dbc_data->sigcnt <= 1)
    {
        shellprintf(" error: No DBC file imported!\r\n");
        return -1;
    }

    sscanf((char const *)argv[0], "%u", &Dbcvalueindex);

    num = dbc_data->sigcnt / 15;
    temp = dbc_data->sigcnt % 15;


    if (Dbcvalueindex == 0)
    {
        shellprintf(" error\r\n");
        return -1;
    }
    else
    {
        if (Dbcvalueindex > ((temp) ? (num + 1) : num))
        {
            shellprintf(" error\r\n");
            return -1;
        }
        else
        {
            Dbcvalueindex = Dbcvalueindex - 1;
        }
    }

    shellprintf("DBCValue timers : %d \r\n", ((temp) ? (num + 1) : num));

    for (i = 0; ((i < 15) && ((Dbcvalueindex * 15 + i) < dbc_data->sigcnt)); i++)
    {
        dbc_sig_t *sig = dbc_data->siglst + i + Dbcvalueindex * 15;
        shellprintf(" %3d: %32s, %64s = %lf(%s)\r\n",
                    i + 1, sig->ecu, sig->name, sig->value, sig->unit);
    }

    return 0;
}

int dbc_init(INIT_PHASE phase)
{
    char fpath[256];
    int  ret = 0;
    uint32_t len;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(dbc_callback_lst, 0, sizeof(dbc_callback_lst));
            dbc_mem[0].next = dbc_mem + 1;
            dbc_mem[1].next = dbc_mem;
            dbc_data = NULL;
            dbc_usrflags = 0;

            sprintf(fmt_getkwd, "%%%us", DBC_KEYWORD_LEN - 1);
            sprintf(fmt_getmsg, "%%*s %%lu %%*[^:]: %%*u %%%us", DBC_ECUNAME_LEN - 1);
            sprintf(fmt_getsig, "%%*s %%%us : %%d|%%d@%%c%%c (%%lf,%%lf) [%%lf|%%lf] %%%us",
                    DBC_SIGNAME_LEN - 1, DBC_SIGUNIT_LEN - 2);
            sprintf(fmt_getdef, "BA_ \"SigDefault\" %%%us %%lf", DBC_SIGNAME_LEN - 1);
            ret |= pthread_mutex_init(&dbc_upmtx, NULL);
            ret |= pthread_mutex_init(&dbc_ldmtx, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            len = sizeof(fpath);

            if (0 == cfg_get_para(CFG_ITEM_DBC_PATH, (void *)fpath, &len))
            {
                fpath[len] = 0;
                ret |= can_set_dbc(fpath);
            }

            ret |= shell_cmd_register_ex("dbcinfo", "dumpdbc", dbc_shell_info, "show dbc information");
            ret |= shell_cmd_register_ex("dbcsigls", "dumpdbcv", dbc_shell_sigls, "show dbc signal list");
            ret |= shell_cmd_register_ex("dbcvpage", "drdbcv", dbc_shell_drdbcv, "get dbc content by page");
            break;

        default:
            break;
    }

    return ret;
}


static FILE *dbc_open_file(const char *fpath, int *backup)
{
    FILE *fdbc = fopen(fpath, "r");

    if (fdbc == NULL && strncmp(COM_APP_CUR_DBC_DIR, fpath, sizeof(COM_APP_CUR_DBC_DIR) - 1) != 0)
    {
        char newpath[256];
        const char *fname;

        strcpy(newpath, COM_APP_CUR_DBC_DIR);

        if ((fname = strrchr(fpath, '/')) == NULL)
        {
            fname = fpath;
            strcat(newpath, "/");
        }

        strcat(newpath, fname);

        if ( !file_exists(fpath) )
        {
        	log_e(LOG_CAN, "can't find dbc file %s", fpath);
        }
		
		if ( !file_exists(newpath) )
		{
			log_e(LOG_CAN, "can't find bakeup dbc file %s,dbc is lost", newpath);
			return NULL;
		}
		
        log_e(LOG_CAN, "use backup file %s", newpath);

        if ((fdbc = fopen(newpath, "r")) != NULL)
        {
            dir_make_path(fpath, S_IRUSR | S_IWUSR | S_IXUSR, true);
            file_copy(newpath, fpath);
        }

        *backup = 1;
    }

    return fdbc;
}

static int dbc_backup_filepath(const char *fpath)
{
    char newpath[256];
    unsigned int len = sizeof(newpath);

    if (cfg_get_para(CFG_ITEM_DBC_PATH, newpath, &len) == 0 && strcmp(fpath, newpath) == 0)
    {
        return 0;
    }

    strncpy(newpath, fpath, 255);
    newpath[255] = 0;

    return cfg_set_para(CFG_ITEM_DBC_PATH, newpath, sizeof(newpath));
}

static void dbc_backup_file(const char *fpath)
{
    if (strncmp(COM_APP_CUR_DBC_DIR, fpath, sizeof(COM_APP_CUR_DBC_DIR) - 1) != 0)
    {
        char newpath[256];
        const char *fname;

        strcpy(newpath, COM_APP_CUR_DBC_DIR);

        if ((fname = strrchr(fpath, '/')) == NULL)
        {
            fname = fpath;
            strcat(newpath, "/");
        }

        strcat(newpath, fname);

        if (!file_exists(newpath))
        {
            dir_make_path(COM_APP_CUR_DBC_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);
            file_copy(fpath, newpath);
            log_o(LOG_CAN, "backup dbc file %s to %s(create)", fpath, newpath);
        }
        else if (file_comp(newpath, fpath) != 0)
        {
            file_copy(fpath, newpath);
            log_o(LOG_CAN, "backup dbc file %s to %s(overlap)", fpath, newpath);
        }
    }
}

#define dbc_load_exit(n)    do {ret = (n); goto load_exit;} while(0)

int dbc_load_file(const char *fpath, short *baud)
{
    FILE       *fdbc;
    char        linebuf[DBC_MAX_LINESZ];
    int         linenum = 0, ret = 0, i, backup = 0;
    dbc_msg_t  *msg = NULL;
    dbc_data_t *dbc = dbc_data == NULL ? dbc_mem : dbc_data->next;
    MD5_CTX     md5;

    if ( 0 == strlen(fpath) )
	{
		log_e(LOG_CAN, "dbc is not set!");
        return -1;	
	}
	
    if (NULL == (fdbc = dbc_open_file(fpath, &backup)))
    {
        log_e(LOG_CAN, "open dbc file fail");
        return -1;
    }

    for (i = 0; i < DBC_MAX_CALLBACK && dbc_callback_lst[i]; i++)
    {
        dbc_callback_lst[i](DBC_EVENT_RELOAD, (uint32_t)fpath, 0);
    }

    dbc->msgcnt = dbc->sigcnt = 0;
    dbc_match_tbl_init();
    MD5Init(&md5);

    while (fgets(linebuf, DBC_MAX_LINESZ - 1, fdbc))
    {
        char keywd[DBC_KEYWORD_LEN];

        MD5Update(&md5, (uint8_t *)linebuf, strlen(linebuf));

        if (++linenum == 1 && !strstr(linebuf, "VERSION"))
        {
            log_e(LOG_CAN, "file %s format is incorrect", fpath);
            dbc_load_exit(-1);
        }
        else if (sscanf(linebuf, fmt_getkwd, keywd) != 1)
        {
            continue;
        }
        else if (strcmp(keywd, DBC_KEYWORD_MSG) == 0)
        {
            if ((msg = dbc_parse_msg(dbc, linebuf)) == NULL)
            {
                log_e(LOG_CAN, "parse message failed @ %d", linenum);
                dbc_load_exit(-1);
            }
        }
        else if (strcmp(keywd, DBC_KEYWORD_SIGNAL) == 0)
        {
            if (msg == NULL || dbc_parse_sig(dbc, msg, linebuf) == NULL)
            {
                log_e(LOG_CAN, "parse signal failed @ %d", linenum);
                dbc_load_exit(-1);
            }
        }
        else if (strcmp(keywd, DBC_KEYWORD_BA) == 0)
        {
            dbc_parse_def(dbc, linebuf, baud);
        }
    }

    if (!feof(fdbc))
    {
        log_e(LOG_CAN, "file %s accessing failed", fpath);
        dbc_load_exit(-1);
    }

    if (0 != dbc_match_tbl_adj(dbc))
    {
        log_e(LOG_CAN, "adjust match table error");
        dbc_load_exit(-1);
    }

    if (0 != dbc_backup_filepath(fpath))
    {
        log_e(LOG_CAN, "save dbc file path %s failed", fpath);
        dbc_load_exit(-1);
    }

    if (!backup)
    {
        dbc_backup_file(fpath);
    }

    dbc_reset_signal(dbc);
    MD5Final(&md5);
    memcpy(dbc->md5, md5.digest, 16);
    strncpy(dbc->fpath, fpath, sizeof(dbc->fpath) - 1);
    dbc->fpath[sizeof(dbc->fpath) - 1] = 0;

    DBC_LOCK_RELOAD();
    dbc_data = dbc;

load_exit:

    for (i = 0; i < DBC_MAX_CALLBACK && dbc_callback_lst[i]; i++)
    {
        dbc_callback_lst[i](DBC_EVENT_FINISHED, (uint32_t)ret, 0);
    }

    if (ret)
    {
        log_e(LOG_CAN, "dbc file load failed");
    }
    else
    {
        log_i(LOG_CAN, "dbc file load succeed");

		#if 0
        dbc_canid_to_mcu();
		#endif

        DBC_UNLOCK_RELOAD();
    }

    fclose(fdbc);
    return ret;
}

int dbc_register_callback(can_cb_t cb)
{
    int i = 0;

    assert(cb != NULL);

    while (i < DBC_MAX_CALLBACK && dbc_callback_lst[i] != NULL)
    {
        i++;
    }

    if (i >= DBC_MAX_CALLBACK)
    {
        log_e(LOG_CAN, "no space for new callback!");
        return -1;
    }

    dbc_callback_lst[i] = cb;
    return 0;
}

int dbc_request_flag(void)
{
    uint32_t ret = (uint32_t)dbc_usrflags + 1;
    dbc_usrflags |=  ret;
    return ret;
}


void dbc_lock(void)
{
    DBC_LOCK_RELOAD();
}

void dbc_unlock(void)
{
    DBC_UNLOCK_RELOAD();
}

int dbc_get_md5(uint8_t *md5)
{
    assert(md5 != NULL);

    if (dbc_data == NULL)
    {
        return -1;
    }

    memcpy(md5, dbc_data->md5, 16);
    return 0;
}

char *dbc_get_fpath(char *fpath, int size)
{
    char *ret = NULL;

    assert(fpath != NULL);

    if (size > 0 && dbc_data != NULL)
    {
        strncpy(fpath, dbc_data->fpath, size - 1);
        fpath[size - 1] = 0;
        ret = fpath;
    }

    return ret;
}

int dbc_get_signal_cnt(void)
{
    if (dbc_data == NULL)
    {
        return -1;
    }

    return (int)dbc_data->sigcnt;
}

int dbc_get_signal_id(const char *name)
{
    int i;

    assert(name != NULL);

    if (dbc_data == NULL)
    {
        return -1;
    }

    for (i = 0; i < dbc_data->sigcnt && strcmp(dbc_data->siglst[i].name, name); i++)
    {
    }

    return i < dbc_data->sigcnt ? i + 1 : -1;
}

char *dbc_get_signal_name(int id, char *name, int size)
{
    char *ret = NULL;

    assert(id > 0);
    assert(name != NULL);

    if (dbc_data != NULL && size > 0 && id <= dbc_data->sigcnt)
    {
        strncpy(name, dbc_data->siglst[id - 1].name, size - 1);
        name[size - 1] = 0;
        ret = name;
    }

    return ret;
}

char *dbc_get_signal_ecu(int id, char *ecu, int size)
{
    char *ret = NULL;

    assert(id > 0);
    assert(ecu != NULL);

    if (dbc_data != NULL && size > 0 && id <= dbc_data->sigcnt)
    {
        strncpy(ecu, dbc_data->siglst[id - 1].ecu, size - 1);
        ecu[size - 1] = 0;
        ret = ecu;
    }

    return ret;
}

char *dbc_get_signal_unit(int id, char *unit, int size)
{
    char *ret = NULL;

    assert(id > 0);
    assert(unit != NULL);

    if (dbc_data != NULL && size > 0 && id <= dbc_data->sigcnt)
    {
        strncpy(unit, dbc_data->siglst[id - 1].name, size - 1);
        unit[size - 1] = 0;
        ret = unit;
    }

    return ret;
}

double dbc_get_signal_value(int id)
{
    double ret = 0;

    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        DBC_LOCK_UPDATE();
        ret = dbc_data->siglst[id - 1].value;
        DBC_UNLOCK_UPDATE();
    }

    return ret;
}

double dbc_get_signal_lastval(int id)
{
    double ret = 0;

    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        DBC_LOCK_UPDATE();
        ret = dbc_data->siglst[id - 1].lastv;
        DBC_UNLOCK_UPDATE();
    }

    return ret;
}

int dbc_get_signal_port(int id)
{
    int ret = -1;

    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        ret = dbc_data->siglst[id - 1].port;
    }

    return ret;
}



const dbc_sig_t *dbc_get_signal_from_id(int id)
{
    dbc_sig_t *ret = NULL;

    if (id > 0 && dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        ret = dbc_data->siglst + id - 1;
    }

    return ret;
}

const dbc_sig_t *dbc_get_signal_from_name(const char *name)
{
    int id;
    dbc_sig_t *ret = NULL;

    assert(name != NULL);

    if ((id = dbc_get_signal_id(name)) > 0)
    {
        ret = dbc_data->siglst + id - 1;
    }

    return ret;
}

void dbc_set_signal_flag(int id, int flags)
{
    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        dbc_data->siglst[id - 1].usrflags |= flags;
    }
}

void dbc_clr_signal_flag(int id, int flags)
{
    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        dbc_data->siglst[id - 1].usrflags &= ~flags;
    }
}

int dbc_test_signal_flag(int id, int flags, int and)
{
    assert(id > 0);

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        return and ? (dbc_data->siglst[id - 1].usrflags & flags) == flags :
               (dbc_data->siglst[id - 1].usrflags & flags) != 0;
    }

    return 0;
}

int dbc_is_load(void)
{
    return (dbc_data != NULL);
}

int dbc_copy_signal_from_id(uint32_t id, dbc_sig_t *sig)
{
    int ret = -1;

    assert(id > 0);
    assert(sig != NULL);

    DBC_LOCK_RELOAD();

    if (dbc_data != NULL && id <= dbc_data->sigcnt)
    {
        DBC_LOCK_UPDATE();
        memcpy(sig, dbc_data->siglst + id - 1, sizeof(dbc_sig_t));
        DBC_UNLOCK_UPDATE();
        ret = 0;
    }

    DBC_UNLOCK_RELOAD();
    return ret;
}

int dbc_copy_signal_from_name(const char *name, dbc_sig_t *sig)
{
    int id, ret = -1;

    assert(name != NULL);
    assert(sig != NULL);

    DBC_LOCK_RELOAD();

    if (dbc_data != NULL && (id = dbc_get_signal_id(name)) > 0)
    {
        DBC_LOCK_UPDATE();
        memcpy(sig, dbc_data->siglst + id - 1, sizeof(dbc_sig_t));
        DBC_UNLOCK_UPDATE();
        ret = 0;
    }

    DBC_UNLOCK_RELOAD();
    return ret;
}

