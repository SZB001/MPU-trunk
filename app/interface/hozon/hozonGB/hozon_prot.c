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
#include "hozon.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "hozon_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "../support/protocol.h"
#include "pm_api.h"
#include "at.h"
#include "ftp_api.h"
#include "dev_api.h"
#include "rds.h"
#include "gb32960_api.h"
#include "at_api.h"

#define HZ_PROT_LOGIN      0x01
#define HZ_PROT_REPORT     0x02
#define HZ_PROT_REISSUE    0x03
#define HZ_PROT_LOGOUT     0x04
#define HZ_PROT_HBEAT      0x07
#define HZ_PROT_CALTIME    0x08
#define HZ_PROT_QUREY      0x80
#define HZ_PROT_SETUP      0x81
#define HZ_PROT_CONTRL     0x82

#define HZ_SERVR_TIMEOUT    (1000 * hz_timeout)
#define HZ_LOGIN_INTV       (1000 * hz_regintv)
#define HZ_SOCKET_RSVD      1024

#define HZ_WAKEUP_TIME      3*60*1000  /*3 mint*/

typedef enum
{
    HZ_CFG_URL = 1,
    HZ_CFG_PORT,
    HZ_CFG_ADDR,
    HZ_CFG_VIN,
    HZ_CFG_DATINTV,
    HZ_CFG_REGINTV,
    HZ_CFG_TIMEOUT,
} HZ_CFG_TYPE;


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
    int ftptrans;
    int upgtype;
    uint64_t ftpuptime;
    uint64_t hbeattime;
} hz_stat_t;

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
} hz_cfg_t;

typedef union
{
    /* for HZ_MSG_NETWORK */
    int network;
    /* for HZ_MSG_SOCKET */
    int socket;
    /* for HZ_MSG_CONFIG */
    hz_cfg_t cfg;
    struct
    {
        int ftp_evt;
        int ftp_par;
    };
} hz_msg_t;

static char       hz_vin[18];
static svr_addr_t hz_addr;
static uint16_t   hz_timeout;
static uint16_t   hz_regintv;
static uint16_t   hz_regdate;
static uint16_t   hz_regseq;
static char       hz_iccid[21];
static char       hz_battcode[64];
static int        hz_ota_link_state = NM_OTA_LINK_NORMAL;
timer_t hz_wakeup_timer; 

int 	   hz_allow_sleep;

extern int hz_is_allow_login;


/****************************************************************
 function:     hz_get_tbox_ver
 description:  get all version
 input:        unsigned int size
 output:       unsigned char *ver
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int hz_get_tbox_ver(unsigned char *ver, unsigned int size)
{
    int ret;
    char version[TBOX_FW_VER_LEN];
    unsigned int fw, mpu_app, mcu_app;

    memset(version,0,sizeof(version));
    ret = upg_get_fw_ex_ver(version, TBOX_FW_VER_LEN);

    /* ex ver: "+HZCOMMxxx" */
    if (ret != 10 || 1 != sscanf(version, "+%*6s%3x", &fw))
    {
        fw = 0;
        log_e(LOG_DEV, "externd version error,ret:%d,ver:%s", ret, version);
    }

    /* mpu app ver: "MPU_J303HZCOMMxxx[A_xx]" */
    memset(version,0,sizeof(version));
    upg_get_app_ver((unsigned char *)version,COM_APP_VER_LEN);
    if (1 != sscanf(version, "MPU_J303HZCOMM%d[%*s]", &mpu_app))
    {
        mpu_app = 0;
        log_e(LOG_DEV, "MPU app version error,ver:%s", version);
    }

    /* mcu app ver: "MCU_J303HZCOMMxxx[A_xx]" */
    memset(version,0,sizeof(version));
    upg_get_mcu_run_ver((unsigned char *)version,APP_VER_LEN);
    if (1 != sscanf(version, "MCU_J303HZCOMM%d[%*s]", &mcu_app))
    {
        mcu_app = 0;
        log_e(LOG_DEV, "MCU app version error,ver:%s", version);
    }

    memset(ver, 0, size);
    snprintf((char *)&ver[0], size, "%02x", mcu_app);
    snprintf((char *)&ver[2], size, "%02x", mpu_app);
    snprintf((char *)&ver[4], size, "%d", fw % 10);

    return 0;
}

static void hz_reset(hz_stat_t *state)
{
    sock_close(state->socket);
    state->wait     = 0;
    state->online   = 0;
    state->retry    = 0;
    state->waittime = 0;
    state->rcvstep  = -1;
    state->rcvlen   = 0;
    state->datalen  = 0;
}

static int hz_checksum(uint8_t *data, int len)
{
    uint8_t cs = 0;

    while (len--)
    {
        cs ^= *data++;
    }

    return cs;
}

static int hz_pack_head(int type, int dir, uint8_t *buf)
{
    int len = 0;

    buf[len++] = '#';
    buf[len++] = '#';
    buf[len++] = type;
    buf[len++] = dir;
    /* vin number */
    memcpy(buf + len, hz_vin, 17);
    len += 17;
    /* encryption mode : */
    /* 0x01 : NONE */
    /* 0x02 : RSA */
    /* 0x03 : AES128 */
    buf[len++] = 0x01;

    return len;
}

static int hz_pack_report(uint8_t *buf, hz_pack_t *rpt)
{
    int len;

    len = hz_pack_head(rpt->type, 0xfe, buf);
    /* data length */
    buf[len++] = rpt->len >> 8;
    buf[len++] = rpt->len;
    /* report data */
    memcpy(buf + len, rpt->data, rpt->len);
    len += rpt->len;
    /* check sum */
    buf[len] = hz_checksum(buf + 2, len - 2);

    return len + 1;
}

static int hz_pack_login(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp, plen, pos;


    tm_get_abstime(&time);

    if (hz_regdate != time.mon * 100 + time.mday)
    {
        uint32_t reginf;

        hz_regdate  = time.mon * 100 + time.mday;
        hz_regseq   = 1;
        reginf      = (hz_regdate << 16) + hz_regseq;
        rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *)&reginf, sizeof(reginf));
    }

    /* header */
    pos = hz_pack_head(HZ_PROT_LOGIN, 0xfe, buf);
    /* jump length */
    len = pos + 2;
    /* time */
    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
    buf[len++] = hz_regseq >> 8;
    buf[len++] = hz_regseq;

    /* iccid */
    at_get_iccid(hz_iccid);
    memcpy(buf + len, hz_iccid, 20);
    len += 20;
    /* support only one battery  */
    buf[len++] = 1;
    /* battery code */
    tmp = strlen(hz_battcode);
    buf[len++] = tmp;
    memcpy(buf + len, hz_battcode, tmp);
    len += tmp;
    /* adjust length */
    plen = len - pos - 2;
    buf[pos++] = plen >> 8;
    buf[pos++] = plen;
    /* check sum */
    buf[len] = hz_checksum(buf + 2, len - 2);

    return len + 1;
}
static int hz_pack_hbeat(uint8_t *buf)
{
    int len = 0, plen, pos;
    
    /* header */
    pos = hz_pack_head(HZ_PROT_HBEAT, 0xFE, buf);
    /* jump length */
    len = pos + 2;

    /* adjust length */
    plen = len - pos - 2;
    buf[pos++] = plen >> 8;
    buf[pos++] = plen;
    /* check sum */
    buf[len] = hz_checksum(buf + 2, len - 2);

    return len + 1;
}

