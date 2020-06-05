#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "mid_def.h"
#include "init.h"
#include "log.h"
#include "shell_api.h"
#include "tcom_api.h"
#include "fota.h"
#include "xml.h"
#include "uds_client.h"
#include "uds.h"
#include "../support/protocol.h"

#define FOTA_UDS_TIMEOUT    3

enum
{
    UDS_STATE_IDLE,
    UDS_STATE_OPEN,
    UDS_STATE_WAIT_OPEN,
    UDS_STATE_WAIT_SERV,
};

typedef struct
{
    int req_type;
    int sid;
    int sub;

} fota_uds_t;

static pthread_mutex_t uds_m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  uds_c = PTHREAD_COND_INITIALIZER;
static uint8_t uds_data[512];
static int     uds_size, uds_code, blk_cnt;

static int uds_state;
static int uds_srv_id;
static int uds_sub_fun;
static int uds_req_id;
static UDS_T *uds_client;

extern void uds_hold_client(int en);
extern void set_upgrade_result_errcode(unsigned int ErrCode);

static void fota_uds_callback(UDS_T *uds, int msg_id, int can_id, uint8_t *data, int len)
{
    int next = uds_state, sid;
    unsigned int errcode;

    pthread_mutex_lock(&uds_m);

    switch (msg_id)
    {
        case -1:
            log_e(LOG_FOTA, "uds timeout");

            switch (uds_state)
            {
                case UDS_STATE_WAIT_SERV:

                    //log_e(LOG_FOTA, "x11");

                    if (uds_req_id == uds->can_id_fun || (uds_sub_fun & 0x80))
                    {
                        //log_e(LOG_FOTA, "x22");
                        uds_code = 0;
                        uds_size = 0;
                    }
                    else
                    {
                        //log_e(LOG_FOTA, "x33");
                        uds_code = -1;

                        errcode = (uds_srv_id << 8) + 0xFF;
                        set_upgrade_result_errcode(errcode);
                    }

                    next = UDS_STATE_OPEN;
                    break;

                case UDS_STATE_WAIT_OPEN:
                    uds_code = -1;
                    next = UDS_STATE_IDLE;

                default:
                    break;
            }

            break;

        case MSG_ID_UDS_C_ACK:
            if (uds_state == UDS_STATE_WAIT_OPEN)
            {
                uds_code = 0;
                uds_size = 0;
                uds_client = uds;
                next = UDS_STATE_OPEN;
            }

            break;

        case MSG_ID_UDS_IND:
            sid = data[0] == SID_NegativeResponse ? data[1] : data[0] - POS_RESPOND_SID_MASK;

            if (uds_state == UDS_STATE_WAIT_SERV && uds_srv_id == sid)
            {
                if (data[0] == SID_NegativeResponse)
                {
                    uds_code = data[2];
                    uds_size = 0;
                    next = UDS_STATE_OPEN;

                    errcode = (sid << 8) + uds_code;
                    set_upgrade_result_errcode(errcode);
                }
                else if (uds_req_id != uds->can_id_fun && uds->can_id_res == can_id)
                {
                    if (uds_sub_fun && uds_sub_fun == data[1])
                    {
                        uds_size = len - 2;
                        memcpy(uds_data, data + 2, len - 2);
                        uds_code = 0;
                        next = UDS_STATE_OPEN;
                    }
                    else if (!uds_sub_fun)
                    {
                        uds_size = len - 1;
                        memcpy(uds_data, data + 1, len - 1);
                        uds_code = 0;
                        next = UDS_STATE_OPEN;
                    }
                }
            }

        default:
            break;
    }

    if (next != uds_state)
    {
        if (data && len > 0)
        {
            protocol_dump(LOG_FOTA, "UDS RESPONSE", data, len, 0);
        }

        uds_state = next;
        pthread_cond_signal(&uds_c);
    }

    pthread_mutex_unlock(&uds_m);
}

