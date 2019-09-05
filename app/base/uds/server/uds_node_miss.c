#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "init.h"
#include "log.h"
#include "list.h"
#include "shell_api.h"
#include "timer.h"
#include "sock_api.h"
#include "pm_api.h"
#include "timer.h"
#include "uds_node_miss.h"
#include <sys/select.h>
#include "can_api.h"
#include "uds_diag.h"
#include "tcom_api.h"
#include "fault_sync.h"
#include "dev_api.h"



static timer_t can_node_miss_test_timer;

static unsigned char can_node_miss_msgbuf[TCOM_MAX_MSG_LEN];

typedef struct
{
    unsigned long long sig_time[CAN_NODE_MISS_ITEM_NUM];
    pthread_mutex_t node_sig_time_mtx;
} NODE_SIG_TIME_T;
NODE_SIG_TIME_T node_sig_time;

typedef struct
{
    int sleep_flag;
    pthread_mutex_t sleep_flag_mutex;
} UDS_NODE_MISS_PM_T;
static UDS_NODE_MISS_PM_T uds_node_miss_pm;


typedef struct
{
    CAN_NODE_MISS_STATE miss_flag[CAN_NODE_MISS_ITEM_NUM];
    pthread_mutex_t uds_node_miss_flag_mtx;
} NODE_MISS_FLAG_T;
static NODE_MISS_FLAG_T uds_can_node_miss_flag;


typedef struct
{
    int node_id[CAN_NODE_MISS_MAX_SIGNAL];
    pthread_mutex_t miss_node_id_mtx;
} MISS_NODE_ID_T;
static MISS_NODE_ID_T miss_node_id;


static int get_miss_node_id(int sig_id)
{
    int node_id = CAN_NODE_MISS_ITEM_INVALID;
    pthread_mutex_lock(&miss_node_id.miss_node_id_mtx);
    node_id = miss_node_id.node_id[sig_id];
    pthread_mutex_unlock(&miss_node_id.miss_node_id_mtx);
    return node_id;
}

static int set_miss_node_id(int sig_id, int node_id)
{
    pthread_mutex_lock(&miss_node_id.miss_node_id_mtx);
    miss_node_id.node_id[sig_id] = node_id;
    pthread_mutex_unlock(&miss_node_id.miss_node_id_mtx);
    return 0;
}


int get_can_node_miss_state(int can_node_miss_id)
{
    int ret = 0;
    pthread_mutex_lock(&uds_can_node_miss_flag.uds_node_miss_flag_mtx);
    ret = (int)(uds_can_node_miss_flag.miss_flag[can_node_miss_id]);
    pthread_mutex_unlock(&uds_can_node_miss_flag.uds_node_miss_flag_mtx);
    return ret;
}

int set_can_node_miss_state(int can_node_miss_id, CAN_NODE_MISS_STATE miss_state)
{
    pthread_mutex_lock(&uds_can_node_miss_flag.uds_node_miss_flag_mtx);
    uds_can_node_miss_flag.miss_flag[can_node_miss_id] = miss_state;
    pthread_mutex_unlock(&uds_can_node_miss_flag.uds_node_miss_flag_mtx);
    return 0;
}

static long long get_node_sig_time(int node_id)
{
    long long ret = 0;
    pthread_mutex_lock(&node_sig_time.node_sig_time_mtx);
    ret = node_sig_time.sig_time[node_id];
    pthread_mutex_unlock(&node_sig_time.node_sig_time_mtx);
    return ret;
}

