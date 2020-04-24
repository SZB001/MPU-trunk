#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "gb32960.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "gb32960_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "../support/protocol.h"
#include "pm_api.h"
#include "at.h"
#include "dev_time.h"
#include "dev_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"

#define PROT_LOGIN      0x01
#define PROT_REPORT     0x02
#define PROT_REISSUE    0x03
#define PROT_LOGOUT     0x04
#define PROT_HBEAT      0x07
#define PROT_CALTIME    0x08
#define PROT_QUREY      0x80
#define PROT_SETUP      0x81
#define PROT_CONTRL     0x82

#define GB_SERVR_TIMEOUT    (1000 * gb_timeout)
#define GB_LOGIN_INTV       (1000 * gb_regintv)
#define GB_SOCKET_RSVD      1024


typedef enum
{
    GB_CFG_URL = 1,
    GB_CFG_PORT,
    GB_CFG_ADDR,
    GB_CFG_VIN,
    GB_CFG_DATINTV,
    GB_CFG_REGINTV,
    GB_CFG_TIMEOUT,
} GB_CFG_TYPE;


typedef struct
{
    /* protocol status */
    int online;
    int wait;
    int socket;
    int retry;
    uint64_t waittime;
    uint8_t rcvbuf[1024];
    int rcvlen;
    int rcvstep;
    int datalen;
    /* static status */
    int login;
    int suspend;
    int caltime;
    int can;
    int network;
    int errtrig;

    int caltimewait;
    uint64_t caltimewaittime;
    uint8_t calflag;
} gb_stat_t;

typedef struct
{
    int cfgid;
    union
    {
        char vin[18];
        svr_addr_t addr;
        uint16_t datintv;
        uint16_t regintv;
        uint16_t timeout;
    };
} gb_cfg_t;

typedef union
{
    /* for GB_MSG_NETWORK */
    int network;
    /* for GB_MSG_SOCKET */
    int socket;
    /* for GB_MSG_CONFIG */
    gb_cfg_t cfg;
} gb_msg_t;

static char       gb_vin[18];
static svr_addr_t gb_addr;
static uint16_t   gb_timeout;
static uint16_t   gb_regintv;
static uint16_t   gb_regdate;
static uint16_t   gb_regseq;
static char       gb_iccid[21];
static char       gb_battcode[64];
static int        gb_allow_sleep;
static uint8_t gbnosend = 0;
static char powerOffFlag = 0xff;

//static gb_stat_t *socket_st = NULL;
static gb_stat_t state;
static int gb32960_MsgSend(uint8_t* Msg,int len,void (*sync)(void));
static int gb_shell_nosend(int argc, const char **argv);
static void gb_reset(gb_stat_t *state)
{
    //sock_close(state->socket);
	//sockproxy_socketclose((int)(PP_SP_COLSE_GB));
    state->wait     = 0;
    state->online   = 0;
    state->retry    = 0;
    state->waittime = 0;
    state->rcvstep  = -1;
    state->rcvlen   = 0;
    state->datalen  = 0;
}

static int gb_checksum(uint8_t *data, int len)
{
    uint8_t cs = 0;

    while (len--)
    {
        cs ^= *data++;
    }

    return cs;
}

static int gb_pack_head(int type, int dir, uint8_t *buf)
{
    int len = 0;

    buf[len++] = '#';
    buf[len++] = '#';
    buf[len++] = type;
    buf[len++] = dir;
    /* vin number */
    memcpy(buf + len, gb_vin, 17);
    len += 17;
    /* encryption mode : */
    /* 0x01 : NONE */
    /* 0x02 : RSA */
    /* 0x03 : AES128 */
    buf[len++] = 0x01;

    return len;
}

static int gb_pack_report(uint8_t *buf, gb_pack_t *rpt)
{
    int len;

    len = gb_pack_head(rpt->type, 0xfe, buf);
    /* data length */
    buf[len++] = rpt->len >> 8;
    buf[len++] = rpt->len;
    /* report data */
    memcpy(buf + len, rpt->data, rpt->len);
    len += rpt->len;
    /* check sum */
    buf[len] = gb_checksum(buf + 2, len - 2);

    return len + 1;
}

static int gb_pack_login(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp, plen, pos;


    tm_get_abstime(&time);

    if (gb_regdate != time.mon * 100 + time.mday)
    {
        uint32_t reginf;

        gb_regdate  = time.mon * 100 + time.mday;
        gb_regseq   = 1;
        reginf      = (gb_regdate << 16) + gb_regseq;
        cfg_set_user_para(CFG_ITEM_GB32960_REGSEQ, &reginf, sizeof(reginf));
    }

    /* header */
    pos = gb_pack_head(PROT_LOGIN, 0xfe, buf);
    /* jump length */
    len = pos + 2;
    /* time */
    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
    buf[len++] = gb_regseq >> 8;
    buf[len++] = gb_regseq;

    /* iccid */
    at_get_iccid(gb_iccid);
    memcpy(buf + len, gb_iccid, 20);
    len += 20;
    /* support only one battery  */
    buf[len++] = 1;
    /* battery code */
    tmp = strlen(gb_battcode);
    buf[len++] = tmp;
    memcpy(buf + len, gb_battcode, tmp);
    len += tmp;
    /* adjust length */
    plen = len - pos - 2;
    buf[pos++] = plen >> 8;
    buf[pos++] = plen;
    /* check sum */
    buf[len] = gb_checksum(buf + 2, len - 2);

    return len + 1;
}

static int gb_pack_logout(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp;

    tmp = gb_regseq == 1 ? 65531 : gb_regseq - 1;
    tm_get_abstime(&time);

    len = gb_pack_head(PROT_LOGOUT, 0xfe, buf);
    /* length */
    buf[len++] = 0;
    buf[len++] = 8;
    /* time */
    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;
    buf[len]   = gb_checksum(buf + 2, len - 2);

    return len + 1;
}

static int gb_pack_tspcailtime(uint8_t *buf)
{
    int len = 0;

    len = gb_pack_head(PROT_CALTIME, 0xfe, buf);
    /* length */
    buf[len++] = 0;
    buf[len++] = 0;

    buf[len]   = gb_checksum(buf + 2, len - 2);

    return len + 1;
}

