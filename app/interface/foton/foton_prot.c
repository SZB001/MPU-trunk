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
#include "foton.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "foton_api.h"
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

#define PROT_LOGIN      0x01
#define PROT_REPORT     0x02
#define PROT_REISSUE    0x03
#define PROT_LOGOUT     0x04
#define PROT_HBEAT      0x07
#define PROT_CALTIME    0x08
#define PROT_QUREY      0x80
#define PROT_SETUP      0x81
#define PROT_CONTRL     0x82

#define FT_SERVR_TIMEOUT    (1000 * ft_timeout)
#define FT_LOGIN_INTV       (1000 * ft_regintv)
#define FT_SOCKET_RSVD      1024

#define FT_WAKEUP_TIME      3*60*1000  /*3 mint*/

typedef enum
{
    FT_CFG_URL = 1,
    FT_CFG_PORT,
    FT_CFG_ADDR,
    FT_CFG_VIN,
    FT_CFG_DATINTV,
    FT_CFG_REGINTV,
    FT_CFG_TIMEOUT,
} FT_CFG_TYPE;


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
} ft_stat_t;

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
} ft_cfg_t;

typedef union
{
    /* for FT_MSG_NETWORK */
    int network;
    /* for FT_MSG_SOCKET */
    int socket;
    /* for FT_MSG_CONFIG */
    ft_cfg_t cfg;
    struct
    {
        int ftp_evt;
        int ftp_par;
    };
} ft_msg_t;

static char       ft_vin[18];
static svr_addr_t ft_addr;
static uint16_t   ft_timeout;
static uint16_t   ft_regintv;
static uint16_t   ft_regdate;
static uint16_t   ft_regseq;
static char       ft_iccid[21];
static char       ft_battcode[64];
static int        ft_ota_link_state = NM_OTA_LINK_NORMAL;
timer_t ft_wakeup_timer; 

int 	   ft_allow_sleep;

extern int ft_is_allow_login;


/****************************************************************
 function:     ft_get_tbox_ver
 description:  get all version
 input:        unsigned int size
 output:       unsigned char *ver
 return:       0 indicates upgrade success;
               others indicates upgrade failed
 *****************************************************************/
int ft_get_tbox_ver(unsigned char *ver, unsigned int size)
{
    int ret;
    char version[TBOX_FW_VER_LEN];
    unsigned int fw, mpu_app, mcu_app;

    memset(version,0,sizeof(version));
    ret = upg_get_fw_ex_ver(version, TBOX_FW_VER_LEN);

    /* ex ver: "+FTCOMMxxx" */
    if (ret != 10 || 1 != sscanf(version, "+%*6s%3x", &fw))
    {
        fw = 0;
        log_e(LOG_DEV, "externd version error,ret:%d,ver:%s", ret, version);
    }

    /* mpu app ver: "MPU_J303FTCOMMxxx[A_xx]" */
    memset(version,0,sizeof(version));
    upg_get_app_ver((unsigned char *)version,COM_APP_VER_LEN);
    if (1 != sscanf(version, "MPU_J303FTCOMM%d[%*s]", &mpu_app))
    {
        mpu_app = 0;
        log_e(LOG_DEV, "MPU app version error,ver:%s", version);
    }

    /* mcu app ver: "MCU_J303FTCOMMxxx[A_xx]" */
    memset(version,0,sizeof(version));
    upg_get_mcu_run_ver((unsigned char *)version,APP_VER_LEN);
    if (1 != sscanf(version, "MCU_J303FTCOMM%d[%*s]", &mcu_app))
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

static void ft_reset(ft_stat_t *state)
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

static int ft_checksum(uint8_t *data, int len)
{
    uint8_t cs = 0;

    while (len--)
    {
        cs ^= *data++;
    }

    return cs;
}

static int ft_pack_head(int type, int dir, uint8_t *buf)
{
    int len = 0;

    buf[len++] = '#';
    buf[len++] = '#';
    buf[len++] = type;
    buf[len++] = dir;
    /* vin number */
    memcpy(buf + len, ft_vin, 17);
    len += 17;
    /* encryption mode : */
    /* 0x01 : NONE */
    /* 0x02 : RSA */
    /* 0x03 : AES128 */
    buf[len++] = 0x01;

    return len;
}

static int ft_pack_report(uint8_t *buf, ft_pack_t *rpt)
{
    int len;

    len = ft_pack_head(rpt->type, 0xfe, buf);
    /* data length */
    buf[len++] = rpt->len >> 8;
    buf[len++] = rpt->len;
    /* report data */
    memcpy(buf + len, rpt->data, rpt->len);
    len += rpt->len;
    /* check sum */
    buf[len] = ft_checksum(buf + 2, len - 2);

    return len + 1;
}

static int ft_pack_login(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp, plen, pos;


    tm_get_abstime(&time);

    if (ft_regdate != time.mon * 100 + time.mday)
    {
        uint32_t reginf;

        ft_regdate  = time.mon * 100 + time.mday;
        ft_regseq   = 1;
        reginf      = (ft_regdate << 16) + ft_regseq;
        rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *)&reginf, sizeof(reginf));
    }

    /* header */
    pos = ft_pack_head(PROT_LOGIN, 0xfe, buf);
    /* jump length */
    len = pos + 2;
    /* time */
    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
    buf[len++] = ft_regseq >> 8;
    buf[len++] = ft_regseq;

    /* iccid */
    at_get_iccid(ft_iccid);
    memcpy(buf + len, ft_iccid, 20);
    len += 20;
    /* support only one battery  */
    buf[len++] = 1;
    /* battery code */
    tmp = strlen(ft_battcode);
    buf[len++] = tmp;
    memcpy(buf + len, ft_battcode, tmp);
    len += tmp;
    /* adjust length */
    plen = len - pos - 2;
    buf[pos++] = plen >> 8;
    buf[pos++] = plen;
    /* check sum */
    buf[len] = ft_checksum(buf + 2, len - 2);

    return len + 1;
}
static int ft_pack_hbeat(uint8_t *buf)
{
    int len = 0, plen, pos;
    
    /* header */
    pos = ft_pack_head(PROT_HBEAT, 0xFE, buf);
    /* jump length */
    len = pos + 2;

    /* adjust length */
    plen = len - pos - 2;
    buf[pos++] = plen >> 8;
    buf[pos++] = plen;
    /* check sum */
    buf[len] = ft_checksum(buf + 2, len - 2);

    return len + 1;
}