int set_node_sig_time(int node_id, long long sig_time)
{
    pthread_mutex_lock(&node_sig_time.node_sig_time_mtx);
    node_sig_time.sig_time[node_id] = sig_time;
    pthread_mutex_unlock(&node_sig_time.node_sig_time_mtx);
    return 0;
}
static void test_all_node_miss(void)
{
    const unsigned short low_voltage_threshold = 9000, high_voltage_threshold = 16000;
    int node_num = 0;
    unsigned short voltage = 0;
    unsigned int length = sizeof(unsigned short);
    int can2_busoff_status = 0;
    can2_busoff_status = flt_get_by_id(CAN_BUS2);

    if((0 == st_get(ST_ITEM_POW_VOLTAGE, (unsigned char *)&voltage, &length))
        &&(voltage >= low_voltage_threshold) 
        &&(voltage <= high_voltage_threshold)
        &&(can2_busoff_status == 1))
    {
        for (node_num = CAN_NODE_MISS_ITEM_ACU; node_num < CAN_NODE_MISS_ITEM_NUM; node_num++)
        {
            if (CAN_NODE_MISS_FLAG_NORMAL == get_can_node_miss_state(node_num))
            {
                if ((tm_get_time() - get_node_sig_time(node_num)) > CAN_NODE_MISS_CONFIRMED_TIME)
                {
                    set_can_node_miss_state(node_num, CAN_NODE_MISS_FLAG_MISS);
                }

            }
            else if (CAN_NODE_MISS_FLAG_MISS == get_can_node_miss_state(node_num))
            {
                if ((tm_get_time() - get_node_sig_time(node_num)) < CAN_NODE_MISS_TEST_INTERVAL)
                {
                    set_can_node_miss_state(node_num, CAN_NODE_MISS_FLAG_NORMAL);
                }
            }

        }
    }
    else
    {
        for (node_num = CAN_NODE_MISS_ITEM_ACU; node_num < CAN_NODE_MISS_ITEM_NUM; node_num++)
        {
            set_can_node_miss_state(node_num, CAN_NODE_MISS_FLAG_NORMAL);
            set_node_sig_time(node_num, tm_get_time());
        }
    }
    uds_diag_devices(DTC_NUM_MISSING_ACU, DIAG_ITEM_NUM);
}
static void *can_node_miss_main(void)
{
    int ret, tcom_fd;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;


    prctl(PR_SET_NAME, "CAN_NODE_MISS");

    tcom_fd = tcom_get_read_fd(MPU_MID_CAN_NODE_MISS);

    if (tcom_fd < 0)
    {
        log_e(LOG_CAN_NODE_MISS, "get can_node_miss fd failed");
        return NULL;
    }

    ret = tm_start(can_node_miss_test_timer, CAN_NODE_MISS_TEST_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

    if (ret != 0)
    {
        log_e(LOG_CAN_NODE_MISS, "start CAN node miss test timer failed!");
    }

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);
        ret = select(tcom_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(tcom_fd, &fds))
            {
                ret = tcom_recv_msg(MPU_MID_CAN_NODE_MISS, &msgheader, can_node_miss_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_CAN_NODE_MISS, "MPU_MID_CAN_NODE_MISS failed,ret:0x%08x", ret);
                    continue;
                }

                if (MPU_MID_MID_PWDG == msgheader.msgid)
                {
                    pwdg_feed(MPU_MID_CAN_NODE_MISS);
                }
                else if (MPU_MID_TIMER == msgheader.sender)
                {
                    /* used for node miss test */
                    if (CAN_NODE_MISS_TEST_TIMER == msgheader.msgid)
                    {
                        test_all_node_miss();
                    }
                }

            }
        }



    }

}


static int uds_node_miss_allow_sleep_handler(PM_EVT_ID id)
{
    int tmp;
    pthread_mutex_lock(&uds_node_miss_pm.sleep_flag_mutex);
    tmp = uds_node_miss_pm.sleep_flag;
    pthread_mutex_unlock(&uds_node_miss_pm.sleep_flag_mutex);

    return tmp ? 0 : 1;
}

static int can_node_miss_dbc_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{


    switch (event)
    {
        case DBC_EVENT_SURFIX:
            {
                int sig_id;
                uint32_t can_node_miss_id;
                const char *sfx;

                sig_id = (int)arg1;
                sfx = (const char *)arg2;

                assert(sig_id > 0 && sfx != NULL);

                if (1 != sscanf(sfx, "N%d", &can_node_miss_id))
                {
                    return 0;
                }

                set_miss_node_id(sig_id, can_node_miss_id);

                break;
            }

        case DBC_EVENT_RCVED:
            {
                int sig_id = (int)arg1;
                int node_id = get_miss_node_id(sig_id);

                if ((node_id > CAN_NODE_MISS_ITEM_INVALID) && (node_id < CAN_NODE_MISS_ITEM_NUM))
                {
                    set_node_sig_time(node_id, tm_get_time());
                }

                break;
            }

        default:
            break;
    }

    return 0;
}


int uds_node_miss_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            uds_node_miss_pm.sleep_flag = 0;
            pthread_mutex_init(&uds_node_miss_pm.sleep_flag_mutex, NULL);
            pthread_mutex_init(&uds_can_node_miss_flag.uds_node_miss_flag_mtx , NULL);
            pthread_mutex_init(&node_sig_time.node_sig_time_mtx , NULL);
            pthread_mutex_init(&miss_node_id.miss_node_id_mtx, NULL);
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= pm_reg_handler(MPU_MID_CAN_NODE_MISS, uds_node_miss_allow_sleep_handler);
            ret |= dbc_register_callback(can_node_miss_dbc_cb);
            ret = tm_create(TIMER_REL, CAN_NODE_MISS_TEST_TIMER, MPU_MID_CAN_NODE_MISS,
                            &can_node_miss_test_timer);

            if (ret != 0)
            {
                log_e(LOG_CAN_NODE_MISS, "tm_create CAN node miss test failed, ret:0x%08x", ret);
                return ret;
            }
            else
            {
                log_e(LOG_CAN_NODE_MISS, "tm_create CAN node miss test timer succeed!");
            }

            break;
    }

    return ret;
}

int uds_node_miss_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)can_node_miss_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_CAN_NODE_MISS, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;

}