static int hz_pack_logout(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp;

    tmp = hz_regseq == 1 ? 65531 : hz_regseq - 1;
    tm_get_abstime(&time);

    len = hz_pack_head(HZ_PROT_LOGOUT, 0xfe, buf);
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
    buf[len]   = hz_checksum(buf + 2, len - 2);

    return len + 1;
}

static int hz_handle_query(hz_stat_t *state, uint8_t *data, int len)
{
    int slen, ulen, res;
    uint8_t sdata[1024], *plen;

    if (len < 7)
    {
        return 0;
    }

    slen = hz_pack_head(HZ_PROT_QUREY, 0x01, sdata);
    plen = sdata + slen;
    slen += 2;
    memcpy(sdata + slen, data, 6);
    data += 6;
    slen += 6;

    if (*data == 0 || *data > 252 || *data != len - 7)
    {
        sdata[slen++] = 0xff;
    }
    else
    {
        uint8_t *pcnt = sdata + slen++;
        int incnt, outcnt;

        for (incnt = *data++, outcnt = 0; incnt > 0; incnt--, outcnt++)
        {
            int tmpint, para = *data++;
            //unsigned char version[64];

            sdata[slen++] = para;

            switch (para)
            {
                /* local storage period */
                case 0x01:

                /* warnning report period */
                case 0x03:
                    sdata[slen++] = 1000 >> 8;
                    sdata[slen++] = 1000 & 0xff;
                    break;

                /* normal report period */
                case 0x02:
                    tmpint = hz_data_get_intv();
                    sdata[slen++] = tmpint >> 8;
                    sdata[slen++] = tmpint;
                    break;

                /* server address length */
                case 0x04:
                    sdata[slen++] = strlen(hz_addr.url);
                    break;

                /* server address */
                case 0x05:
                    tmpint = strlen(hz_addr.url);
                    memcpy(sdata + slen, hz_addr.url, tmpint);
                    slen += tmpint;
                    break;

                /* server port */
                case 0x06:
                    sdata[slen++] = hz_addr.port >> 8;
                    sdata[slen++] = hz_addr.port;
                    break;

                /* OEM string */
                case 0x07:
                    {
                        memcpy(sdata + slen, COM_HW_VER, 5);
                        slen += 5;
                        break;
                    }

                case 0x08:
                    {
                        unsigned char ver[5];
                        hz_get_tbox_ver(ver, sizeof(ver));
                        memcpy(sdata + slen, ver, 5);
                        slen += 5;
                        break;
                    }

                /* heart beat period */
                case 0x09:
                    sdata[slen++] = 0xff;
                    break;

                /* terminal timeout */
                case 0x0a:
                    sdata[slen++] = 0;
                    sdata[slen++] = 10;
                    break;

                /* server timeout */
                case 0x0b:
                    sdata[slen++] = hz_timeout >> 8;
                    sdata[slen++] = hz_timeout;
                    break;

                /* relogin period */
                case 0x0c:
                    sdata[slen++] = hz_regintv;
                    break;

                /* not used */
                case 0x0f:
                    sdata[slen++] = 0;

                case 0x0d:
                case 0x10:
                    sdata[slen++] = 0;
                    break;

                /* VIN */
                case 0x80:
                    memcpy(sdata + slen, hz_vin, 17);
                    slen += 17;
                    break;
                    #if 0

                /* MCU version */
                case 0x81:
                    if (upg_get_mcu_ver(version, sizeof(version)) != 0)
                    {
                        sdata[slen++] = 0;
                    }
                    else
                    {
                        tmpint = strlen(version);
                        sdata[slen++] = tmpint;
                        memcpy(sdata + slen, version, tmpint);
                        slen += tmpint;
                    }

                    break;

                /* MPU version */
                case 0x82:
                    tmpint = strlen(COM_APP_SYS_VER);
                    sdata[slen++] = tmpint;
                    memcpy(sdata + slen, COM_APP_SYS_VER, tmpint);
                    slen += tmpint;
                    break;

                /* FW version */
                case 0x83:
                    if (upg_get_fw_ver(version, sizeof(version)) != 0)
                    {
                        sdata[slen++] = 0;
                    }
                    else
                    {
                        tmpint = strlen(version);
                        sdata[slen++] = tmpint;
                        memcpy(sdata + slen, version, tmpint);
                        slen += tmpint;
                    }

                    break;

                /* log storage */
                case 0x84:
                    sdata[slen++] = 0;
                    break;
                    #endif

                default:
                    slen--;
                    outcnt--;
                    break;
            }
        }

        *pcnt = outcnt;
    }

    ulen = slen - 24;
    *plen++ = ulen >> 8;
    *plen++ = ulen;

    sdata[slen] = hz_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_HOZON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_HOZON, "socket send error, reset protocol");
        hz_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_HOZON, "unack list is full, send is canceled");
    }

    return 0;
}



static int hz_check_setup(uint8_t *data, int len)
{
    int cnt, dlen = 0;
    uint32_t tmpint, urllen = 256, curllen = 256;

    for (cnt = *data++, len--; len > 0 && cnt > 0; cnt--, len -= dlen, data += dlen)
    {
        switch (*data)
        {
            /* local storage period */
            case 0x01:

            /* warnning report period */
            case 0x03:

            /* server port */
            case 0x06:

            /* common server port */
            case 0x0f:

            /* terminal timeout */
            case 0x0a:
                dlen = 3;
                break;

            /* normal report period */
            case 0x02:

            /* server timeout */
            case 0x0b:
                tmpint = ((uint32_t)data[1] << 8) + data[2];

                if (tmpint < 1 || tmpint > 600)
                {
                    return -1;
                }

                dlen = 3;
                break;

            /* server address length */
            case 0x04:
                urllen = data[1];
                dlen = 2;
                break;

            /* server address */
            case 0x05:
                if (urllen > 255)
                {
                    return -1;
                }

                dlen = urllen + 1;
                break;

            /* heart beat period */
            case 0x09:

            /* relogin period */
            case 0x0c:
                if (data[1] < 1 || data[1] > 240)
                {
                    return -1;
                }

                dlen = 2;
                break;

            /* common server address length */
            case 0x0d:
                curllen = data[1];
                dlen = 2;
                break;

            /* common server address */
            case 0x0e:
                if (curllen > 255)
                {
                    return -1;
                }

                dlen = curllen + 1;
                break;

            case 0x10:
                #if 0

            /* log storage */
            case 0x84:
                dlen = 2;
                break;
                #endif

            /* VIN */
            case 0x80:
                dlen = 18;
                break;

            default:
                return -1;

        }
    }

    return (cnt != 0 || len != 0);
}