static int gb_handle_query(gb_stat_t *state, uint8_t *data, int len)
{
    return 0;
}
static int gb_handle_setup(gb_stat_t *state, uint8_t *data, int len)
{
    #if 0
    int i, tmp, cnt = data[6], urlset = 0, parSave = 0;
    static unsigned int urlLen = 0, tmpurlLen = 0;

    ProtAck.ackCmd = 0x81;
    ProtAck.ackLen = 6;
    memcpy(ProtAck.ackDat, data, 6);

    if (cnt < 0xfe)
    {
        uint8_t *pdata = data + 7;
        int i, tmp;

        for (i = 0; i < cnt && pdata != NULL; i++)
        {
            switch (*pdata++)
            {
                case 0x01: /* set local storage period */
                    tmp = GET_WORD(pdata);

                    if (tmp >= 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        pdata += 2;
                    }

                    break;

                case 0x02: /* set data period */
                    tmp = GET_WORD(pdata);

                    if (tmp == 0 || tmp >= 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        gb_set_interval(tmp);
                        pdata += 2;
                        log_i(LOG_GB32960, "set report period: %d s", tmp);
                    }

                    break;

                case 0x03: /* set alarm period */
                    tmp = GET_WORD(pdata);

                    if (tmp == 1000)
                    {
                        log_i(LOG_GB32960, "set alarm period: 1000 ms");
                    }
                    else
                    {
                        log_e(LOG_GB32960, "set alarm period: not support other values expect 1000 ms");
                    }

                    pdata += 3;
                    break;

                case 0x04: /* set URL length */
                    urlLen = *pdata++;
                    log_i(LOG_GB32960, "set URL length: %d", urlLen);
                    break;

                case 0x05: /* set comm URL */
                    if (urlLen == 0)
                    {
                        pdata = NULL;
                        log_e(LOG_GB32960, "URL length is not setup");
                    }
                    else
                    {
                        char *url = pdata;

                        pdata += urlLen;
                        urlLen = 0;
                        log_i(LOG_GB32960, "set URL: %s", url);
                    }

                    break;

                case 0x06: /* set comm port */
                    tmp = GET_WORD(pdata);

                    if (tmp > 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        pdata += 2;
                        log_i(LOG_GB32960, "set port: %d", tmp);
                    }

                    break;

                case 0x09: /* set heart beat period */
                    if (*pdata >= 0xfe || *pdata < 10)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        pdata++;
                        log_i(LOG_GB32960, "set heartbeat period: %d s!", *pdata);
                    }

                    break;

                case 0x0a: /* set terminal timeout */
                    tmp = GET_WORD(pdata);

                    if (tmp >= 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        pdata += 3;
                    }

                    break;

                case 0x0b: /* set server timeout */
                    tmp = ((unsigned int)pdata[1] << 8) + pdata[2];

                    if (tmp >= 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        Parameter.serverTimeout = tmp;
                        parSave++;
                        pdata += 3;
                    }

                    break;

                case 0x0c: /* set relogin time */
                    tmp = ((unsigned int)pdata[1] << 8) + pdata[2];

                    if (pdata[1] < 1 || pdata[1] > 240)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        Parameter.reloginTime = pdata[1];
                        parSave++;
                        pdata += 2;
                    }

                    break;

                case 0x0d:
                    tmpurlLen = pdata[1];
                    pdata += 2;
                    log_i(LOG_GB32960, "set temp URL length: %d!", urlLen);
                    break;

                case 0x0e: /* set comm URL */
                    if (tmpurlLen == 0)
                    {
                        pdata = NULL;
                        log_e(LOG_GB32960, "temp URL length is zero!");
                    }
                    else
                    {
                        memcpy(Parameter.tempURL, pdata + 1, tmpurlLen);
                        Parameter.tempURL[tmpurlLen] = 0;
                        parSave++;
                        pdata += tmpurlLen + 1;
                        tmpurlLen = 0;
                        log_i(LOG_GB32960, "set temp URL : %s!", RTData.remoteCommURL);
                    }

                    break;

                case 0x0f: /* set comm port */
                    tmp = ((unsigned int)pdata[1] << 8) + pdata[2];

                    if (tmp > 0xfffe)
                    {
                        pdata = NULL;
                    }
                    else
                    {
                        Parameter.tempPort = tmp;
                        parSave++;
                        pdata += 3;
                        log_i(LOG_GB32960, "set temp Port : %d!", RTData.remoteCommPort);
                    }

                    break;

                case 0x10:
                    pdata += 2;
                    break;

                default:
                    log_e(LOG_GB32960, "Unsupport setup ID(%02X)!", pdata[0]);
                    pdata = NULL;
                    break;
            }
        }

        if (pdata != NULL && pdata == data + len)
        {
            ProtAck.ackSta = 0x01;
        }
        else
        {
            ProtAck.ackSta = 0x02;
        }
    }
    else
    {
        ProtAck.ackSta = 0x02;
    }

    if (RTData.URLType == URL_REMOTE)
    {
        if (urlset != 0x03 || ProtAck.ackSta != 0x01)
        {
            RTData.URLType = URL_LOCAL;
        }
        else
        {
            RTData.serverUPtime = driverGetUptime() + 10 * 60 * 100 ;
            RTData.cushionTime  = driverGetUptime() + 4 * 100;
        }
    }

    if (ProtAck.ackSta == 0x01 && parSave)
    {
        driverSaveParameter((unsigned char *)&Parameter, sizeof(Parameter));
    }

    #endif
    return 0;
}

static int gb_handle_control(gb_stat_t *state, uint8_t *data, int len)
{
    #if 0
    uint8_t cmd = data[6], *arg = data + 7;
    int tmp, ack = 1;

    ProtAck.ackCmd = 0x82;
    ProtAck.ackLen = 6;
    ProtAck.ackSta = 0x01;
    memcpy(ProtAck.ackDat, data, 6);

    switch (cmd)
    {
        case 0x01:
            log_i(LOG_GB32960, "remote command: upgrade, it's not supported now");
            ack = 2;
            break;

        case 0x02:
            state->suspend = 1;
            log_i(LOG_GB32960, "remote command: shutdown");
            break;

        case 0x03:
            ack = 2;
            log_i(LOG_GB32960, "remote command: reset");
            break;

        case 0x04:
            ack = 2;
            log_i(LOG_GB32960, "remote command: set to default");
            break;

        case 0x05:
            state->suspend = 1;
            log_i(LOG_GB32960, "remote command: close link");
            break;

        case 0x06:
            log_i(LOG_GB32960, "remote command: alarm");
            break;

        case 0x07:
            break;

        default:
            ack = 2;
            leg_e(LOG_GB32960, "unsupported remote command: %02X", cmd);
            break;
    }

    return gb_send_ack(state, 0x82, ack, data, len);
    #endif
    return 0;
}