int fota_uds_open(int port, int fid, int rid, int pid)
{
    struct timespec time;
    int res;

    if (uds_state != UDS_STATE_IDLE)
    {
        log_e(LOG_FOTA, "UDS is already opened");
        return -1;
    }

    memset(&time, 0, sizeof(time));
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += FOTA_UDS_TIMEOUT;
    pthread_mutex_lock(&uds_m);

    if (uds_set_client_ex(port, pid, fid, rid, fota_uds_callback) != 0)
    {
        log_e(LOG_FOTA, "open UDS client failed");
        pthread_mutex_unlock(&uds_m);
        return -1;
    }

    uds_state = UDS_STATE_WAIT_OPEN;

    if ((res = pthread_cond_timedwait(&uds_c, &uds_m, &time)) != 0)
    {
        log_e(LOG_FOTA, "wait for openning UDS error: %s", strerror(res));
        uds_clr_client();
        uds_state = UDS_STATE_IDLE;
        pthread_mutex_unlock(&uds_m);
        return -1;
    }

    log_e(LOG_FOTA, "UDS open ok");
    pthread_mutex_unlock(&uds_m);
    return 0;
}


void fota_uds_close(void)
{
    if (uds_state == UDS_STATE_IDLE)
    {
        log_e(LOG_FOTA, "UDS is already closed");
        return;
    }

    uds_state = UDS_STATE_IDLE;
    uds_hold_client(0);
    uds_clr_client();
}


int fota_uds_request(int bc, int sid, int sub, uint8_t *data, int len, int timeout)
{
    struct timespec time;
    int res;
    static uint8_t tmp[1024];

    if (uds_state != UDS_STATE_OPEN)
    {
        log_e(LOG_FOTA, "UDS is busy or not opened");
        return -1;
    }

    if (len + 2 > 1024)
    {
        log_e(LOG_FOTA, "data is too long");
        return -1;
    }

    tmp[0] = sid;

    if (sub)
    {
        tmp[1] = sub;
        memcpy(tmp + 2, data, len);
        len += 2;
    }
    else
    {
        memcpy(tmp + 1, data, len);
        len += 1;
    }

    memset(&time, 0, sizeof(time));
    clock_gettime(CLOCK_REALTIME, &time);
    time.tv_sec += timeout;
    pthread_mutex_lock(&uds_m);

    uds_srv_id  = sid;
    uds_sub_fun = sub;
    uds_req_id  = bc ? uds_client->can_id_fun : uds_client->can_id_phy;

    if (uds_data_request(uds_client, MSG_ID_UDS_REQ, uds_req_id, tmp, len) != 0)
    {
        log_e(LOG_FOTA, "UDS request(%s, sid=%02X, sub=%04X) fail",
              bc ? "function" : "physical", sid, sub);
        pthread_mutex_unlock(&uds_m);
        return -1;
    }

    uds_state = UDS_STATE_WAIT_SERV;

    if ((res = pthread_cond_timedwait(&uds_c, &uds_m, &time)) != 0)
    {
        uds_state = UDS_STATE_OPEN;

        if (res == ETIMEDOUT && uds_req_id == uds_client->can_id_fun)
        {
            uds_code = 0;
            uds_size = 0;
        }
        else
        {
            log_e(LOG_FOTA, "wait for UDS request(%s, sid=%02X, sub=%04X) error: %s",
                  bc ? "function" : "physical", sid, sub, strerror(res));
            pthread_mutex_unlock(&uds_m);
            return -1;
        }
    }

    //log_e(LOG_FOTA, "UDS response: %02X", uds_code);

    if (uds_code)
    {
        pthread_mutex_unlock(&uds_m);
        return -1;
    }

    if (sid == 0x10)
    {
        uds_hold_client((sub & 0x7f) != 0x01);
    }

    pthread_mutex_unlock(&uds_m);
    return 0;
}