static int ft_pack_logout(uint8_t *buf)
{
    RTCTIME time;
    int len = 0, tmp;

    tmp = ft_regseq == 1 ? 65531 : ft_regseq - 1;
    tm_get_abstime(&time);

    len = ft_pack_head(PROT_LOGOUT, 0xfe, buf);
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
    buf[len]   = ft_checksum(buf + 2, len - 2);

    return len + 1;
}

static int ft_handle_query(ft_stat_t *state, uint8_t *data, int len)
{
    int slen, ulen, res;
    uint8_t sdata[1024], *plen;

    if (len < 7)
    {
        return 0;
    }

    slen = ft_pack_head(PROT_QUREY, 0x01, sdata);
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
                    tmpint = ft_data_get_intv();
                    sdata[slen++] = tmpint >> 8;
                    sdata[slen++] = tmpint;
                    break;

                /* server address length */
                case 0x04:
                    sdata[slen++] = strlen(ft_addr.url);
                    break;

                /* server address */
                case 0x05:
                    tmpint = strlen(ft_addr.url);
                    memcpy(sdata + slen, ft_addr.url, tmpint);
                    slen += tmpint;
                    break;

                /* server port */
                case 0x06:
                    sdata[slen++] = ft_addr.port >> 8;
                    sdata[slen++] = ft_addr.port;
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
                        ft_get_tbox_ver(ver, sizeof(ver));
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
                    sdata[slen++] = ft_timeout >> 8;
                    sdata[slen++] = ft_timeout;
                    break;

                /* relogin period */
                case 0x0c:
                    sdata[slen++] = ft_regintv;
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
                    memcpy(sdata + slen, ft_vin, 17);
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

    sdata[slen] = ft_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_FOTON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_FOTON, "socket send error, reset protocol");
        ft_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_FOTON, "unack list is full, send is canceled");
    }

    return 0;
}



static int ft_check_setup(uint8_t *data, int len)
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

static int ft_handle_setup(ft_stat_t *state, uint8_t *data, int len)
{
    int slen, res;
    uint8_t sdata[64];

    if (len < 7)
    {
        return 0;
    }

    slen = ft_pack_head(PROT_SETUP, 0x01, sdata);
    sdata[slen++] = 0;
    sdata[slen++] = 6;
    memcpy(sdata + slen, data, 6);
    data += 6;
    slen += 6;

    if (ft_check_setup(data, len - 6))
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
                    ft_set_datintv((uint16_t)data[1] * 0xff + data[2]);
                    clen = 3;
                    break;

                /* server address length */
                case 0x04:
                    urllen = data[1];
                    log_i(LOG_FOTON,"server address length:%d",urllen);
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
                    log_i(LOG_FOTON,"server port:%d",port);
                    ft_set_addr(tmpstr, port);
                    clen = 3;
                    break;

                /* heart beat period */
                case 0x09:
                    //ft_set_hbtintv(data[1]);
                    clen = 2;
                    break;

                /* server timeout */
                case 0x0b:
                    ft_set_timeout((uint16_t)data[1] * 0xff + data[2]);
                    clen = 3;
                    break;

                /* relogin period */
                case 0x0c:
                    ft_set_regintv(data[1]);
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
                    ft_set_vin(tmpstr);
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
                    log_e(LOG_FOTON , "Unsupport setid (%20X)",*data);
                    break;
                
            }
        }
    }

    sdata[slen] = ft_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_FOTON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_FOTON, "socket send error, reset protocol");
        ft_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_FOTON, "unack list is full, send is canceled");
    }

    return 0;
}

static void ft_ftp_cb(int evt, int par)
{
    TCOM_MSG_HEADER msg;
    ft_msg_t ftpmsg;

    ftpmsg.ftp_evt = evt;
    ftpmsg.ftp_par = par;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(ft_msg_t);
    msg.msgid    = FT_MSG_FTP;
    tcom_send_msg(&msg, &ftpmsg);
}

#define UPG_FPATH      COM_APP_PKG_DIR"/"COM_PKG_FILE
#define UPG_FW_FPATH   "/usrdata/"COM_FW_UPDATE

static void reload_wakeup_tm(void )
{
    ft_allow_sleep = 0;
    tm_start(ft_wakeup_timer, FT_WAKEUP_TIME , TIMER_TIMEOUT_REL_PERIOD);
    log_i(LOG_FOTON, "load wakeup timer[%d]",tm_get_time());
}