static int gb_do_checksock(gb_stat_t *state)
{
#if !GB32960_SOCKPROXY
    if (!state->network || !gb_addr.port || !gb_addr.url[0] || gb_allow_sleep)
    {
        return -1;
    }

    if (sock_status(state->socket) == SOCK_STAT_CLOSED)
    {
        static uint64_t time = 0;

        if (!state->suspend && !gb_data_noreport() &&
            (time == 0 || tm_get_time() - time > GB_SERVR_TIMEOUT))
        {
            log_i(LOG_GB32960, "start to connect with server");

            if (sock_open(NM_PUBLIC_NET,state->socket, gb_addr.url, gb_addr.port) != 0)
            {
                log_i(LOG_GB32960, "open socket failed, retry later");
            }

            time = tm_get_time();
        }
    }
    else if (sock_status(state->socket) == SOCK_STAT_OPENED)
    {
        if (sock_error(state->socket) || sock_sync(state->socket))
        {
            log_e(LOG_GB32960, "socket error, reset protocol");
            gb_reset(state);
        }
        else
        {
            return 0;
        }
    }
	else
	{}
#else
	if(((1 == sockproxy_socketState()) && !gb_allow_sleep) || \
        (state->caltimewait))
	{
		 return 0;
	}

	state->wait     = 0;
    state->online   = 0;
    state->retry    = 0;
    state->waittime = 0;
    state->rcvstep  = -1;
    state->rcvlen   = 0;
    state->datalen  = 0;
#endif
	return -1;
}

static int gb_do_wait(gb_stat_t *state)
{
    if(state->caltimewait)//校时中
    {
        if (tm_get_time() - state->caltimewaittime >= 2500)
        {
            state->caltimewait = 0;
            log_e(LOG_GB32960, "tsp caltime time out\n");
        }
        return -1;
    }

    if (!state->wait)
    {
        return 0;
    }

    if (tm_get_time() - state->waittime > GB_SERVR_TIMEOUT)
    {
        if (state->wait == PROT_LOGIN)
        {
            state->wait = 0;
            log_e(LOG_GB32960, "login time out, retry later");
            gb_reset(state);
        }
        else if (++state->retry > 3)
        {
            if (state->wait == PROT_LOGOUT)
            {
                state->login = 0;
                gb_allow_sleep = 1;
            }

            log_e(LOG_GB32960, "communication time out, reset protocol");
            gb_reset(state);
        }
        else if (state->wait == PROT_LOGOUT)
        {
            uint8_t buf[256];
            int len, res;

            log_e(LOG_GB32960, "logout time out, retry [%d]", state->retry);

            len = gb_pack_logout(buf);
            res = gb32960_MsgSend(buf, len, NULL);
            protocol_dump(LOG_GB32960, "GB32960", buf, len, 1);

            if (res < 0)
            {
                log_e(LOG_GB32960, "socket send error, reset protocol");
                sockproxy_socketclose((int)(PP_SP_COLSE_GB + 1));//by liujian 20190510
                gb_reset(state);
            }
            else if (res == 0)
            {
                log_e(LOG_GB32960, "unack list is full, send is canceled");
            }
            else
            {
                state->waittime = tm_get_time();
            }
        }
    }

    return -1;
}


static int gb_do_login(gb_stat_t *state)
{
    if (state->online)
    {
        return 0;
    }

    if (state->waittime == 0 || tm_get_time() - state->waittime > GB_LOGIN_INTV)
    {
        uint8_t buf[256];
        int len, res;

        log_e(LOG_GB32960, "start to log into server");

        len = gb_pack_login(buf);
        res = gb32960_MsgSend(buf, len, NULL);
        protocol_dump(LOG_GB32960, "GB32960", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_GB32960, "socket send error, reset protocol");
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 2));//by liujian 20190510
            gb_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_GB32960, "unack list is full, send is canceled");
        }
        else
        {
            state->wait = PROT_LOGIN;
            state->waittime = tm_get_time();
        }
    }

    return -1;
}

static int gb_do_suspend(gb_stat_t *state)
{
    if (state->suspend && gb_data_nosending())
    {
        log_e(LOG_GB32960, "communication is suspended");
        //gb_reset(state);
        return -1;
    }

    return 0;
}

static int gb_do_caltime(gb_stat_t *state)
{
    int len,res;
    uint8_t buf[256];

    if(1 == state->calflag)
    {
        state->calflag = 0;
        log_o(LOG_GB32960, "start to tsp caltime\n");
        len = gb_pack_tspcailtime(buf);
        res = gb32960_MsgSend(buf, len, NULL);
        protocol_dump(LOG_GB32960, "GB32960", buf, len, 1);
        if(res < 0)
        {
            log_e(LOG_GB32960, "socket send error, reset protocol");
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 8));
            gb_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_GB32960, "unack list is full, send is canceled");
        }
        else
        {
            state->caltimewait = 1;
            state->caltimewaittime = tm_get_time();
        }
    }

    return 0;
}

static int gb_do_logout(gb_stat_t *state)
{
    //if (!state->can && gb_data_nosending())
    if (!state->can)
    {
        uint8_t buf[256];
        int len, res;

        log_i(LOG_GB32960, "start to log out from server");

        len = gb_pack_logout(buf);
        res = gb32960_MsgSend(buf, len, NULL);
        protocol_dump(LOG_GB32960, "GB32960", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_GB32960, "socket send error, reset protocol");
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 3));//by liujian 20190510
            gb_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_GB32960, "unack list is full, send is canceled");
        }
        else
        {
            state->wait = PROT_LOGOUT;
            state->waittime = tm_get_time();
        }

        return -1;
    }

    return 0;
}