static int hz_handle_setup(hz_stat_t *state, uint8_t *data, int len)
{
    int slen, res;
    uint8_t sdata[64];

    if (len < 7)
    {
        return 0;
    }

    slen = hz_pack_head(HZ_PROT_SETUP, 0x01, sdata);
    sdata[slen++] = 0;
    sdata[slen++] = 6;
    memcpy(sdata + slen, data, 6);
    data += 6;
    slen += 6;

    if (hz_check_setup(data, len - 6))
    {
        sdata[3] = 0x02;
    }
    else
    {
        volatile int clen/*, tmpint*/;
        int urllen = 0, curllen = 0;
        char tmpstr[256];
		unsigned char gbswitch = 0, gburllen = 0;
		unsigned short port;
		
		
        for (len -= 7, data++, clen = 0; len > 0; data += clen, len -= clen)
        {
            switch (*data)
            {
                /* local storage period */
                case 0x01:

                /* warnning report period */
                case 0x03:

                /* terminal timeout */
                case 0x0a:
                    clen = 3;
                    break;

                /* normal report period */
                case 0x02:
                    hz_set_datintv((uint16_t)data[1] * 0xff + data[2]);
                    clen = 3;
                    break;

                /* server address length */
                case 0x04:
                    urllen = data[1];
                    log_i(LOG_HOZON,"server address length:%d",urllen);
                    clen = 2;
                    break;

                /* server address */
                case 0x05:
					memset(tmpstr ,0,sizeof(tmpstr) );
                    memcpy(tmpstr, data + 1, urllen);
                    tmpstr[urllen] = 0;
                    break;

                /* server port */
                case 0x06:
                    port = ((uint16_t)data[1] << 8) + data[2];
                    log_i(LOG_HOZON,"server port:%d",port);
                    hz_set_addr(tmpstr, port);
                    clen = 3;
                    break;

                /* heart beat period */
                case 0x09:
                    //hz_set_hbtintv(data[1]);
                    clen = 2;
                    break;

                /* server timeout */
                case 0x0b:
                    hz_set_timeout((uint16_t)data[1] * 0xff + data[2]);
                    clen = 3;
                    break;

                /* relogin period */
                case 0x0c:
                    hz_set_regintv(data[1]);
                    clen = 2;
                    break;

                /* common server address length */
                case 0x0d:
                    curllen = data[1];
                    clen = 2;
                    break;

                /* common server address */
                case 0x0e:
                    clen = curllen + 1;
                    break;

                /* common server port */
                case 0x0f:
                    clen = 3;
                    break;

                case 0x10:
                    clen = 2;
                    break;

                /* VIN */
                case 0x80:
                    memcpy(tmpstr, data + 1, 17);
                    tmpstr[17] = 0;
                    hz_set_vin(tmpstr);
                    clen = 18;
                    break;

				case 0x8B:
					gbswitch =  data[1];
					clen = 2;
					break;
				case 0x8C:
					gburllen = data[1];
					clen = 2;
					break;
				case 0x8D:
                    memset(tmpstr ,0,sizeof(tmpstr) );
					memcpy(tmpstr, data+1, gburllen);
					clen = gburllen + 1;
					break;
				case 0x8E:
					port = (data[1]<<8) + data[2];
					if(gbswitch)
					{
						gb_set_addr(tmpstr, port);
					}
					else
					{
						gb_set_addr("", 0);
					}
					clen = 3;
					break;
				
				#if 0
                /* log storage */
                case 0x84:
                    clen = 3;
                    break;
                 #endif                 
                default:
                    log_e(LOG_HOZON , "Unsupport setid (%20X)",*data);
                    break;
                
            }
        }
    }

    sdata[slen] = hz_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_HOZON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_HOZON, "socket send error, reset protocol");
        hz_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_HOZON, "unack list is full, send is canceled");
    }

    return 0;
}

static void hz_ftp_cb(int evt, int par)
{
    TCOM_MSG_HEADER msg;
    hz_msg_t ftpmsg;

    ftpmsg.ftp_evt = evt;
    ftpmsg.ftp_par = par;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(hz_msg_t);
    msg.msgid    = HZ_MSG_FTP;
    tcom_send_msg(&msg, &ftpmsg);
}

#define UPG_FPATH      COM_APP_PKG_DIR"/"COM_PKG_FILE
#define UPG_FW_FPATH   "/usrdata/"COM_FW_UPDATE

static void reload_wakeup_tm(void )
{
    hz_allow_sleep = 0;
    tm_start(hz_wakeup_timer, HZ_WAKEUP_TIME , TIMER_TIMEOUT_REL_PERIOD);
    log_i(LOG_HOZON, "load wakeup timer[%d]",tm_get_time());
}

static int hz_handle_control(hz_stat_t *state, uint8_t *data, int len)
{
    int slen, res;
    uint8_t sdata[64];

    if (len < 7)
    {
        return 0;
    }

    slen = hz_pack_head(HZ_PROT_CONTRL, 0x01, sdata);
    sdata[slen++] = 0;
    sdata[slen++] = 6;
    memcpy(sdata + slen, data, 6);
    slen += 6;

    switch (data[6])
    {
        case 0x01:
            {
                char *tmp = (char *)data + 7, *url = tmp;
                char *path = NULL;

                while (*tmp != ';')
                {
                    tmp++;
                }

                *tmp = 0;
                log_i(LOG_HOZON, "get upgrade command, url=%s", url);

                if (tm_get_time() - state->ftpuptime >= 10000)
                {
                    state->ftptrans = 0;
                }

                if (strstr(url, ".pkg") || strstr(url, ".PKG"))
                {
                    path = UPG_FPATH;
                    state->upgtype = HZ_UPG_PKG;
                }
                else if (strstr(url, ".zip"))
                {
                    path = UPG_FW_FPATH;
                    state->upgtype = HZ_UPG_FW;
                }
                else
                {
                    state->upgtype = HZ_UPG_UNKNOW;
                    state->ftptrans  = 0x01;
                }

                if (state->ftptrans || ftp_request(FTP_REQ_GET, url, path, hz_ftp_cb) != 0)
                {
                    sdata[3] = 0x02;
                }
                else
                {
                    state->ftptrans  = 0x01;
                    state->ftpuptime = tm_get_time();
                }
                reload_wakeup_tm( );
            }
            break;
            #if 0

        case 0x80:
            ftpreq = FTP_REQ_PUT;
            fpath  = LOG_FPATH;
            break;

        case 0x81:
            ftpreq = FTP_REQ_PUT;
            fpath  = ERR_FPATH;
            break;

        case 0x83:
            break;
            #endif

        default:
            sdata[3] = 0x02;
            break;
    }


    sdata[slen] = hz_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_HOZON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_HOZON, "socket send error, reset protocol");
        hz_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_HOZON, "unack list is full, send is canceled");
    }

    return 0;
}