static int ft_handle_control(ft_stat_t *state, uint8_t *data, int len)
{
    int slen, res;
    uint8_t sdata[64];

    if (len < 7)
    {
        return 0;
    }

    slen = ft_pack_head(PROT_CONTRL, 0x01, sdata);
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
                log_i(LOG_FOTON, "get upgrade command, url=%s", url);

                if (tm_get_time() - state->ftpuptime >= 10000)
                {
                    state->ftptrans = 0;
                }

                if (strstr(url, ".pkg") || strstr(url, ".PKG"))
                {
                    path = UPG_FPATH;
                    state->upgtype = FT_UPG_PKG;
                }
                else if (strstr(url, ".zip"))
                {
                    path = UPG_FW_FPATH;
                    state->upgtype = FT_UPG_FW;
                }
                else
                {
                    state->upgtype = FT_UPG_UNKNOW;
                    state->ftptrans  = 0x01;
                }

                if (state->ftptrans || ftp_request(FTP_REQ_GET, url, path, ft_ftp_cb) != 0)
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


    sdata[slen] = ft_checksum(sdata + 2, slen - 2);
    slen++;
    res = sock_send(state->socket, sdata, slen, NULL);
    protocol_dump(LOG_FOTON, "FOTON", sdata, slen, 1);

    if (res < 0)
    {
        log_e(LOG_FOTON, "socket send error, reset protocol");
        ft_reset(state);
        return -1;
    }

    if (res == 0)
    {
        log_e(LOG_FOTON, "unack list is full, send is canceled");
    }

    return 0;
}


static int ft_do_checksock(ft_stat_t *state)
{
    if (!state->network || !ft_addr.port || !ft_addr.url[0] || ft_allow_sleep)
    {
        return -1;
    }
	
    if (sock_status(state->socket) == SOCK_STAT_CLOSED)
    {
        static uint64_t time = 0;

        //if (!state->suspend && !ft_data_noreport() &&
        //    (time == 0 || tm_get_time() - time > FT_SERVR_TIMEOUT))
        if (!state->suspend && !ft_allow_sleep &&
        (time == 0 || tm_get_time() - time > FT_SERVR_TIMEOUT))
        {
            log_i(LOG_FOTON, "start to connect with server");

            ft_ota_link_state = NM_OTA_LINK_FAULT;

            if (sock_open(NM_PUBLIC_NET, state->socket, ft_addr.url, ft_addr.port) != 0)
            {
                log_i(LOG_FOTON, "open socket failed, retry later");
            }

            time = tm_get_time();
        }
        else
        {
            ft_ota_link_state = NM_OTA_LINK_NORMAL;
        }
    }
    else if (sock_status(state->socket) == SOCK_STAT_OPENED)
    {
        ft_ota_link_state = NM_OTA_LINK_NORMAL;
        
        if (sock_error(state->socket) || sock_sync(state->socket))
        {
            log_e(LOG_FOTON, "socket error, reset protocol");
            ft_reset(state);
        }
        else
        {
            return 0;
        }
    }

    return -1;
}

static int ft_do_wait(ft_stat_t *state)
{
    if (!state->wait)
    {
        return 0;
    }

    if (tm_get_time() - state->waittime > FT_SERVR_TIMEOUT)
    {
        if (state->wait == PROT_LOGIN)
        {
            state->wait = 0;
            log_e(LOG_FOTON, "login time out, retry later");
            ft_reset(state);
        }
        else if (++state->retry > 3)
        {
            if (state->wait == PROT_LOGOUT)
            {
                state->login = 0;
                ft_allow_sleep = 1;
            }

            log_e(LOG_FOTON, "communication time out, reset protocol");
            ft_reset(state);
        }
        else if (state->wait == PROT_LOGOUT)
        {
            uint8_t buf[256];
            int len, res;

            log_e(LOG_FOTON, "logout time out, retry [%d]", state->retry);

            len = ft_pack_logout(buf);
            res = sock_send(state->socket, buf, len, NULL);
            protocol_dump(LOG_FOTON, "FOTON", buf, len, 1);

            if (res < 0)
            {
                log_e(LOG_FOTON, "socket send error, reset protocol");
                ft_reset(state);
            }
            else if (res == 0)
            {
                log_e(LOG_FOTON, "unack list is full, send is canceled");
            }
            else
            {
                state->waittime = tm_get_time();
            }
        }
    }

    return -1;
}


static int ft_do_login(ft_stat_t *state)
{
    if (state->online)
    {
        return 0;
    }

    if (state->waittime == 0 || tm_get_time() - state->waittime > FT_LOGIN_INTV)
    {
        uint8_t buf[256];
        int len, res;
        
        log_e(LOG_FOTON, "start to log into server");

        len = ft_pack_login(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_FOTON, "FOTON", buf, len, 1);
        
        if (res < 0)
        {
            log_e(LOG_FOTON, "socket send error, reset protocol");
            ft_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_FOTON, "unack list is full, send is canceled");
        }
        else
        {
            state->wait = PROT_LOGIN;
            state->waittime = tm_get_time();
        }
        state->hbeattime = tm_get_time();
    }

    return -1;
}