static int gb_do_report(gb_stat_t *state)
{
    gb_pack_t *rpt;
    uint8_t buf[1280];

    if ((rpt = gb_data_get_pack()) != NULL &&
        (sizeof(buf) + GB_SOCKET_RSVD) <= sock_bufsz(state->socket))
    {
        int len, res;

        log_i(LOG_GB32960, "start to send report to server");

        len = gb_pack_report(buf, rpt);
        res = gb32960_MsgSend(buf, len, gb_data_ack_pack);
        protocol_dump(LOG_GB32960, "GB32960", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_GB32960, "socket send error, reset protocol");
            gb_data_put_back(rpt);
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 4));//by liujian 20190510
            gb_reset(state);
            return -1;
        }
        else if (res == 0)
        {
            log_e(LOG_GB32960, "unack list is full, send is canceled");
            gb_data_put_back(rpt);
        }
        else
        {
            gb_data_put_send(rpt);
        }
    }

    return 0;
}

static int gb_makeup_pack(gb_stat_t *state, uint8_t *input, int len, int *uselen)
{
    int rlen = 0;

    while (len--)
    {
        if (input[rlen] == '#' && input[rlen + 1] == '#')
        {
            state->rcvlen  = 0;
            state->datalen = 0;
            state->rcvstep = 0;
        }

        state->rcvbuf[state->rcvlen++] = input[rlen++];

        switch (state->rcvstep)
        {
            case 0: /* read head */
                if (state->rcvlen == 22)
                {
                    state->rcvstep = 1;
                }

                break;

            case 1: /* read data length */
                state->datalen = state->datalen * 256 + input[rlen - 1];

                if (state->rcvlen == 24)
                {
                    state->rcvstep = 2;
                }

                break;

            case 2: /* read data */
                if (state->rcvlen == 24 + state->datalen)
                {
                    state->rcvstep = 3;
                }

                break;

            case 3: /* check sum */
                state->rcvstep = -1;
                *uselen = rlen;
                return 0;

            default: /* unknown */
                state->rcvlen  = 0;
                state->datalen = 0;
                break;
        }
    }

    *uselen = rlen;
    return -1;
}

static int gb_do_receive(gb_stat_t *state)
{
    int ret = 0, rlen;
    uint8_t rcvbuf[1456] = {0}, *input = rcvbuf;
#if !GB32960_SOCKPROXY
    if ((rlen = sock_recv(state->socket, rcvbuf, sizeof(rcvbuf))) < 0)
    {
        log_e(LOG_GB32960, "socket recv error: %s", strerror(errno));
        log_e(LOG_GB32960, "socket recv error, reset protocol");
        gb_reset(state);
        return -1;
    }
#else
	if ((rlen = gb32960_rcvMsg(rcvbuf,1456)) <= 0)
    {
        return 0;
    }
#endif

    while (ret == 0 && rlen > 0)
    {
        int uselen, type, ack, dlen;
        uint8_t *data;

        if (gb_makeup_pack(state, input, rlen, &uselen) != 0)
        {
            break;
        }

        rlen  -= uselen;
        input += uselen;
        protocol_dump(LOG_GB32960, "GB32960", state->rcvbuf, state->rcvlen, 0);

        if (state->rcvbuf[state->rcvlen - 1] !=
            gb_checksum(state->rcvbuf + 2, state->rcvlen - 3))
        {
            log_e(LOG_GB32960, "packet checksum error");
            continue;
        }

        type = state->rcvbuf[2];
        ack  = state->rcvbuf[3];
        dlen = state->rcvbuf[22] * 256 + state->rcvbuf[23];
        data = state->rcvbuf + 24;

        switch (type)
        {
            case PROT_LOGIN:
                if (state->wait != PROT_LOGIN)
                {
                    log_e(LOG_GB32960, "unexpected login acknowlage");
                    break;
                }

                state->wait = 0;

                if (ack != 0x01)
                {
                    log_e(LOG_GB32960, "login is rejected");
                    break;
                }

                state->online = 1;

                if (state->login)
                {
                    gb_data_flush_sending();
                    gb_data_flush_realtm();
                }
                else
                {
                    //gb_data_clear_report();
                    gb_data_clear_error();
                    state->login = 1;
                }

                if (++gb_regseq > 65531)
                {
                    gb_regseq = 1;
                }

                {
                    uint32_t reginf = (gb_regdate << 16) + gb_regseq;
                    cfg_set_user_para(CFG_ITEM_GB32960_REGSEQ, &reginf, sizeof(reginf));
                }

                log_o(LOG_GB32960, "login is succeed");
                break;

            case PROT_LOGOUT:
                if (state->wait != PROT_LOGOUT)
                {
                    log_e(LOG_GB32960, "unexpected logout acknowlage");
                    break;
                }

                if (ack != 0x01)
                {
                    log_e(LOG_GB32960, "logout is rejected!");
                    break;
                }

                log_i(LOG_GB32960, "logout is succeed");
                //gb_reset(state);//by liujian 20190510
                state->login = 0;
                gb_allow_sleep = 1;
                ret = -1;
                break;

            case PROT_CALTIME:
            #if 0
                if (state->wait != PROT_CALTIME)
                {
                    log_e(LOG_GB32960, "unexpected time-calibration acknowlage");
                    break;
                }
            #endif
                if (ack != 0x01)
                {
                    log_e(LOG_GB32960, "time-calibration is rejected!");
                    break;
                }

                state->caltimewait = 0;
                state->caltime = 0;
                
                RTCTIME time;
                int res;
                if(6 == dlen)
                {
                    time.year = data[0] + 2000;
                    time.mon  = data[1];
                    time.mday = data[2];
                    time.hour = data[3];
                    time.min  = data[4];
                    time.sec  = data[5];
                    res = dev_syn_time(&time , TSP_TIME_SOURCE);
                    if(0 == res)
                    {
                        log_o(LOG_GB32960, "time-calibration success\n");
                    }
                }
                else
                {
                    log_e(LOG_GB32960, "time-calibration size error");
                } 
                ret = -1;
                break;

            case PROT_QUREY:
                log_i(LOG_GB32960, "receive query command");
                ret = gb_handle_query(state, data, dlen);
                break;

            case PROT_SETUP:
                log_i(LOG_GB32960, "receive setup command");
                ret = gb_handle_setup(state, data, dlen);
                break;

            case PROT_CONTRL:
                log_i(LOG_GB32960, "receive control command");
                ret = gb_handle_control(state, data, dlen);
                break;

            default:
                log_e(LOG_GB32960, "receive unknown packet");
                break;
        }
    }

    return ret;
}