int fota_uds_result(uint8_t *buf, int siz)
{
    if (uds_code || siz < uds_size)
    {
        return -1;
    }

    memcpy(buf, uds_data, uds_size);
    return uds_size;
}

int fota_uds_req_download(uint32_t addr, int size)
{
    int i = 0;
    uint8_t buf[64];

    /* data format identifier */
    buf[i++] = 0;
    /* address & length format identifier */
    buf[i++] = 0x44;
    /* address */
    buf[i++] = addr >> 24;
    buf[i++] = addr >> 16;
    buf[i++] = addr >> 8;
    buf[i++] = addr;
    /* length */
    buf[i++] = size >> 24;
    buf[i++] = size >> 16;
    buf[i++] = size >> 8;
    buf[i++] = size;

    if (fota_uds_request(0, 0x34, 0, buf, i, 2) != 0)
    {
        log_e(LOG_FOTA, "request download failed");
        return -1;
    }

    if (fota_uds_result(buf, sizeof(buf)) < 0)
    {
        log_e(LOG_FOTA, "response data is too long(max %d)", sizeof(buf));
    }
    else
    {
        uint8_t *p;
        int l, plen;

        for (plen = 0, l = buf[0] >> 4, p = buf + 1; l > 0; l--, p++)
        {
            plen = plen * 256 + *p;
        }

        blk_cnt = 0;
        return plen > 256 ? 256 : plen;
    }

    return -1;
}

int fota_uds_trans_data(uint8_t *data, int size)
{
    uint8_t buf[512 + 10];

    if (size + 1 > sizeof(buf))
    {
        log_e(LOG_FOTA, "data size is too long(max %d)", sizeof(buf) - 1);
        return -1;
    }

    buf[0] = ++blk_cnt;
    memcpy(buf + 1, data, size);

    return fota_uds_request(0, 0x36, 0, buf, size + 1, 2);
}

int fota_uds_trans_exit(void)
{
    return fota_uds_request(0, 0x37, 0, NULL, 0, 2);
}

int fota_uds_req_seed(int lvl, uint8_t *buf, int size)
{
    int seed_sz;

    if (fota_uds_request(0, 0x27, lvl, NULL, 0, 2) != 0)
    {
        log_e(LOG_FOTA, "request seed with level %d failed", lvl);
        return -1;
    }

    if ((seed_sz = fota_uds_result(buf, size)) < 0)
    {
        log_e(LOG_FOTA, "seed is too long(max %d)", size);
        return -1;
    }

    log_o(LOG_FOTA, "request seed success, seed size: %d", seed_sz);
    return 0;
}

int fota_uds_send_key(int lvl, uint8_t *key, int len)
{
    return fota_uds_request(0, 0x27, lvl + 1, key, len, 2);
}

int fota_uds_prog_prepare(void)
{
    if (fota_uds_request(1, 0x10, 0x03, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x85, 0x02, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x28, 0x03, (uint8_t *)"\x01", 1, 2) != 0)
    {
        return -1;
    }

    return 0;
}

int fota_uds_prog_post(void)
{
    if (fota_uds_request(1, 0x10, 0x03, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x28, 0, (uint8_t *)"\x00\x01", 2, 2) != 0 ||
        fota_uds_request(1, 0x85, 0x01, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x10, 0x01, NULL, 0, 2) != 0)
    {
        return -1;
    }

    return 0;
}


int fota_uds_read_data_by_identifier(uint8_t *identifier, uint8_t *data, int *len)
{
    uint8_t ret = -1;
    uint32_t DataLen;

    ret = fota_uds_request(0, 0x22, 0, identifier, 2, 2);

    if (0 == ret)
    {
        DataLen = uds_size - 2;
        if(DataLen >  *len)
        {
            log_w(LOG_FOTA, "Data truncated %dB -> %dB.", DataLen, *len);
        }
        else
        {
            *len = DataLen;
        }
    
        memcpy(data, uds_data + 2, *len);
    }

    return ret;
}