static int ft_do_suspend(ft_stat_t *state)
{
    if (state->suspend && ft_data_nosending())
    {
        log_e(LOG_FOTON, "communication is suspended");
        ft_reset(state);
        return -1;
    }

    return 0;
}
static int ft_do_hbeat(ft_stat_t *state)
{
    if(!state->online)
    {
        return 0;
    }
    //log_i(LOG_FOTON, "update heart beat time[%d]",state->hbeattime);
    if (tm_get_time() - state->hbeattime > (ft_data_get_intv() + 20)*1000 )
    {
        uint8_t buf[256];
        int len, res;
        //log_o(LOG_FOTON,"current time[%d],heart beat time[%d],intevel[%d] ",
        //                tm_get_time(),state->hbeattime,(ft_data_get_intv() + 10)*1000 );
        len = ft_pack_hbeat(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_FOTON, "FOTON", buf, len, 1);
        
        if (res < 0)
        {
            log_e(LOG_FOTON, "socket send error, reset protocol");
            ft_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_FOTON, "unack list is full, send is canceled");
        }
        else
        {
            state->hbeattime = tm_get_time();
            //log_i(LOG_FOTON, "update heart beat time[%d]",state->hbeattime);
		    return -1;
    	}

	}
	
	return 0;
}

static int ft_do_logout(ft_stat_t *state)
{
    //if (!state->can && ft_data_nosending() && (tm_get_time() - state->caltime) > 5000)
	if (!ft_is_allow_login && ft_data_nosending() && (tm_get_time() - state->caltime) > 5000)	
    {
        uint8_t buf[256];
        int len, res;

        log_i(LOG_FOTON, "start to log out from server");

        len = ft_pack_logout(buf);
        res = sock_send(state->socket, buf, len, NULL);
        protocol_dump(LOG_FOTON, "FOTON", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_FOTON, "socket send error, reset protocol");
            ft_reset(state);
        }
        else if (res == 0)
        {
            log_e(LOG_FOTON, "unack list is full, send is canceled");
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

static int ft_do_report(ft_stat_t *state)
{
    ft_pack_t *rpt;
    uint8_t buf[1280];

    if ((rpt = ft_data_get_pack()) != NULL &&
        (sizeof(buf) + FT_SOCKET_RSVD) <= sock_bufsz(state->socket))
    {
        int len, res;

        log_i(LOG_FOTON, "start to send report to server");
        
        len = ft_pack_report(buf, rpt);
        res = sock_send(state->socket, buf, len, ft_data_ack_pack);
        protocol_dump(LOG_FOTON, "FOTON", buf, len, 1);

        if (res < 0)
        {
            log_e(LOG_FOTON, "socket send error, reset protocol");
            ft_data_put_back(rpt);
            ft_reset(state);
            return -1;
        }
        else if (res == 0)
        {
            log_e(LOG_FOTON, "unack list is full, send is canceled");
            ft_data_put_back(rpt);
        }
        else
        {
            ft_data_put_send(rpt);
        }
        state->hbeattime = tm_get_time();
    }

    return 0;
}

static int ft_makeup_pack(ft_stat_t *state, uint8_t *input, int len, int *uselen)
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

static int ft_do_receive(ft_stat_t *state)
{
    int ret = 0, rlen;
    uint8_t rcvbuf[1024], *input = rcvbuf;

    if ((rlen = sock_recv(state->socket, rcvbuf, sizeof(rcvbuf))) < 0)
    {
        log_e(LOG_FOTON, "socket recv error: %s", strerror(errno));
        log_e(LOG_FOTON, "socket recv error, reset protocol");
        ft_reset(state);
        return -1;
    }

    while (ret == 0 && rlen > 0)
    {
        int uselen, type, ack, dlen;
        uint8_t *data;

        if (ft_makeup_pack(state, input, rlen, &uselen) != 0)
        {
            break;
        }

        rlen  -= uselen;
        input += uselen;
        protocol_dump(LOG_FOTON, "FOTON", state->rcvbuf, state->rcvlen, 0);

        if (state->rcvbuf[state->rcvlen - 1] !=
            ft_checksum(state->rcvbuf + 2, state->rcvlen - 3))
        {
            log_e(LOG_FOTON, "packet checksum error");
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
                    log_e(LOG_FOTON, "unexpected login acknowlage");
                    break;
                }

                state->wait = 0;

                if (ack != 0x01)
                {
                    log_e(LOG_FOTON, "login is rejected");
                    break;
                }

                state->online = 1;
                state->login  = 1;
                ft_data_flush_sending();
                ft_data_flush_realtm();

                if (++ft_regseq > 65531)
                {
                    ft_regseq = 1;
                }

                {
                    uint32_t reginf = (ft_regdate << 16) + ft_regseq;
                    rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *)&reginf, sizeof(reginf));
                }

                log_o(LOG_FOTON, "login is succeed");
                reload_wakeup_tm();
                ft_is_allow_login = 1;
                break;

            case PROT_LOGOUT:
                if (state->wait != PROT_LOGOUT)
                {
                    log_e(LOG_FOTON, "unexpected logout acknowlage");
                    break;
                }

                if (ack != 0x01)
                {
                    log_e(LOG_FOTON, "logout is rejected!");
                    break;
                }

                log_o(LOG_FOTON, "logout is succeed");
                ft_reset(state);
                state->login = 0;
                ft_allow_sleep = 1;
                ret = -1;
                break;

            case PROT_CALTIME:
                if (state->wait != PROT_CALTIME)
                {
                    log_e(LOG_FOTON, "unexpected time-calibration acknowlage");
                    break;
                }

                if (ack != 0x01)
                {
                    log_e(LOG_FOTON, "time-calibration is rejected!");
                    break ;
                }

                state->wait    = 0;
                state->caltime = 0;
                log_i(LOG_FOTON, "time-calibration succeed");
                break;

            case PROT_QUREY:
                log_i(LOG_FOTON, "receive query command");
                ret = ft_handle_query(state, data, dlen);
                break;

            case PROT_SETUP:
                log_i(LOG_FOTON, "receive setup command");
                ret = ft_handle_setup(state, data, dlen);
                break;

            case PROT_CONTRL:
                log_i(LOG_FOTON, "receive control command");
                ret = ft_handle_control(state, data, dlen);
                break;

            default:
                log_e(LOG_FOTON, "receive unknown packet");
                break;
        }
    }

    return ret;
}