static int hz_do_checksock(hz_stat_t *state)
{
    if (!state->network || !hz_addr.port || !hz_addr.url[0] || hz_allow_sleep)
    {
        return -1;
    }
	
    if (sock_status(state->socket) == SOCK_STAT_CLOSED)
    {
        static uint64_t time = 0;

        //if (!state->suspend && !hz_data_noreport() &&
        //    (time == 0 || tm_get_time() - time > HZ_SERVR_TIMEOUT))
        if (!state->suspend && !hz_allow_sleep &&
        (time == 0 || tm_get_time() - time > HZ_SERVR_TIMEOUT))
        {
            log_i(LOG_HOZON, "start to connect with server");

            hz_ota_link_state = NM_OTA_LINK_FAULT;

            if (sock_open(NM_PUBLIC_NET, state->socket, hz_addr.url, hz_addr.port) != 0)
            {
                log_i(LOG_HOZON, "open socket failed, retry later");
            }

            time = tm_get_time();
        }
        else
        {
            hz_ota_link_state = NM_OTA_LINK_NORMAL;
        }
    }
    else if (sock_status(state->socket) == SOCK_STAT_OPENED)
    {
        hz_ota_link_state = NM_OTA_LINK_NORMAL;
        
        if (sock_error(state->socket) || sock_sync(state->socket))
        {
            log_e(LOG_HOZON, "socket error, reset protocol");
            hz_reset(state);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

static int hz_do_wait(hz_stat_t *state)
{
    if (!state->wait)
    {
        return 0;
    }

    if (tm_get_time() - state->waittime > HZ_SERVR_TIMEOUT)
    {
        if (state->wait == HZ_PROT_LOGIN)
        {
            state->wait = 0;
            log_e(LOG_HOZON, "login time out, retry later");
            hz_reset(state);
        }
        else if (++state->retry > 3)
        {
            if (state->wait == HZ_PROT_LOGOUT)
            {
                state->login = 0;
                hz_allow_sleep = 1;
            }

            log_e(LOG_HOZON, "communication time out, reset protocol");
            hz_reset(state);
        }
        else if (state->wait == HZ_PROT_LOGOUT)
        {
            uint8_t buf[256];
            int len, res;

            log_e(LOG_HOZON, "logout time out, retry [%d]", state->retry);

            len = hz_pack_logout(buf);
            res = sock_send(state->socket, buf, len, NULL);
            protocol_dump(LOG_HOZON, "FOTON", buf, len, 1);

            if (res < 0)
            {
                log_e(LOG_HOZON, "socket send error, reset protocol");
                hz_reset(state);
            }
            else if (res == 0)
            {
                log_e(LOG_HOZON, "unack list is full, send is canceled");
            }
            else
            {
                state->waittime = tm_get_time();
            }
        }
    }

    return -1;
}


static int hz_do_login(hz_stat_t *state)
{
    if (state->online)
    {
        return 0;
    }

    if (state->waittime == 0 || tm_get_time() - state->waittime > HZ_LOGIN_INTV)
    {
        uint8_t buf[256];
        int len, res;
        
        log_e(LOG_HOZON, "start to log into server");

        len = hz_pack_login(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_HOZON, "FOTON", buf, len, 1);
        
        if (res < 0)
        {
            log_e(LOG_HOZON, "socket send error, reset protocol");
            hz_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_HOZON, "unack list is full, send is canceled");
        }
        else
        {
            state->wait = HZ_PROT_LOGIN;
            state->waittime = tm_get_time();
        }
        state->hbeattime = tm_get_time();
    }

    return -1;
}

static int hz_do_suspend(hz_stat_t *state)
{
    if (state->suspend && hz_data_nosending())
    {
        log_e(LOG_HOZON, "communication is suspended");
        hz_reset(state);
        return -1;
    }

    return 0;
}
static int hz_do_hbeat(hz_stat_t *state)
{
    if(!state->online)
    {
        return 0;
    }
    //log_i(LOG_HOZON, "update heart beat time[%d]",state->hbeattime);
    if (tm_get_time() - state->hbeattime > (hz_data_get_intv() + 20)*1000 )
    {
        uint8_t buf[256];
        int len, res;
        //log_o(LOG_HOZON,"current time[%d],heart beat time[%d],intevel[%d] ",
        //                tm_get_time(),state->hbeattime,(hz_data_get_intv() + 10)*1000 );
        len = hz_pack_hbeat(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_HOZON, "FOTON", buf, len, 1);
        
        if (res < 0)
        {
            log_e(LOG_HOZON, "socket send error, reset protocol");
            hz_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_HOZON, "unack list is full, send is canceled");
        }
        else
        {
            state->hbeattime = tm_get_time();
            //log_i(LOG_HOZON, "update heart beat time[%d]",state->hbeattime);
		    return -1;
    	}

	}
	
	return 0;
}

static int hz_do_logout(hz_stat_t *state)
{
    //if (!state->can && hz_data_nosending() && (tm_get_time() - state->caltime) > 5000)
	if (!hz_is_allow_login && hz_data_nosending() && (tm_get_time() - state->caltime) > 5000)	
    {
        uint8_t buf[256];
        int len, res;

        log_i(LOG_HOZON, "start to log out from server");

        len = hz_pack_logout(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_HOZON, "FOTON", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_HOZON, "socket send error, reset protocol");
            hz_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_HOZON, "unack list is full, send is canceled");
        }
        else
        {
            state->wait = HZ_PROT_LOGOUT;
            state->waittime = tm_get_time();
        }

        return -1;
    }

    return 0;
}

static int hz_do_report(hz_stat_t *state)
{
    hz_pack_t *rpt;
    uint8_t buf[1280];

    if ((rpt = hz_data_get_pack()) != NULL &&
        (sizeof(buf) + HZ_SOCKET_RSVD) <= sock_bufsz(state->socket))
    {
        int len, res;

        log_i(LOG_HOZON, "start to send report to server");
        
        len = hz_pack_report(buf, rpt);
        res = sock_send(state->socket, buf, len, hz_data_ack_pack);
        protocol_dump(LOG_HOZON, "FOTON", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_HOZON, "socket send error, reset protocol");
            hz_data_put_back(rpt);
            hz_reset(state);
            return -1;
        }
        else if (res == 0)
        {
            log_e(LOG_HOZON, "unack list is full, send is canceled");
            hz_data_put_back(rpt);
        }
        else
        {
            hz_data_put_send(rpt);
        }
        state->hbeattime = tm_get_time();
    }

    return 0;
}