int fota_uds_write_data_by_identifier(uint8_t *identifier, uint8_t *data, int len)
{
    uint8_t buf[256];

    if (len + 2 > 256)
    {
        log_e(LOG_FOTA, "data len is too long, %d", len);
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    memcpy(buf, identifier, 2);
    memcpy(buf + 2, data, len);

    return fota_uds_request(0, 0x2E, 0, buf, len + 2, 5);
}

//This Function Use To Start GW Upgrade Other ECU, It Needs ?? To Upgrade, So We Wait 1 Hour For It
int fota_uds_write_data_by_identifier_ex(uint8_t *identifier, uint8_t *data, int len)
{
    uint8_t buf[256];

    if (len + 2 > 256)
    {
        log_e(LOG_FOTA, "data len is too long, %d", len);
        return -1;
    }

    memset(buf, 0, sizeof(buf));
    memcpy(buf, identifier, 2);
    memcpy(buf + 2, data, len);

    return fota_uds_request(0, 0x2E, 0, buf, len + 2, 60 * 60);
}

int fota_uds_enter_diag(void)
{
    return fota_uds_request(0, 0x10, 0x02, NULL, 0, 2);
}

int fota_uds_enter_diag_GW(void)
{
    return fota_uds_request(0, 0x10, 0x04, NULL, 0, 2);
}

int fota_uds_enter_normal_GW(void)
{
    return fota_uds_request(0, 0x10, 0x01, NULL, 0, 2);
}


int fota_uds_reset(void)
{
    return fota_uds_request(0, 0x11, 0x01, NULL, 0, 2);
}

int fota_uds_get_version_gw(uint8_t *s_ver,            int32_t *s_siz, 
                                   uint8_t *h_ver,    int32_t *h_siz, 
                                   uint8_t *bl_ver,   int32_t *bl_siz, 
                                   uint8_t *sn,       int32_t *sn_siz,
                                   uint8_t *partnum,  int32_t *partnum_siz,
                                   uint8_t *supplier, int32_t *supplier_siz)
{
    fota_uds_enter_diag_GW();

    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x8C", sn, sn_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\xC0", s_ver, s_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x80", bl_ver, bl_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x91", h_ver, h_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x87", partnum, partnum_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x8A", supplier, supplier_siz);

    fota_uds_enter_normal_GW();

    return 0;
}

int fota_uds_get_version(uint8_t *s_ver,           int32_t *s_siz, 
                               uint8_t *h_ver,    int32_t *h_siz, 
                               uint8_t *bl_ver,   int32_t *bl_siz, 
                               uint8_t *sn,       int32_t *sn_siz,
                               uint8_t *partnum,  int32_t *partnum_siz,
                               uint8_t *supplier, int32_t *supplier_siz)
{
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x8C", sn, sn_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\xC0", s_ver, s_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x80", bl_ver, bl_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x91", h_ver, h_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x87", partnum, partnum_siz);
    fota_uds_read_data_by_identifier((uint8_t *)"\xF1\x8A", supplier, supplier_siz);

    return 0;
}

int foton_erase(uint32_t addr, int size)
{
    uint8_t tmp[16];

    tmp[0]  = 0xff;
    tmp[1]  = 0;
    tmp[2]  = 0x44;
    tmp[3]  = addr >> 24;
    tmp[4]  = addr >> 16;
    tmp[5]  = addr >> 8;
    tmp[6]  = addr;
    tmp[7]  = size >> 24;
    tmp[8]  = size >> 16;
    tmp[9]  = size >> 8;
    tmp[10] = size;

    if (fota_uds_request(0, 0x31, 0x01, tmp, 11, 5) != 0 ||
        (fota_uds_result(tmp, sizeof(tmp)) != 3 && tmp[0] != 0xff && tmp[1] != 0 && tmp[2] != 0))
    {
        log_e(LOG_FOTA, "erase flash(addr=%08X, size=%d) fail", addr, size);
        return -1;
    }

    return 0;
}