static int gb_do_config(gb_stat_t *state, gb_cfg_t *cfg)
{
    switch (cfg->cfgid)
    {
        case GB_CFG_ADDR:
            if (cfg_set_user_para(CFG_ITEM_GB32960_URL, cfg->addr.url, sizeof(cfg->addr.url)) ||
                cfg_set_user_para(CFG_ITEM_GB32960_PORT, &cfg->addr.port, sizeof(cfg->addr.port)))
            {
                log_e(LOG_GB32960, "save server address failed");
                return -1;
            }

            memcpy(&gb_addr, &cfg->addr, sizeof(cfg->addr));
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 5));//by liujian 20190510
            gb_reset(state);
            break;

        case GB_CFG_VIN:
            if (cfg_set_user_para(CFG_ITEM_GB32960_VIN, cfg->vin, sizeof(cfg->vin)))
            {
                log_e(LOG_GB32960, "save vin failed");
                return -1;
            }

            strcpy(gb_vin, cfg->vin);
            sockproxy_socketclose((int)(PP_SP_COLSE_GB + 6));//by liujian 20190510
            gb_reset(state);
            break;

        case GB_CFG_REGINTV:
            if (cfg_set_user_para(CFG_ITEM_GB32960_REGINTV, &cfg->regintv, sizeof(cfg->regintv)))
            {
                log_e(LOG_GB32960, "save login period failed");
                return -1;
            }

            gb_regintv = cfg->regintv;
            break;

        case GB_CFG_TIMEOUT:
            if (cfg_set_user_para(CFG_ITEM_GB32960_TIMEOUT, &cfg->timeout, sizeof(cfg->timeout)))
            {
                log_e(LOG_GB32960, "save communication timeout failed");
                return -1;
            }

            gb_timeout = cfg->timeout;
            break;

        case GB_CFG_DATINTV:
            if (cfg_set_user_para(CFG_ITEM_GB32960_INTERVAL, &cfg->datintv, sizeof(cfg->datintv)))
            {
                log_e(LOG_GB32960, "save report period failed");
                return -1;
            }

            gb_data_set_intv(cfg->datintv);
            break;
    }

    return 0;
}