static int hz_makeup_pack(hz_stat_t *state, uint8_t *input, int len, int *uselen)
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

static int hz_do_receive(hz_stat_t *state)
{
    int ret = 0, rlen;
    uint8_t rcvbuf[1024], *input = rcvbuf;

    if ((rlen = sock_recv(state->socket, rcvbuf, sizeof(rcvbuf))) < 0)
    {
        log_e(LOG_HOZON, "socket recv error: %s", strerror(errno));
        log_e(LOG_HOZON, "socket recv error, reset protocol");
        hz_reset(state);
        return -1;
    }

    while (ret == 0 && rlen > 0)
    {
        int uselen, type, ack, dlen;
        uint8_t *data;

        if (hz_makeup_pack(state, input, rlen, &uselen) != 0)
        {
            break;
        }

        rlen  -= uselen;
        input += uselen;
        protocol_dump(LOG_HOZON, "FOTON", state->rcvbuf, state->rcvlen, 0);

        if (state->rcvbuf[state->rcvlen - 1] !=
            hz_checksum(state->rcvbuf + 2, state->rcvlen - 3))
        {
            log_e(LOG_HOZON, "packet checksum error");
            continue;
        }

        type = state->rcvbuf[2];
        ack  = state->rcvbuf[3];
        dlen = state->rcvbuf[22] * 256 + state->rcvbuf[23];
        data = state->rcvbuf + 24;

        switch (type)
        {
            case HZ_PROT_LOGIN:
                if (state->wait != HZ_PROT_LOGIN)
                {
                    log_e(LOG_HOZON, "unexpected login acknowlage");
                    break;
                }

                state->wait = 0;

                if (ack != 0x01)
                {
                    log_e(LOG_HOZON, "login is rejected");
                    break;
                }

                state->online = 1;
                state->login  = 1;
                hz_data_flush_sending();
                hz_data_flush_realtm();

                if (++hz_regseq > 65531)
                {
                    hz_regseq = 1;
                }

                {
                    uint32_t reginf = (hz_regdate << 16) + hz_regseq;
                    rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *)&reginf, sizeof(reginf));
                }

                log_o(LOG_HOZON, "login is succeed");
                reload_wakeup_tm();
                hz_is_allow_login = 1;
                break;

            case HZ_PROT_LOGOUT:
                if (state->wait != HZ_PROT_LOGOUT)
                {
                    log_e(LOG_HOZON, "unexpected logout acknowlage");
                    break;
                }

                if (ack != 0x01)
                {
                    log_e(LOG_HOZON, "logout is rejected!");
                    break;
                }

                log_o(LOG_HOZON, "logout is succeed");
                hz_reset(state);
                state->login = 0;
                hz_allow_sleep = 1;
                ret = -1;
                break;

            case HZ_PROT_CALTIME:
                if (state->wait != HZ_PROT_CALTIME)
                {
                    log_e(LOG_HOZON, "unexpected time-calibration acknowlage");
                    break;
                }

                if (ack != 0x01)
                {
                    log_e(LOG_HOZON, "time-calibration is rejected!");
                    break ;
                }

                state->wait    = 0;
                state->caltime = 0;
                log_i(LOG_HOZON, "time-calibration succeed");
                break;

            case HZ_PROT_QUREY:
                log_i(LOG_HOZON, "receive query command");
                ret = hz_handle_query(state, data, dlen);
                break;

            case HZ_PROT_SETUP:
                log_i(LOG_HOZON, "receive setup command");
                ret = hz_handle_setup(state, data, dlen);
                break;

            case HZ_PROT_CONTRL:
                log_i(LOG_HOZON, "receive control command");
                ret = hz_handle_control(state, data, dlen);
                break;

            default:
                log_e(LOG_HOZON, "receive unknown packet");
                break;
        }
    }

    return ret;
}

static int hz_do_config(hz_stat_t *state, hz_cfg_t *cfg)
{
    switch (cfg->cfgid)
    {
        case HZ_CFG_ADDR:
            if (cfg_set_para(CFG_ITEM_FOTON_URL, cfg->addr.url, sizeof(cfg->addr.url)) ||
                cfg_set_para(CFG_ITEM_FOTON_PORT, &cfg->addr.port, sizeof(cfg->addr.port)))
            {
                log_e(LOG_HOZON, "save server address failed");
                return -1;
            }

            memcpy(&hz_addr, &cfg->addr, sizeof(cfg->addr));
            hz_reset(state);
            break;

        case HZ_CFG_VIN:
            strcpy(hz_vin, cfg->vin);
            hz_reset(state);
            break;

        case HZ_CFG_REGINTV:
            if (cfg_set_para(CFG_ITEM_FOTON_REGINTV, &cfg->regintv, sizeof(cfg->regintv)))
            {
                log_e(LOG_HOZON, "save login period failed");
                return -1;
            }

            hz_regintv = cfg->regintv;
            break;

        case HZ_CFG_TIMEOUT:
            if (cfg_set_para(CFG_ITEM_FOTON_TIMEOUT, &cfg->timeout, sizeof(cfg->timeout)))
            {
                log_e(LOG_HOZON, "save communication timeout failed");
                return -1;
            }

            hz_timeout = cfg->timeout;
            break;

        case HZ_CFG_DATINTV:
            if (cfg_set_para(CFG_ITEM_FOTON_INTERVAL, &cfg->datintv, sizeof(cfg->datintv)))
            {
                log_e(LOG_HOZON, "save report period failed");
                return -1;
            }

            hz_data_set_intv(cfg->datintv);
            break;
    }

    return 0;
}