static int ft_do_config(ft_stat_t *state, ft_cfg_t *cfg)
{
    switch (cfg->cfgid)
    {
        case FT_CFG_ADDR:
            if (cfg_set_para(CFG_ITEM_FOTON_URL, cfg->addr.url, sizeof(cfg->addr.url)) ||
                cfg_set_para(CFG_ITEM_FOTON_PORT, &cfg->addr.port, sizeof(cfg->addr.port)))
            {
                log_e(LOG_FOTON, "save server address failed");
                return -1;
            }

            memcpy(&ft_addr, &cfg->addr, sizeof(cfg->addr));
            ft_reset(state);
            break;

        case FT_CFG_VIN:
            strcpy(ft_vin, cfg->vin);
            ft_reset(state);
            break;

        case FT_CFG_REGINTV:
            if (cfg_set_para(CFG_ITEM_FOTON_REGINTV, &cfg->regintv, sizeof(cfg->regintv)))
            {
                log_e(LOG_FOTON, "save login period failed");
                return -1;
            }

            ft_regintv = cfg->regintv;
            break;

        case FT_CFG_TIMEOUT:
            if (cfg_set_para(CFG_ITEM_FOTON_TIMEOUT, &cfg->timeout, sizeof(cfg->timeout)))
            {
                log_e(LOG_FOTON, "save communication timeout failed");
                return -1;
            }

            ft_timeout = cfg->timeout;
            break;

        case FT_CFG_DATINTV:
            if (cfg_set_para(CFG_ITEM_FOTON_INTERVAL, &cfg->datintv, sizeof(cfg->datintv)))
            {
                log_e(LOG_FOTON, "save report period failed");
                return -1;
            }

            ft_data_set_intv(cfg->datintv);
            break;
    }

    return 0;
}

