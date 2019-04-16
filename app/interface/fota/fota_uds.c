#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "uds.h"
#include "log.h"
#include "fota.h"
#include "uds_client.h"
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

static pthread_mutex_t uds_mutex = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t  uds_cond1 = PTHREAD_COND_INITIALIZER; 
static pthread_cond_t  uds_cond2 = PTHREAD_COND_INITIALIZER;  
static uint8_t uds_data[512];
static int     uds_size, uds_code, blk_cnt;

static int uds_state;
static int uds_srv_id;
static int uds_sub_fun;
static int uds_req_id;
static UDS_T *uds_client;

static void fota_uds_callback(UDS_T *uds, int msg_id, int can_id, uint8_t *data, int len)
{
    int next = uds_state, sid;

    pthread_mutex_lock(&uds_mutex);

    //printf("uds state: %d\n", uds_state);
    
    switch (msg_id)
    {
        case -1:
            switch (uds_state)
            {
                case UDS_STATE_WAIT_SERV:
                    if (uds_req_id == uds->can_id_fun || (uds_sub_fun & 0x80))
                    {
                        uds_code = 0;
                        uds_size = 0;
                    }
                    else
                    {
                        uds_code = -1;
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
        //printf("wakeup, %llu\n", tm_get_time());
        pthread_cond_signal(&uds_cond1);
        pthread_cond_wait(&uds_cond2, &uds_mutex);        
    }

    pthread_mutex_unlock(&uds_mutex);
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
    
    pthread_mutex_lock(&uds_mutex);
    
    if (uds_set_client_ex(port, pid, fid, rid, fota_uds_callback) != 0)
    {
        log_e(LOG_FOTA, "open UDS client failed");
        pthread_mutex_unlock(&uds_mutex);
        return -1;
    }

    uds_state = UDS_STATE_WAIT_OPEN;
    res = pthread_cond_timedwait(&uds_cond1, &uds_mutex, &time);
    pthread_cond_signal(&uds_cond2);
    
    if (res)
    {
        log_e(LOG_FOTA, "wait for openning UDS error: %s", strerror(res));
        uds_clr_client();
        uds_state = UDS_STATE_IDLE;
        pthread_mutex_unlock(&uds_mutex);
        return -1;
    }

    log_o(LOG_FOTA, "UDS open ok");
    pthread_mutex_unlock(&uds_mutex);
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
    time.tv_sec += 2;
    
    pthread_mutex_lock(&uds_mutex);

    uds_srv_id  = sid;
    uds_sub_fun = sub;
    uds_req_id  = bc ? uds_client->can_id_fun : uds_client->can_id_phy;

    if (uds_data_request(uds_client, MSG_ID_UDS_REQ, uds_req_id, tmp, len) != 0)
    {        
        log_i(LOG_FOTA, "UDS request(%s, sid=%02X, sub=%04X) fail", 
            bc ? "function" : "physical", sid, sub);
        pthread_mutex_unlock(&uds_mutex);
        return -1;
    }

    protocol_dump(LOG_FOTA, "UDS REQUEST", tmp, len, 1);
    
retry:
    //printf("request, %llu\n", tm_get_time());
    uds_state = UDS_STATE_WAIT_SERV;
    res = pthread_cond_timedwait(&uds_cond1, &uds_mutex, &time);
    pthread_cond_signal(&uds_cond2);
    
    if (res)
    {
        uds_state = UDS_STATE_OPEN;
        if (res == ETIMEDOUT && uds_req_id == uds_client->can_id_fun)
        {
            uds_code = 0;
            uds_size = 0;
        }
        else
        {
            //printf("error, %llu\n", tm_get_time());
            log_e(LOG_FOTA, "wait for UDS request(%s, sid=%02X, sub=%04X) error: %s", 
                bc ? "function" : "physical", sid, sub, strerror(res));
            pthread_mutex_unlock(&uds_mutex);
            return -1;
        }
    }

    if (uds_code == 0x78)
    {
        log_i(LOG_FOTA, "UDS request delayed");
        memset(&time, 0, sizeof(time));
        clock_gettime(CLOCK_REALTIME, &time);
        time.tv_sec += 6;
        //printf("retry, %llu\n", tm_get_time());
        goto retry;
    }
    
    if (uds_code)
    {
        log_e(LOG_FOTA, "UDS request fail: %02X", uds_code & 0xff);
        pthread_mutex_unlock(&uds_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&uds_mutex);
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

int fota_uds_req_download(uint32_t base, int size)
{
    int i = 0;
    uint8_t buf[64];

    /* data format identifier */
    buf[i++] = 0;
    /* address & length format identifier */
    buf[i++] = 0x44;
    /* address */
    buf[i++] = base >> 24;
    buf[i++] = base >> 16;
    buf[i++] = base >> 8;
    buf[i++] = base;
    /* length */
    buf[i++] = size >> 24;
    buf[i++] = size >> 16;
    buf[i++] = size >> 8;
    buf[i++] = size;

    if (fota_uds_request(0, 0x34, 0, buf, i, 3) != 0)
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
        return plen > 258 ? 256 : plen - 2;
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

    return fota_uds_request(0, 0x36, 0, buf, size + 1, 3);
}

int fota_uds_trans_exit(void)
{
    return fota_uds_request(0, 0x37, 0, NULL, 0, 3);
}