static int hz_shell_settid(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: hzsettid <xxx>\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 7)
    {
        shellprintf(" error: tid must be 7 charactores\r\n");
        return -1;
    }

    if (cfg_set_para(CFG_ITEM_FT_TID, (char *)argv[0], strlen(argv[0])))
    {
        shellprintf(" save Terminal id failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_setsim(int argc, const char **argv)
{
	char sim[13] = {0};
    if (argc != 1)
    {
        shellprintf(" usage: hzsetsim <xxx>\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 13)
    {
        shellprintf(" error: sim must be 13 charactores\r\n");
        return -1;
    }

	memcpy(sim, (char *)argv[0], strlen(argv[0]));	

	if (cfg_set_para(CFG_ITEM_FT_SIM, sim, 13))
    {
        shellprintf(" save sim id failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_setdtn(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: hzsetdtn <xxx>\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 29)
    {
        shellprintf(" error: dtn must be 29 charactores\r\n");
        return -1;
    }

    if (cfg_set_para(CFG_ITEM_FT_DTN, (char *)argv[0], strlen(argv[0])))
    {
        shellprintf(" save devices ID failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_setdevtype(int argc, const char **argv)
{
	char devtype[10] = {0};
	
    if (argc != 1)
    {
        shellprintf(" usage: hzsetdevtype <xxx>\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 10)
    {
        shellprintf(" error: dev type must be less than 10 charactores\r\n");
        return -1;
    }

	memcpy(devtype, (char *)argv[0], strlen(argv[0]));	

 	if (cfg_set_para(CFG_ITEM_FT_DEV_TYPE, devtype, 10))
    {
        shellprintf(" save devices type failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_setdevsn(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: hzsetdevsn <xxx>\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 13)
    {
        shellprintf(" error: devices number must be 13 charactores\r\n");
        return -1;
    }

    if (cfg_set_para(CFG_ITEM_FT_DEV_SN, (char *)argv[0], strlen(argv[0])))
    {
        shellprintf(" save devices number failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_settype(int argc, const char **argv)
{
    unsigned char type;
    
    if (argc != 1)
    {
        shellprintf(" usage: hzsettype <xxx>\r\n");
        return -1;
    }

    type = atoi(argv[0]);

    if (cfg_set_para(CFG_ITEM_FT_TYPE, &type, sizeof(type)))
    {
        shellprintf(" save engine type failed\r\n");
        return -2;
    }
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_setport(int argc, const char **argv)
{
    unsigned char port;
    
    if (argc != 1)
    {
        shellprintf(" usage: hzsetport <xxx>\r\n");
        return -1;
    }

    port = atoi(argv[0]);

    if (cfg_set_para(CFG_ITEM_FT_PORT, &port, sizeof(port)))
    {
        shellprintf(" save foton port failed\r\n");
        return -2;
    }
    
    shellprintf(" save ok\r\n");

    return 0;
}
static int hz_shell_set_register(int argc, const char **argv)
{
    unsigned char status;
    
    if (argc != 1)
    {
        shellprintf(" usage: hzsetreg <xxx>\r\n");
        return -1;
    }

    status = atoi(argv[0]);

    if (cfg_set_para(CFG_ITEM_FT_REGISTER, &status, sizeof(status)))
    {
        shellprintf(" save foton register failed\r\n");
        return -2;
    }
    
    shellprintf(" save ok\r\n");

    return 0;
}

static int hz_shell_setvin(int argc, const char **argv)
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

    if (hz_set_vin(argv[0]))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int hz_shell_setaddr(int argc, const char **argv)
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

    if (hz_set_addr(argv[0], port))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int hz_shell_settmout(int argc, const const char **argv)
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

    if (hz_set_timeout(timeout))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    return 0;
}

static int hz_shell_setregtm(int argc, const const char **argv)
{
    uint16_t intv;

    if (argc != 1 || sscanf(argv[0], "%hu", &intv) != 1)
    {
        shellprintf(" usage: gbsetregtm <time in seconds>\r\n");
        return -1;
    }

    if (hz_set_regintv(intv))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    return 0;
}

static int hz_shell_status(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = HZ_MSG_STATUS;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int hz_shell_suspend(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = HZ_MSG_SUSPEND;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int hz_shell_resume(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = HZ_MSG_RESUME;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int hz_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
{
    TCOM_MSG_HEADER msg;
    int network;

    if (NM_PUBLIC_NET != type)
    {
        return 0;
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

    msg.msgid    = HZ_MSG_NETWORK;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(network);

    return tcom_send_msg(&msg, &network);
}

static int hz_can_callback(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    int ret = 0;

    switch (event)
    {
        case CAN_EVENT_ACTIVE:
            {
                TCOM_MSG_HEADER msg;

                msg.sender   = MPU_MID_FOTON;
                msg.receiver = MPU_MID_FOTON;
                msg.msglen   = 0;
                msg.msgid    = HZ_MSG_CANON;
                ret = tcom_send_msg(&msg, NULL);
                break;
            }

        case CAN_EVENT_SLEEP:
        case CAN_EVENT_TIMEOUT:
            {
                TCOM_MSG_HEADER msg;

                msg.sender   = MPU_MID_FOTON;
                msg.receiver = MPU_MID_FOTON;
                msg.msglen   = 0;
                msg.msgid    = HZ_MSG_CANOFF;
                ret = tcom_send_msg(&msg, NULL);
                break;
            }

        default:
            break;
    }

    return ret;
}

static void hz_show_status(hz_stat_t *state)
{
    unsigned char version[6];
    
    shellprintf(" FOTON status\r\n");
    shellprintf("  can         : %s\r\n", state->can ? "active" : "inactive");
    shellprintf("  network     : %s\r\n", state->network ? "on" : "off");
    shellprintf("  socket      : %s\r\n", sock_strstat(sock_status(state->socket)));
    shellprintf("  server      : %s\r\n", state->online ? "connected" : "unconnected");
    shellprintf("  suspend     : %s\r\n", state->suspend ? "yes" : "no");
    shellprintf("  log out     : %s\r\n", state->login ? "no" : "yes");

    shellprintf("  server url  : %s : %u\r\n", hz_addr.url, hz_addr.port);
    shellprintf("  vin         : %s\r\n", hz_vin);


    shellprintf("  rpt period  : %u s\r\n", hz_data_get_intv());
    shellprintf("  srv timeout : %u s\r\n", hz_timeout);
    shellprintf("  reg period  : %u s\r\n", hz_regintv);
    shellprintf("  reg seq     : %u\r\n", hz_regseq);
    shellprintf("  fix iccid   : %s\r\n", hz_iccid);
    shellprintf("  batt code   : %s\r\n", hz_battcode);
    shellprintf("  sleep       : %s\r\n", hz_allow_sleep?"allow":"no allow");
    shellprintf("  logout allow: %s\r\n", hz_is_allow_login?"not allow":"allow");
    memset(version, 0, sizeof(version));
    hz_get_tbox_ver(version, sizeof(version));
    shellprintf("  tbox version: %s\r\n", version);
}

static void *hz_main(void)
{
    int tcomfd;
    hz_stat_t state;
    uint8_t onvalue; 
    unsigned int len = 1;
                
    log_o(LOG_HOZON, "FOTON thread running");

    prctl(PR_SET_NAME, "FOTON");
    reload_wakeup_tm();
    if ((tcomfd = tcom_get_read_fd(MPU_MID_FOTON)) < 0)
    {
        log_e(LOG_HOZON, "get module pipe failed, thread exit");
        return NULL;
    }

    memset(&state, 0, sizeof(hz_stat_t));

    if ((state.socket = sock_create("FOTON", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_HOZON, "create socket failed, thread exit");
        return NULL;
    }

    while (1)
    {
        TCOM_MSG_HEADER msg;
        hz_msg_t msgdata;
        int res,hz_ftp_evt;
        hz_ftp_evt = 0;
        memset(&msgdata,0,sizeof(hz_msg_t));
        res = protocol_wait_msg(MPU_MID_FOTON, tcomfd, &msg, &msgdata, 200);

        if (res < 0)
        {
            log_e(LOG_HOZON, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            case HZ_MSG_FTP:
                if (msgdata.ftp_evt == FTP_PROCESS)
                {
                    hz_ftp_evt = FTP_PROCESS;
                    log_i(LOG_HOZON, "get FTP message: PROCESS %d%%", msgdata.ftp_par);
                }
                else if (msgdata.ftp_evt == FTP_ERROR)
                {
                    reload_wakeup_tm();
                    hz_ftp_evt = FTP_ERROR;
                    log_i(LOG_HOZON, "get FTP message: ERROR %s", ftp_errstr(msgdata.ftp_par));
                }
                else if (msgdata.ftp_evt == FTP_FINISH)
                {
                    log_o(LOG_HOZON, "get FTP message: FINISH %d", msgdata.ftp_par);
                    hz_ftp_evt = FTP_FINISH;
                    if (msgdata.ftp_par == 0)
                    {
                        if (1 == state.upgtype)
                        {
                            log_e(LOG_HOZON, "try to upgrade: %s", shell_cmd_exec("pkgupgrade", NULL, 0) ? "ERR" : "OK");
                        }
                        else if (2 == state.upgtype)
                        {
                            log_e(LOG_HOZON, "try to upgrade firmware: %s", upg_fw_start(UPG_FW_FPATH) ? "ERR" : "OK");
                        }
                    }
                }
                else
                {
                    log_i(LOG_HOZON, "get FTP message: unknown event");
                }

                break;

            case HZ_MSG_NETWORK:
                log_i(LOG_HOZON, "get NETWORK message: %d", msgdata.network);

                if (state.network != msgdata.network)
                {
                    hz_reset(&state);
                    state.network = msgdata.network;
                }

                break;

            case PM_MSG_EMERGENCY:
                log_i(LOG_HOZON, "get EMERGENCY message");
                hz_data_emergence(1);
                break;

            case PM_MSG_OFF:
                log_i(LOG_HOZON, "get POWER OFF message");
                hz_data_emergence(0);
                hz_cache_save_all();
                break;

            case PM_MSG_RUNNING:
                log_i(LOG_HOZON, "get RUNNING message");
                hz_data_emergence(0);
                reload_wakeup_tm();
                break;

            case HZ_MSG_CANON:
                log_i(LOG_HOZON, "get CANON message");
                state.can = 1;
                hz_allow_sleep = 0; 
                hz_is_allow_login  = 1;
                reload_wakeup_tm();
                break;

            case HZ_MSG_CANOFF:
                log_i(LOG_HOZON, "get CANOFF message");
                state.can = 0;
                state.caltime = tm_get_time();
                //hz_allow_sleep = !state.online;
                //hz_is_allow_login  = 0;
                //tm_stop(hz_wakeup_timer);
                break;

            case HZ_MSG_SUSPEND:
                log_i(LOG_HOZON, "get SUSPEND message");
                state.suspend = 1;
                hz_data_set_pendflag(1);
                break;

            case HZ_MSG_RESUME:
                state.suspend = 0;
                log_i(LOG_HOZON, "get RESUME message");
                hz_data_set_pendflag(0);
                break;

            case HZ_MSG_STATUS:
                log_i(LOG_HOZON, "get STATUS message");
                hz_show_status(&state);
                break;

            case HZ_MSG_ERRON:
                log_i(LOG_HOZON, "get ERRON message");
                state.errtrig = 1;
                break;

            case HZ_MSG_ERROFF:
                log_i(LOG_HOZON, "get ERROFF message");
                state.errtrig = 0;
                break;

            case HZ_MSG_CONFIG:
                log_i(LOG_HOZON, "get CONFIG message");
                hz_do_config(&state, &msgdata.cfg);
                break;

            case MPU_MID_MID_PWDG:
                pwdg_feed(MPU_MID_FOTON);
                break;
            case AT_MSG_ID_RING:
                log_i(LOG_HOZON, "ring event wakeup");
                reload_wakeup_tm();
                break;
            case HZ_MSG_WAKEUP_TIMEOUT:
                /*can off and update finish , allow sleep */
                st_get(ST_ITEM_KL15_SIG, &onvalue, &len);
                if((hz_ftp_evt == FTP_FINISH || hz_ftp_evt == FTP_ERROR )
                    && 0 == onvalue && 0 == state.can )
                {
                    log_i(LOG_HOZON, "wakeup timer timeout[%d]",tm_get_time());
                    log_i(LOG_HOZON, "can off ,no update task,on line off");
                    if(state.online) 
                    {
                        log_o(LOG_HOZON,"allow terminal logout");
                        hz_is_allow_login  = 0;
                    }
                    else 
                    {
                        log_o(LOG_HOZON,"allow terminal sleep");
                        hz_allow_sleep = 1;
                    }
                    tm_stop(hz_wakeup_timer);
                }
                else
                {
                    /*don't allow sleep */
                    reload_wakeup_tm();
                    log_i(LOG_HOZON, "FTP event %d(0 ,finish;1,error;2,process)",
                                        hz_ftp_evt); 
                    log_i(LOG_HOZON, "on state %s,can %s",onvalue?"on":"off",state.can?"on":"off");
                }
                break;
        }
        //log_i(LOG_HOZON, "current heart beat time[%d]",state.hbeattime);
        res = hz_do_checksock(&state) ||
              hz_do_receive(&state) ||
              hz_do_wait(&state) ||
              hz_do_login(&state) ||
              hz_do_suspend(&state) ||
              hz_do_report(&state) ||
              hz_do_hbeat(&state)||
              hz_do_logout(&state);

    }

    sock_delete(state.socket);
    return NULL;
}

static int hz_allow_sleep_handler(PM_EVT_ID id)
{
    return hz_allow_sleep;
}

static int hz_get_ota_status(void)
{
    log_i(LOG_HOZON, "foton ota link status:%u", hz_ota_link_state);
    return hz_ota_link_state;
}

int hz_vin_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                          unsigned char *new_para, unsigned int len)
{
    TCOM_MSG_HEADER msg;
    hz_cfg_t cfg;

    if(CFG_ITEM_FOTON_VIN != id)
    {
        log_e(LOG_HOZON, "error id:%d ",id);
        return 0;
    }
    
    /* not changed */
    if (0 == strcmp((char *)old_para , (char *)new_para))
    {
        log_o(LOG_NM, "FOTON VIN is not changed!");
        return 0;
    }
    
    msg.msgid    = HZ_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.vin) + sizeof(cfg.cfgid);

    cfg.cfgid = HZ_CFG_VIN;
    strncpy(cfg.vin, (char *)new_para, len);
    cfg.vin[17] = 0;
    
    return tcom_send_msg(&msg, &cfg);

}

int hz_init(INIT_PHASE phase)
{
    int ret = 0;
    uint32_t reginf = 0, cfglen;
    char ver[COM_APP_VER_LEN];
    ret |= hz_data_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(hz_vin, 0, sizeof(hz_vin));
            memset(hz_iccid, 0, sizeof(hz_iccid));
            memset(hz_battcode, 0, sizeof(hz_battcode));
            hz_addr.url[0] = 0;
            hz_addr.port   = 0;
            hz_timeout   = 5;
            hz_regdate   = 0;
            hz_regseq    = 0;
            hz_allow_sleep     = 1;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            cfglen = sizeof(reginf);
            ret = rds_get(RDS_FOTON_REGSEQ, (unsigned char *) &reginf, &cfglen, ver);

            if (ret == 0)
            {
                hz_regdate = reginf >> 16;
                hz_regseq  = reginf;
            }
            else
            {
                reginf = 0;
                ret = rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *) &reginf, sizeof(reginf));
            }

            cfglen = sizeof(hz_addr.url);
            ret |= cfg_get_para(CFG_ITEM_FOTON_URL, hz_addr.url, &cfglen);
            cfglen = sizeof(hz_addr.port);
            ret |= cfg_get_para(CFG_ITEM_FOTON_PORT, &hz_addr.port, &cfglen);
            cfglen = sizeof(hz_vin);
            ret |= cfg_get_para(CFG_ITEM_FOTON_VIN, hz_vin, &cfglen);
            cfglen = sizeof(hz_regintv);
            ret |= cfg_get_para(CFG_ITEM_FOTON_REGINTV, &hz_regintv, &cfglen);
            cfglen = sizeof(hz_timeout);
            ret |= cfg_get_para(CFG_ITEM_FOTON_TIMEOUT, &hz_timeout, &cfglen);

            ret |= pm_reg_handler(MPU_MID_FOTON, hz_allow_sleep_handler);
            ret |= shell_cmd_register("hzstat", hz_shell_status, "show FOTON status");
            ret |= shell_cmd_register("hzsetvin", hz_shell_setvin, "set FOTON vin");
            ret |= shell_cmd_register("hzsetaddr", hz_shell_setaddr, "set FOTON server address");
            ret |= shell_cmd_register("hzsettmout", hz_shell_settmout, "set FOTON server timeout");
            ret |= shell_cmd_register("hzsetregtm", hz_shell_setregtm, "set FOTON register period");
            ret |= shell_cmd_register("hzsuspend", hz_shell_suspend, "stop FOTON");
            ret |= shell_cmd_register("hzresume", hz_shell_resume, "open FOTON");
//			ret |= shell_cmd_register("hzsethbintv", hz_shell_set_heartbeat, "set heartbeat period ");

			/*comm_code(7)*/
            ret |= shell_cmd_register("hzsettid", hz_shell_settid, "set Terminal ID");

			/*std_simcode*/
            ret |= shell_cmd_register("hzsetsim", hz_shell_setsim, "set sim phone num");

			/*trace_code*/
            ret |= shell_cmd_register("hzsetdtn", hz_shell_setdtn, "set Ternninal traceablity number ");

			/*terminal_kind; default ZKC02B; current YLA02*/
			/*ECU Model number*/
            ret |= shell_cmd_register("hzsetdevtype", hz_shell_setdevtype, "set devices type");

			/*设备编号*/
            ret |= shell_cmd_register("hzsetdevsn", hz_shell_setdevsn, "set devices number");

			/*发动机类型*/
            ret |= shell_cmd_register("hzsettype", hz_shell_settype, "set engine type");

			/*平台端口号序号*/
            ret |= shell_cmd_register("hzsetport", hz_shell_setport, "set foton port");		
			
            ret |= shell_cmd_register("hzsetreg", hz_shell_set_register, "set foton register status");
            ret |= cfg_register(CFG_ITEM_FOTON_VIN, hz_vin_changed);
            ret |= can_register_callback(hz_can_callback);
            ret |= nm_reg_status_changed(NM_PUBLIC_NET, hz_nm_callback);
            ret |= nm_register_get_ota_status(hz_get_ota_status);
            ret |= tm_create(TIMER_REL, HZ_MSG_WAKEUP_TIMEOUT, MPU_MID_FOTON, &hz_wakeup_timer);
            break;
    }

    return ret;
}

int hz_run(void)
{
    int ret = 0;
    pthread_t tid;

    ret = pthread_create(&tid, NULL, (void *)hz_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_HOZON, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
}

int hz_set_addr(const char *url, uint16_t port)
{
    TCOM_MSG_HEADER msg;
    hz_cfg_t cfg;

    if (strlen(url) > 255)
    {
        log_e(LOG_HOZON, "url length must be less than 255: %s", url);
        return -1;
    }

    msg.msgid    = HZ_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.addr) + sizeof(cfg.cfgid);

    cfg.cfgid = HZ_CFG_ADDR;
    strcpy(cfg.addr.url, url);
    cfg.addr.port = port;

    return tcom_send_msg(&msg, &cfg);
}

int hz_set_vin(const char *vin)
{
    char hzvin[18];
    
    if (strlen(vin) != 17)
    {
        log_e(LOG_HOZON, "vin number must be 17 charactor: %s", vin);
        return -1;
    }
    
    memcpy(hzvin,vin,17);
    hzvin[17] = 0;
    if (cfg_set_para(CFG_ITEM_FOTON_VIN, hzvin, sizeof(hzvin)))
    {
        log_e(LOG_HOZON, "save vin failed");
        return -1;
    }
    
    return 0;
}

int hz_set_datintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    hz_cfg_t cfg;

    msg.msgid    = HZ_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.datintv) + sizeof(cfg.cfgid);

    cfg.cfgid = HZ_CFG_DATINTV;
    cfg.datintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int hz_set_regintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    hz_cfg_t cfg;

    msg.msgid    = HZ_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.regintv) + sizeof(cfg.cfgid);

    cfg.cfgid = HZ_CFG_REGINTV;
    cfg.regintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int hz_set_timeout(uint16_t timeout)
{
    TCOM_MSG_HEADER msg;
    hz_cfg_t cfg;

    msg.msgid    = HZ_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.timeout) + sizeof(cfg.cfgid);

    cfg.cfgid = HZ_CFG_TIMEOUT;
    cfg.timeout = timeout;

    return tcom_send_msg(&msg, &cfg);
}