static int ft_shell_settid(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: ftsettid <xxx>\r\n");
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
static int ft_shell_setsim(int argc, const char **argv)
{
	char sim[13] = {0};
    if (argc != 1)
    {
        shellprintf(" usage: ftsetsim <xxx>\r\n");
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
static int ft_shell_setdtn(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: ftsetdtn <xxx>\r\n");
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
static int ft_shell_setdevtype(int argc, const char **argv)
{
	char devtype[10] = {0};
	
    if (argc != 1)
    {
        shellprintf(" usage: ftsetdevtype <xxx>\r\n");
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
static int ft_shell_setdevsn(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: ftsetdevsn <xxx>\r\n");
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
static int ft_shell_settype(int argc, const char **argv)
{
    unsigned char type;
    
    if (argc != 1)
    {
        shellprintf(" usage: ftsettype <xxx>\r\n");
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
static int ft_shell_setport(int argc, const char **argv)
{
    unsigned char port;
    
    if (argc != 1)
    {
        shellprintf(" usage: ftsetport <xxx>\r\n");
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
static int ft_shell_set_register(int argc, const char **argv)
{
    unsigned char status;
    
    if (argc != 1)
    {
        shellprintf(" usage: ftsetreg <xxx>\r\n");
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

static int ft_shell_setvin(int argc, const char **argv)
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

    if (ft_set_vin(argv[0]))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int ft_shell_setaddr(int argc, const char **argv)
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

    if (ft_set_addr(argv[0], port))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    sleep(1);

    return 0;
}

static int ft_shell_settmout(int argc, const const char **argv)
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

    if (ft_set_timeout(timeout))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    return 0;
}

static int ft_shell_setregtm(int argc, const const char **argv)
{
    uint16_t intv;

    if (argc != 1 || sscanf(argv[0], "%hu", &intv) != 1)
    {
        shellprintf(" usage: gbsetregtm <time in seconds>\r\n");
        return -1;
    }

    if (ft_set_regintv(intv))
    {
        shellprintf(" error: call FOTON API failed\r\n");
        return -2;
    }

    return 0;
}

static int ft_shell_status(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = FT_MSG_STATUS;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int ft_shell_suspend(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = FT_MSG_SUSPEND;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int ft_shell_resume(int argc, const const char **argv)
{
    TCOM_MSG_HEADER msg;

    msg.msgid    = FT_MSG_RESUME;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int ft_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
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

    msg.msgid    = FT_MSG_NETWORK;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(network);

    return tcom_send_msg(&msg, &network);
}

static int ft_can_callback(uint32_t event, uint32_t arg1, uint32_t arg2)
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
                msg.msgid    = FT_MSG_CANON;
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
                msg.msgid    = FT_MSG_CANOFF;
                ret = tcom_send_msg(&msg, NULL);
                break;
            }

        default:
            break;
    }

    return ret;
}

static void ft_show_status(ft_stat_t *state)
{
    unsigned char version[6];
    
    shellprintf(" FOTON status\r\n");
    shellprintf("  can         : %s\r\n", state->can ? "active" : "inactive");
    shellprintf("  network     : %s\r\n", state->network ? "on" : "off");
    shellprintf("  socket      : %s\r\n", sock_strstat(sock_status(state->socket)));
    shellprintf("  server      : %s\r\n", state->online ? "connected" : "unconnected");
    shellprintf("  suspend     : %s\r\n", state->suspend ? "yes" : "no");
    shellprintf("  log out     : %s\r\n", state->login ? "no" : "yes");

    shellprintf("  server url  : %s : %u\r\n", ft_addr.url, ft_addr.port);
    shellprintf("  vin         : %s\r\n", ft_vin);


    shellprintf("  rpt period  : %u s\r\n", ft_data_get_intv());
    shellprintf("  srv timeout : %u s\r\n", ft_timeout);
    shellprintf("  reg period  : %u s\r\n", ft_regintv);
    shellprintf("  reg seq     : %u\r\n", ft_regseq);
    shellprintf("  fix iccid   : %s\r\n", ft_iccid);
    shellprintf("  batt code   : %s\r\n", ft_battcode);
    shellprintf("  sleep       : %s\r\n", ft_allow_sleep?"allow":"no allow");
    shellprintf("  logout allow: %s\r\n", ft_is_allow_login?"not allow":"allow");
    memset(version, 0, sizeof(version));
    ft_get_tbox_ver(version, sizeof(version));
    shellprintf("  tbox version: %s\r\n", version);
}

static void *ft_main(void)
{
    int tcomfd;
    ft_stat_t state;
    uint8_t onvalue; 
    unsigned int len = 1;
                
    log_o(LOG_FOTON, "FOTON thread running");

    prctl(PR_SET_NAME, "FOTON");
    reload_wakeup_tm();
    if ((tcomfd = tcom_get_read_fd(MPU_MID_FOTON)) < 0)
    {
        log_e(LOG_FOTON, "get module pipe failed, thread exit");
        return NULL;
    }

    memset(&state, 0, sizeof(ft_stat_t));

    if ((state.socket = sock_create("FOTON", SOCK_TYPE_SYNCTCP)) < 0)
    {
        log_e(LOG_FOTON, "create socket failed, thread exit");
        return NULL;
    }

    while (1)
    {
        TCOM_MSG_HEADER msg;
        ft_msg_t msgdata;
        int res,ft_ftp_evt;
        ft_ftp_evt = 0;
        memset(&msgdata,0,sizeof(ft_msg_t));
        res = protocol_wait_msg(MPU_MID_FOTON, tcomfd, &msg, &msgdata, 200);

        if (res < 0)
        {
            log_e(LOG_FOTON, "thread exit unexpectedly, error:%s", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            case FT_MSG_FTP:
                if (msgdata.ftp_evt == FTP_PROCESS)
                {
                    ft_ftp_evt = FTP_PROCESS;
                    log_i(LOG_FOTON, "get FTP message: PROCESS %d%%", msgdata.ftp_par);
                }
                else if (msgdata.ftp_evt == FTP_ERROR)
                {
                    reload_wakeup_tm();
                    ft_ftp_evt = FTP_ERROR;
                    log_i(LOG_FOTON, "get FTP message: ERROR %s", ftp_errstr(msgdata.ftp_par));
                }
                else if (msgdata.ftp_evt == FTP_FINISH)
                {
                    log_o(LOG_FOTON, "get FTP message: FINISH %d", msgdata.ftp_par);
                    ft_ftp_evt = FTP_FINISH;
                    if (msgdata.ftp_par == 0)
                    {
                        if (1 == state.upgtype)
                        {
                            log_e(LOG_FOTON, "try to upgrade: %s", shell_cmd_exec("pkgupgrade", NULL, 0) ? "ERR" : "OK");
                        }
                        else if (2 == state.upgtype)
                        {
                            log_e(LOG_FOTON, "try to upgrade firmware: %s", upg_fw_start(UPG_FW_FPATH) ? "ERR" : "OK");
                        }
                    }
                }
                else
                {
                    log_i(LOG_FOTON, "get FTP message: unknown event");
                }

                break;

            case FT_MSG_NETWORK:
                log_i(LOG_FOTON, "get NETWORK message: %d", msgdata.network);

                if (state.network != msgdata.network)
                {
                    ft_reset(&state);
                    state.network = msgdata.network;
                }

                break;

            case PM_MSG_EMERGENCY:
                log_i(LOG_FOTON, "get EMERGENCY message");
                ft_data_emergence(1);
                break;

            case PM_MSG_OFF:
                log_i(LOG_FOTON, "get POWER OFF message");
                ft_data_emergence(0);
                ft_cache_save_all();
                break;

            case PM_MSG_RUNNING:
                log_i(LOG_FOTON, "get RUNNING message");
                ft_data_emergence(0);
                reload_wakeup_tm();
                break;

            case FT_MSG_CANON:
                log_i(LOG_FOTON, "get CANON message");
                state.can = 1;
                ft_allow_sleep = 0; 
                ft_is_allow_login  = 1;
                reload_wakeup_tm();
                break;

            case FT_MSG_CANOFF:
                log_i(LOG_FOTON, "get CANOFF message");
                state.can = 0;
                state.caltime = tm_get_time();
                //ft_allow_sleep = !state.online;
                //ft_is_allow_login  = 0;
                //tm_stop(ft_wakeup_timer);
                break;

            case FT_MSG_SUSPEND:
                log_i(LOG_FOTON, "get SUSPEND message");
                state.suspend = 1;
                ft_data_set_pendflag(1);
                break;

            case FT_MSG_RESUME:
                state.suspend = 0;
                log_i(LOG_FOTON, "get RESUME message");
                ft_data_set_pendflag(0);
                break;

            case FT_MSG_STATUS:
                log_i(LOG_FOTON, "get STATUS message");
                ft_show_status(&state);
                break;

            case FT_MSG_ERRON:
                log_i(LOG_FOTON, "get ERRON message");
                state.errtrig = 1;
                break;

            case FT_MSG_ERROFF:
                log_i(LOG_FOTON, "get ERROFF message");
                state.errtrig = 0;
                break;

            case FT_MSG_CONFIG:
                log_i(LOG_FOTON, "get CONFIG message");
                ft_do_config(&state, &msgdata.cfg);
                break;

            case MPU_MID_MID_PWDG:
                pwdg_feed(MPU_MID_FOTON);
                break;
            case AT_MSG_ID_RING:
                log_i(LOG_FOTON, "ring event wakeup");
                reload_wakeup_tm();
                break;
            case FT_MSG_WAKEUP_TIMEOUT:
                /*can off and update finish , allow sleep */
                st_get(ST_ITEM_KL15_SIG, &onvalue, &len);
                if((ft_ftp_evt == FTP_FINISH || ft_ftp_evt == FTP_ERROR )
                    && 0 == onvalue && 0 == state.can )
                {
                    log_i(LOG_FOTON, "wakeup timer timeout[%d]",tm_get_time());
                    log_i(LOG_FOTON, "can off ,no update task,on line off");
                    if(state.online) 
                    {
                        log_o(LOG_FOTON,"allow terminal logout");
                        ft_is_allow_login  = 0;
                    }
                    else 
                    {
                        log_o(LOG_FOTON,"allow terminal sleep");
                        ft_allow_sleep = 1;
                    }
                    tm_stop(ft_wakeup_timer);
                }
                else
                {
                    /*don't allow sleep */
                    reload_wakeup_tm();
                    log_i(LOG_FOTON, "FTP event %d(0 ,finish;1,error;2,process)",
                                        ft_ftp_evt); 
                    log_i(LOG_FOTON, "on state %s,can %s",onvalue?"on":"off",state.can?"on":"off");
                }
                break;
        }
        //log_i(LOG_FOTON, "current heart beat time[%d]",state.hbeattime);
        res = ft_do_checksock(&state) ||
              ft_do_receive(&state) ||
              ft_do_wait(&state) ||
              ft_do_login(&state) ||
              ft_do_suspend(&state) ||
              ft_do_report(&state) ||
              ft_do_hbeat(&state)||
              ft_do_logout(&state);

    }

    sock_delete(state.socket);
    return NULL;
}

static int ft_allow_sleep_handler(PM_EVT_ID id)
{
    return ft_allow_sleep;
}

static int ft_get_ota_status(void)
{
    log_i(LOG_FOTON, "foton ota link status:%u", ft_ota_link_state);
    return ft_ota_link_state;
}

int ft_vin_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                          unsigned char *new_para, unsigned int len)
{
    TCOM_MSG_HEADER msg;
    ft_cfg_t cfg;

    if(CFG_ITEM_FOTON_VIN != id)
    {
        log_e(LOG_FOTON, "error id:%d ",id);
        return 0;
    }
    
    /* not changed */
    if (0 == strcmp((char *)old_para , (char *)new_para))
    {
        log_o(LOG_NM, "FOTON VIN is not changed!");
        return 0;
    }
    
    msg.msgid    = FT_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.vin) + sizeof(cfg.cfgid);

    cfg.cfgid = FT_CFG_VIN;
    strncpy(cfg.vin, (char *)new_para, len);
    cfg.vin[17] = 0;
    
    return tcom_send_msg(&msg, &cfg);

}

int ft_init(INIT_PHASE phase)
{
    int ret = 0;
    uint32_t reginf = 0, cfglen;
    char ver[COM_APP_VER_LEN];
    ret |= ft_data_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(ft_vin, 0, sizeof(ft_vin));
            memset(ft_iccid, 0, sizeof(ft_iccid));
            memset(ft_battcode, 0, sizeof(ft_battcode));
            ft_addr.url[0] = 0;
            ft_addr.port   = 0;
            ft_timeout   = 5;
            ft_regdate   = 0;
            ft_regseq    = 0;
            ft_allow_sleep     = 1;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            cfglen = sizeof(reginf);
            ret = rds_get(RDS_FOTON_REGSEQ, (unsigned char *) &reginf, &cfglen, ver);

            if (ret == 0)
            {
                ft_regdate = reginf >> 16;
                ft_regseq  = reginf;
            }
            else
            {
                reginf = 0;
                ret = rds_update_once(RDS_FOTON_REGSEQ, (unsigned char *) &reginf, sizeof(reginf));
            }

            cfglen = sizeof(ft_addr.url);
            ret |= cfg_get_para(CFG_ITEM_FOTON_URL, ft_addr.url, &cfglen);
            cfglen = sizeof(ft_addr.port);
            ret |= cfg_get_para(CFG_ITEM_FOTON_PORT, &ft_addr.port, &cfglen);
            cfglen = sizeof(ft_vin);
            ret |= cfg_get_para(CFG_ITEM_FOTON_VIN, ft_vin, &cfglen);
            cfglen = sizeof(ft_regintv);
            ret |= cfg_get_para(CFG_ITEM_FOTON_REGINTV, &ft_regintv, &cfglen);
            cfglen = sizeof(ft_timeout);
            ret |= cfg_get_para(CFG_ITEM_FOTON_TIMEOUT, &ft_timeout, &cfglen);

            ret |= pm_reg_handler(MPU_MID_FOTON, ft_allow_sleep_handler);
            ret |= shell_cmd_register("ftstat", ft_shell_status, "show FOTON status");
            ret |= shell_cmd_register("ftsetvin", ft_shell_setvin, "set FOTON vin");
            ret |= shell_cmd_register("ftsetaddr", ft_shell_setaddr, "set FOTON server address");
            ret |= shell_cmd_register("ftsettmout", ft_shell_settmout, "set FOTON server timeout");
            ret |= shell_cmd_register("ftsetregtm", ft_shell_setregtm, "set FOTON register period");
            ret |= shell_cmd_register("ftsuspend", ft_shell_suspend, "stop FOTON");
            ret |= shell_cmd_register("ftresume", ft_shell_resume, "open FOTON");
//			ret |= shell_cmd_register("ftsethbintv", ft_shell_set_heartbeat, "set heartbeat period ");

			/*comm_code(7)*/
            ret |= shell_cmd_register("ftsettid", ft_shell_settid, "set Terminal ID");

			/*std_simcode*/
            ret |= shell_cmd_register("ftsetsim", ft_shell_setsim, "set sim phone num");

			/*trace_code*/
            ret |= shell_cmd_register("ftsetdtn", ft_shell_setdtn, "set Ternninal traceablity number ");

			/*terminal_kind; default ZKC02B; current YLA02*/
			/*ECU Model number*/
            ret |= shell_cmd_register("ftsetdevtype", ft_shell_setdevtype, "set devices type");

			/*�豸���*/
            ret |= shell_cmd_register("ftsetdevsn", ft_shell_setdevsn, "set devices number");

			/*����������*/
            ret |= shell_cmd_register("ftsettype", ft_shell_settype, "set engine type");

			/*ƽ̨�˿ں����*/
            ret |= shell_cmd_register("ftsetport", ft_shell_setport, "set foton port");		
			
            ret |= shell_cmd_register("ftsetreg", ft_shell_set_register, "set foton register status");
            ret |= cfg_register(CFG_ITEM_FOTON_VIN, ft_vin_changed);
            ret |= can_register_callback(ft_can_callback);
            ret |= nm_register_status_changed( ft_nm_callback);
            ret |= nm_register_get_ota_status(ft_get_ota_status);
            ret |= tm_create(TIMER_REL, FT_MSG_WAKEUP_TIMEOUT, MPU_MID_FOTON, &ft_wakeup_timer);
            break;
    }

    return ret;
}

int ft_run(void)
{
    int ret = 0;
    pthread_t tid;

    ret = pthread_create(&tid, NULL, (void *)ft_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_FOTON, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
}

int ft_set_addr(const char *url, uint16_t port)
{
    TCOM_MSG_HEADER msg;
    ft_cfg_t cfg;

    if (strlen(url) > 255)
    {
        log_e(LOG_FOTON, "url length must be less than 255: %s", url);
        return -1;
    }

    msg.msgid    = FT_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.addr) + sizeof(cfg.cfgid);

    cfg.cfgid = FT_CFG_ADDR;
    strcpy(cfg.addr.url, url);
    cfg.addr.port = port;

    return tcom_send_msg(&msg, &cfg);
}

int ft_set_vin(const char *vin)
{
    char ftvin[18];
    
    if (strlen(vin) != 17)
    {
        log_e(LOG_FOTON, "vin number must be 17 charactor: %s", vin);
        return -1;
    }
    
    memcpy(ftvin,vin,17);
    ftvin[17] = 0;
    if (cfg_set_para(CFG_ITEM_FOTON_VIN, ftvin, sizeof(ftvin)))
    {
        log_e(LOG_FOTON, "save vin failed");
        return -1;
    }
    
    return 0;
}

int ft_set_datintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    ft_cfg_t cfg;

    msg.msgid    = FT_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.datintv) + sizeof(cfg.cfgid);

    cfg.cfgid = FT_CFG_DATINTV;
    cfg.datintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int ft_set_regintv(uint16_t period)
{
    TCOM_MSG_HEADER msg;
    ft_cfg_t cfg;

    msg.msgid    = FT_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.regintv) + sizeof(cfg.cfgid);

    cfg.cfgid = FT_CFG_REGINTV;
    cfg.regintv = period;

    return tcom_send_msg(&msg, &cfg);
}

int ft_set_timeout(uint16_t timeout)
{
    TCOM_MSG_HEADER msg;
    ft_cfg_t cfg;

    msg.msgid    = FT_MSG_CONFIG;
    msg.sender   = MPU_MID_FOTON;
    msg.receiver = MPU_MID_FOTON;
    msg.msglen   = sizeof(cfg.timeout) + sizeof(cfg.cfgid);

    cfg.cfgid = FT_CFG_TIMEOUT;
    cfg.timeout = timeout;

    return tcom_send_msg(&msg, &cfg);
}