static int gb_shell_setvin(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: gbsetvin <vin>\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 17)
    {
        shellprintf(" error: vin must be 17 charactores\r\n");
        return -1;
    }

    if (gb_set_vin(argv[0]))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int gb_shell_setaddr(int argc, const char **argv)
{
    uint16_t port;

    if (argc != 2)
    {
        shellprintf(" usage: gbsetaddr <server url> <server port>\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 63)
    {
        shellprintf(" error: url length can't over 63 charactores\r\n");
        return -1;
    }

    if (sscanf(argv[1], "%hu", &port) != 1)
    {
        shellprintf(" error: \"%s\" is a invalid port\r\n", argv[1]);
        return -1;
    }

    if (gb_set_addr(argv[0], port))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int gb_shell_nosend(int argc, const char **argv)
{
    unsigned int val;
    if (argc != 1)
    {
        shellprintf(" usage: gbnosend <suspend> \r\n");
        return -1;
    }

	sscanf(argv[0], "%u", &val);

    gbnosend = (unsigned char)val;

    sleep(1);

    return 0;
}

static int gb_shell_setcaltime(int argc, const char **argv)
{
    state.calflag = 1;
    sleep(1);

    return 0;
}

static int gb_shell_settmout(int argc, const const char **argv)
{
    uint16_t timeout;

    if (argc != 1 || sscanf(argv[0], "%hu", &timeout) != 1)
    {
        shellprintf(" usage: gbsetmout <timeout in seconds> \r\n");
        return -1;
    }

    if (timeout == 0)
    {
        shellprintf(" error: timeout value can't be 0\r\n");
        return -1;
    }

    if (gb_set_timeout(timeout))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    return 0;
}

static int gb_shell_setregtm(int argc, const const char **argv)
{
    uint16_t intv;

    if (argc != 1 || sscanf(argv[0], "%hu", &intv) != 1)
    {
        shellprintf(" usage: gbsetregtm <time in seconds>\r\n");
        return -1;
    }

    if (gb_set_regintv(intv))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    return 0;
}

static int gb_shell_status(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = GB_MSG_STATUS;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int gb_shell_suspend(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = GB_MSG_SUSPEND;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int gb_shell_resume(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = GB_MSG_RESUME;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int gb_shell_fuelcell(int argc, const const char **argv)
{
    uint8_t is_user;

    if (argc != 1)
    {
        shellprintf(" usage: fuelcell <0/1>\r\n");
        return -1;
    }

    is_user = atoi(argv[0]);
    if(is_user)
    {
        is_user = 1;
    }
    else
    {
        is_user = 0;
    }
    
    cfg_set_para(CFG_ITEM_FUELCELL, &is_user, sizeof(is_user));

    return 0;
}

static int gb_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
{
    TCOM_MSG_HEADER msg;
    int network;

    if(!sockproxy_nm_apncardtype())
    {
        if (NM_PUBLIC_NET != type)
        {
            return 0;
        }
    }
    else
	{
		if (NM_PRIVATE_NET != type)
		{
			return 0;
		}
	}

    switch (nmmsg)
    {
        case NM_REG_MSG_CONNECTED:
            network = 1;
            break;

        case NM_REG_MSG_DISCONNECTED:
            network = 0;
            break;

        default:
            return -1;
    }

    msg.msgid    = GB_MSG_NETWORK;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(network);

    return tcom_send_msg(&msg, &network);
}


static int gb_can_callback(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    int ret = 0;

    switch (event)
    {
        case CAN_EVENT_ACTIVE:
            {
                TCOM_MSG_HEADER msg;

                msg.sender   = MPU_MID_GB32960;
                msg.receiver = MPU_MID_GB32960;
                msg.msglen   = 0;
                msg.msgid    = GB_MSG_CANON;
                ret = tcom_send_msg(&msg, NULL);
                break;
            }

        case CAN_EVENT_SLEEP:
        case CAN_EVENT_TIMEOUT:
            {
                TCOM_MSG_HEADER msg;
                log_o(LOG_GB32960, "GB_MSG_CANOFF!");
                msg.sender   = MPU_MID_GB32960;
                msg.receiver = MPU_MID_GB32960;
                msg.msglen   = 0;
                msg.msgid    = GB_MSG_CANOFF;
                ret = tcom_send_msg(&msg, NULL);
                break;
            }

        default:
            break;
    }

    return ret;
}

static void gb_show_status(gb_stat_t *state)
{
    shellprintf(" GB32960 status\r\n");
    shellprintf("  can         : %s\r\n", state->can ? "active" : "inactive");
    shellprintf("  network     : %s\r\n", state->network ? "on" : "off");
    shellprintf("  socket      : %s\r\n", sock_strstat(sock_status(state->socket)));
    shellprintf("  server      : %s\r\n", state->online ? "connected" : "unconnected");
    shellprintf("  suspend     : %s\r\n", state->suspend ? "yes" : "no");
    shellprintf("  log out     : %s\r\n", state->login ? "no" : "yes");

    shellprintf("  server url  : %s : %u\r\n", gb_addr.url, gb_addr.port);
    shellprintf("  vin         : %s\r\n", gb_vin);

    shellprintf(" gb allow sleep : %s\r\n", gb_allow_sleep?"yes":"no");
    shellprintf("pp allow sleep : %s\r\n", GetPrvtProt_Sleep()?"yes":"no");
    shellprintf("  rpt period  : %u s\r\n", gb_data_get_intv());
    shellprintf("  srv timeout : %u s\r\n", gb_timeout);
    shellprintf("  reg period  : %u s\r\n", gb_regintv);
    shellprintf("  reg seq     : %u\r\n", gb_regseq);
    shellprintf("  fix iccid   : %s\r\n", gb_iccid);
    shellprintf("  batt code   : %s\r\n", gb_battcode);
}

#if GB32960_THREAD
static void *gb_main(void)
{
    int tcomfd;
    //static gb_stat_t state;

    log_o(LOG_GB32960, "GB32960 thread running");

    prctl(PR_SET_NAME, "GB32960");

    if ((tcomfd = tcom_get_read_fd(MPU_MID_GB32960)) < 0)
    {
        log_e(LOG_GB32960, "get module pipe failed, thread exit");
        return NULL;
    }

    memset(&state, 0, sizeof(gb_stat_t));

    if ((state.socket = sock_create("GB32960", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_GB32960, "create socket failed, thread exit");
        return NULL;
    }

    while (1)
    {
        TCOM_MSG_HEADER msg;
        gb_msg_t msgdata;
        int res;

        res = protocol_wait_msg(MPU_MID_GB32960, tcomfd, &msg, &msgdata, 200);

        if (res < 0)
        {
            log_e(LOG_GB32960, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            case GB_MSG_NETWORK:
                log_i(LOG_GB32960, "get NETWORK message: %d", msgdata.network);

                if (state.network != msgdata.network)
                {
                	sockproxy_socketclose((int)(PP_SP_COLSE_GB + 7));//by liujian 20190510
                    gb_reset(&state);
                    state.network = msgdata.network;
                }

                break;

            case PM_MSG_EMERGENCY:
                gb_data_emergence(1);
                powerOffFlag = 1;
                break;

            case PM_MSG_RUNNING:
            	 powerOffFlag = 0;
            	 SetPrvtProt_Awaken((int)PM_MSG_RUNNING);
            	 Setsocketproxy_Awaken();
            	break;
            case PM_MSG_OFF:
                gb_data_emergence(0);
                powerOffFlag = 1;
                break;

            case GB_MSG_CANON:
                log_i(LOG_GB32960, "get CANON message");
                Setsocketproxy_Awaken();
                SetPrvtProt_Awaken((int)GB_MSG_CANON);
                state.can = 1;
                gb_allow_sleep = 0;
                break;

            case GB_MSG_CANOFF:
                log_i(LOG_GB32960, "get CANOFF message");
                state.can = 0;
                gb_allow_sleep = !state.online;
                break;

            case GB_MSG_SUSPEND:
                log_i(LOG_GB32960, "get SUSPEND message");
                state.suspend = 1;
                gb_data_set_pendflag(1);
                break;

            case GB_MSG_RESUME:
                state.suspend = 0;
                log_i(LOG_GB32960, "get RESUME message");
                gb_data_set_pendflag(0);
                break;

            case GB_MSG_STATUS:
                log_i(LOG_GB32960, "get STATUS message");
                gb_show_status(&state);
                break;

            case GB_MSG_ERRON:
                log_i(LOG_GB32960, "get ERRON message");
                state.errtrig = 1;
                break;

            case GB_MSG_ERROFF:
                log_i(LOG_GB32960, "get ERROFF message");
                state.errtrig = 0;
                break;

            case GB_MSG_CONFIG:
                log_i(LOG_GB32960, "get CONFIG message");
                gb_do_config(&state, &msgdata.cfg);
                break;

            case MPU_MID_MID_PWDG:
                pwdg_feed(MPU_MID_GB32960);
                break;
        }

		if(gbnosend != 0) continue;

        static char IGNnewSt,IGNoldSt = 0;

        IGNnewSt = dev_get_KL15_signal();
        if(IGNoldSt != IGNnewSt)
        {
            IGNoldSt = IGNnewSt;
            if(1 == IGNnewSt)//IGN ON
            {
                state.calflag = 1;
            }
        }
		
        res = gb_do_checksock(&state) ||	//检查连接
              gb_do_receive(&state)   ||	//socket 接收
              gb_do_wait(&state)      ||	//等待
              gb_do_login(&state)     ||	//登入
              gb_do_suspend(&state)   ||	//暂停
              gb_do_report(&state)    ||	//发实时数据
              gb_do_logout(&state);			//登出

        if((1 == sockproxy_socketState()) || \
                    (1 == sockproxy_sgsocketState()))
        {
            gb_do_caltime(&state);//校时
        }
    }

    sock_delete(state.socket);
    return NULL;
}
#endif

int gb_run(void)
{
#if GB32960_THREAD
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)gb_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_GB32960, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
#else
	static int tcomfd = -1;
	TCOM_MSG_HEADER msg;
	gb_msg_t msgdata;
	int res;
	
    log_o(LOG_GB32960, "GB32960 running");

    if ((tcomfd < 0) && ((tcomfd = tcom_get_read_fd(MPU_MID_GB32960)) < 0))
    {
        log_e(LOG_GB32960, "get module pipe failed, running exit");
        return -1;
    }

	res = protocol_wait_msg(MPU_MID_GB32960, tcomfd, &msg, &msgdata, 200);

	if (res < 0)
	{
		log_e(LOG_GB32960, "running exit unexpectedly, error:%s", strerror(errno));
		return -1;
	}

	switch (msg.msgid)
	{
		case GB_MSG_NETWORK:
			log_i(LOG_GB32960, "get NETWORK message: %d", msgdata.network);

			if (state.network != msgdata.network)
			{
				gb_reset(&state);
				state.network = msgdata.network;
			}

			break;

		case PM_MSG_EMERGENCY:
			gb_data_emergence(1);
			break;

		case PM_MSG_RUNNING:
		case PM_MSG_OFF:
			gb_data_emergence(0);
			break;

		case GB_MSG_CANON:
			log_i(LOG_GB32960, "get CANON message");
			state.can = 1;
			gb_allow_sleep = 0;
			break;

		case GB_MSG_CANOFF:
			log_i(LOG_GB32960, "get CANOFF message");
			state.can = 0;
			gb_allow_sleep = !state.online;
			break;

		case GB_MSG_SUSPEND:
			log_i(LOG_GB32960, "get SUSPEND message");
			state.suspend = 1;
			gb_data_set_pendflag(1);
			break;

		case GB_MSG_RESUME:
			state.suspend = 0;
			log_i(LOG_GB32960, "get RESUME message");
			gb_data_set_pendflag(0);
			break;

		case GB_MSG_STATUS:
			log_i(LOG_GB32960, "get STATUS message");
			gb_show_status(&state);
			break;

		case GB_MSG_ERRON:
			log_i(LOG_GB32960, "get ERRON message");
			state.errtrig = 1;
			break;

		case GB_MSG_ERROFF:
			log_i(LOG_GB32960, "get ERROFF message");
			state.errtrig = 0;
			break;

		case GB_MSG_CONFIG:
			log_i(LOG_GB32960, "get CONFIG message");
			gb_do_config(&state, &msgdata.cfg);
			break;

		case MPU_MID_MID_PWDG:
			pwdg_feed(MPU_MID_GB32960);
			break;
	}

	res = gb_do_checksock(&state) ||	//���socket����
		  gb_do_receive(&state) ||		//socket���ݽ���
		  gb_do_wait(&state) ||			//��ʱ�ȴ��������ǳ���
		  gb_do_login(&state) ||		//����
		  gb_do_suspend(&state) ||		//ͨ����ͣ
		  gb_do_report(&state) ||		//����ʵʱ��Ϣ
		  gb_do_logout(&state);			//�ǳ�

	return res;
#endif
}


static int gb_allow_sleep_handler(PM_EVT_ID id)
{
    return (gb_allow_sleep && GetPrvtProt_Sleep() && \
            sockproxy_Sleep() && PP_rmtDiag_sleepflag());
}

int gb_init(INIT_PHASE phase)
{
    int ret = 0;
    uint32_t reginf = 0, cfglen;

    ret |= gb_data_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(gb_vin, 0, sizeof(gb_vin));
            memset(gb_iccid, 0, sizeof(gb_iccid));
            memset(gb_battcode, 0, sizeof(gb_battcode));
			memset(&state, 0, sizeof(gb_stat_t));
            gb_addr.url[0] = 0;
            gb_addr.port   = 0;
            gb_timeout   = 5;
            gb_regdate   = 0;
            gb_regseq    = 0;
            gb_allow_sleep     = 1;
            state.calflag = 1;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:

        	//cfglen = sizeof(dbcpath);
			//ret |= cfg_get_para(CFG_ITEM_DBC_PATH,dbcpath , &cfglen);
			//if (NULL == strstr(dbcpath,".dbc"))
			//{
			//	memset(dbcpath, 0 ,sizeof(dbcpath));
			//	log_e(LOG_PDOTA, "dbc file is empty");
			//	pdota_setdbc(dbcpath);
			//}

            cfglen = sizeof(gb_addr.url);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_URL, gb_addr.url, &cfglen);
            cfglen = sizeof(gb_addr.port);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_PORT, &gb_addr.port, &cfglen);
            cfglen = sizeof(gb_vin);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_VIN, gb_vin, &cfglen);
            cfglen = sizeof(gb_regintv);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_REGINTV, &gb_regintv, &cfglen);
            cfglen = sizeof(reginf);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_REGSEQ, &reginf, &cfglen);
            cfglen = sizeof(gb_timeout);
            ret |= cfg_get_user_para(CFG_ITEM_GB32960_TIMEOUT, &gb_timeout, &cfglen);

            if (ret == 0)
            {
                gb_regdate = reginf >> 16;
                gb_regseq  = reginf;
            }

            ret |= pm_reg_handler(MPU_MID_GB32960, gb_allow_sleep_handler);
            ret |= shell_cmd_register("gbstat", gb_shell_status, "show GB32960 status");
            ret |= shell_cmd_register_ex("gbsetvin", "setvin", gb_shell_setvin, "set GB32960 vin");
            ret |= shell_cmd_register("gbsetaddr", gb_shell_setaddr, "set GB32960 server address");
            ret |= shell_cmd_register_ex("gbsettmout", "transtimeout", gb_shell_settmout,
                                         "set GB32960 server timeout");
            ret |= shell_cmd_register_ex("gbsetregtm", "logintime", gb_shell_setregtm,
                                         "set GB32960 register period");
            ret |= shell_cmd_register_ex("gbsuspend", "tcpshut", gb_shell_suspend, "stop GB32960");
            ret |= shell_cmd_register_ex("gbresume", "tcpopen", gb_shell_resume, "open GB32960");
            ret |= shell_cmd_register("fuelcell", gb_shell_fuelcell, "set GB32960 whether use the fuelcell");

            ret |= can_register_callback(gb_can_callback);
            ret |= nm_register_status_changed(gb_nm_callback);
			
			ret |= shell_cmd_register("gbnosend", gb_shell_nosend, "GB32960 don't send data");
            ret |= shell_cmd_register("gbcaltime", gb_shell_setcaltime, "GB32960 cal time");
            break;
    }

    return ret;
}


int gb_set_addr(const char *url, uint16_t port)
{
    TCOM_MSG_HEADER msg;
    gb_cfg_t cfg;

    if (strlen(url) > 255)
    {
        log_e(LOG_GB32960, "url length must be less than 255: %s", url);
        return -1;
    }

    msg.msgid    = GB_MSG_CONFIG;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(cfg.addr) + sizeof(cfg.cfgid);

    cfg.cfgid = GB_CFG_ADDR;
    strcpy(cfg.addr.url, url);
    cfg.addr.port = port;

    return tcom_send_msg(&msg, &cfg);
}


int gb_set_vin(const char *vin)
{
    TCOM_MSG_HEADER msg;
    gb_cfg_t cfg;

    if (strlen(vin) != 17)
    {
        log_e(LOG_GB32960, "vin number must be 17 charactor: %s", vin);
        return -1;
    }

    msg.msgid    = GB_MSG_CONFIG;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(cfg.vin) + sizeof(cfg.cfgid);

    cfg.cfgid = GB_CFG_VIN;
    strcpy(cfg.vin, vin);

    return tcom_send_msg(&msg, &cfg);
}

int gb_set_datintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    gb_cfg_t cfg;

    msg.msgid    = GB_MSG_CONFIG;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(cfg.datintv) + sizeof(cfg.cfgid);

    cfg.cfgid = GB_CFG_DATINTV;
    cfg.datintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int gb_set_regintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    gb_cfg_t cfg;

    msg.msgid    = GB_MSG_CONFIG;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(cfg.regintv) + sizeof(cfg.cfgid);

    cfg.cfgid = GB_CFG_REGINTV;
    cfg.regintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int gb_set_timeout(uint16_t timeout)
{
    TCOM_MSG_HEADER msg;
    gb_cfg_t cfg;

    msg.msgid    = GB_MSG_CONFIG;
    msg.sender   = MPU_MID_GB32960;
    msg.receiver = MPU_MID_GB32960;
    msg.msglen   = sizeof(cfg.timeout) + sizeof(cfg.cfgid);

    cfg.cfgid = GB_CFG_TIMEOUT;
    cfg.timeout = timeout;

    return tcom_send_msg(&msg, &cfg);
}

/******************************************************
*��������gb32960_MsgSend
*��  �Σ�
		Msg -- ����
		len -- ���ݳ���
		sync -- �ص�����
*����ֵ��
*��  ������������
*��  ע��
******************************************************/
static int gb32960_MsgSend(uint8_t* Msg,int len,void (*sync)(void))
{
	int res;
#if !GB32960_SOCKPROXY
	res = sock_send(state.socket, Msg, len, sync);
#else
	gb_data_ack_pack();
	res = sockproxy_MsgSend(Msg,len, sync);
#endif
	return res;
}

/******************************************************
*��������gb32960_getURL
*��  �Σ�
		
		
*����ֵ��
*��  ������ȡurl
*��  ע��
******************************************************/
void gb32960_getURL(void* ipaddr)
{
	strcpy(((svr_addr_t*)ipaddr)->url, gb_addr.url);
    ((svr_addr_t*)ipaddr)->port = gb_addr.port;
}

/******************************************************
*��������gb32960_getAllowSleepSt
*��  �Σ�
		
		
*����ֵ��
*��  ������ȡ����˯�߱�־
*��  ע��
******************************************************/
int gb32960_getAllowSleepSt(void)
{
	return gb_allow_sleep;
}

/******************************************************
*��������gb32960_getsuspendSt
*��  �Σ�
		
		
*����ֵ��ture -- tcp��ͣ
*��  ������ȡ��ͣ״̬
*��  ע��
******************************************************/
int gb32960_getsuspendSt(void)
{
	return (!state.suspend && !gb_data_noreport());
}
 
unsigned char gb32960_PowerOffSt(void)
{
	return powerOffFlag;
}

/*
 * vin有效性
 */
unsigned char gb32960_vinValidity(void)
{
	if((strcmp(gb_vin,"00000000000000000") == 0) || \
			(strcmp(gb_vin,"") == 0))
	{
		return 0;
	}

	return 1;
}

/*
 * 读取vin
 */
void gb32960_getvin(char* vin)
{
	strcpy(vin, gb_vin);
}

/*
 * 获取gsm网络状态
 */
int gb32960_networkSt(void)
{
	return state.network;
}

/*
 * 获取gb登出状态
 */
int gb32960_gbLogoutSt(void)
{
	if(1 == gb_allow_sleep)
	{
		return 1;
	}

	return 0;
}

/*
 * 获取can bus激活状态
 */
int gb32960_gbCanbusActiveSt(void)
{
    return state.can;
}

/*
* 读取国标每秒实时数据（整包数据：包含协议头和数据、校验位）
*/
uint8_t gb_data_perReportPack(uint8_t *data,int *len)
{
    int datalen = 0;
    int length = 0;
    uint8_t *datalenhighbyte,*datalenlowbyte;
    if(1 == gb_data_perPackValid())
    {
        datalen = gb_pack_head(0x02, 0xfe, data);
        datalenhighbyte = &data[datalen++];
        datalenlowbyte  = &data[datalen++];
        /* report data */
        gb_data_perPack(data + datalen,&length);
        /* data length */
        *datalenhighbyte = length >> 8;
        *datalenlowbyte = length;
        datalen = datalen + length;

        /* check sum */
        data[datalen] = gb_checksum(data + 2, datalen - 2);
        *len = datalen + 1;
        return 1;
    }

    return 0;
}