#include "com_app_def.h"
#include "pwdg.h"
#include "tcom_api.h"
#include "timer.h"
#include "log.h"
#include "mid_def.h"


static unsigned int    wdg_food
;  /* each bit in wdg_food indicates the feed status of the responding moudle ,
                                      0 indicates the moudle does not feed watchdog in the interval,
                                      1 indicates the moudle feed watchdog in the interval */
static pthread_mutex_t wdg_mutex;
static timer_t         wdg_timer;

static unsigned short mid_filter_table[] = \
{
    \
    MPU_MID_TIMER, \
    MPU_MID_TCOM,  \
    MPU_MID_SCOM,  \
    MPU_MID_FCT,   \
    MPU_MID_FOTA,  \
    MPU_MID_GEELYHU,
};

/****************************************************************
function:     pwdg_init
description:  the pthread which need wdg init the timer
input:        unsigned short mid,
              unsigned int msgid
output:       none
return:       none
*****************************************************************/
int pwdg_init(unsigned short mid)
{
    int ret, i;

    /* clear the watchdog */
    wdg_food = (0xffffffff << (MPU_APP_MID_COUNT));

    /* the feed status in the mid filer table is always set to 1 */
    for (i = 0; i < sizeof(mid_filter_table) / sizeof(short); i++)
    {
        wdg_food = wdg_food | (1 << MID_TO_INDEX(mid_filter_table[i]));
    }

    log_o(LOG_MID, "wdg_food: %08x", wdg_food);

    pthread_mutex_init(&wdg_mutex, NULL);

    ret = tm_create(TIMER_REL, MPU_MID_MID_PWDG, mid, &wdg_timer);

    if (ret != 0)
    {
        log_e(LOG_MID, "tm_create wdg timer failed, ret:0x%08x", ret);
        return ret;
    }

    ret = tm_start(wdg_timer, PTHREAD_WDG_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);

    if (ret != 0)
    {
        log_e(LOG_MID, "tm_start wdg timer failed, ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     pwdg_timeout
description:  the pthread which need wdg init the timer
input:        unsigned short mid,
              unsigned int msgid
output:       none
return:       none
*****************************************************************/
int pwdg_timeout(unsigned short mid)
{
    int i, j;
    int ret;
    unsigned short receiver;
    TCOM_MSG_HEADER msghdr;

    for (i = 0; i < MPU_APP_MID_COUNT; i++)
    {
        receiver = INDEX_TO_MID(i);

        for (j = 0; j < sizeof(mid_filter_table) / sizeof(short); j++)
        {
            if (receiver == mid_filter_table[j])
            {
                break;
            }
        }

        /* if found in mid filter table, do nothing */
        if (j < sizeof(mid_filter_table) / sizeof(short))
        {
            continue;
        }

        /* if not found in mid filter table, notify the mid to feed watchdog */
        msghdr.sender     = mid;
        msghdr.receiver   = receiver;
        msghdr.msgid      = MPU_MID_MID_PWDG;
        msghdr.msglen     = 0;

        ret = tcom_send_msg(&msghdr, NULL);

        if (ret != 0)
        {
            log_e(LOG_MID, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
                  msghdr.msgid, msghdr.receiver, ret);
            return ret;
        }
    }

    return 0;
}


/****************************************************************
function:     pwdg_feed
description:  the pthread which init wdg must feed wdg
input:        unsigned short mid
output:       none
return:       none
*****************************************************************/
void pwdg_feed(unsigned short mid)
{
    if ((mid < MPU_MIN_MID) || (mid > MPU_MAX_MID))
    {
        log_e(LOG_MID, "para error, mid:0x%04x", mid);
        return;
    }

    pthread_mutex_lock(&wdg_mutex);
    wdg_food = wdg_food | (1 << MID_TO_INDEX(mid));
    pthread_mutex_unlock(&wdg_mutex);
}

/****************************************************************
function:     pwdg_food
description:  get the food
input:        unsigned short mid
output:       none
return:       none
*****************************************************************/
unsigned int pwdg_food(void)
{
    unsigned int i, food;

    pthread_mutex_lock(&wdg_mutex);

    food = wdg_food;

    /* clear the watchdog */
    wdg_food = (0xffffffff << MPU_APP_MID_COUNT);

    for (i = 0; i < sizeof(mid_filter_table) / sizeof(short); i++)
    {
        wdg_food = wdg_food | (1 << MID_TO_INDEX(mid_filter_table[i]));
    }

    pthread_mutex_unlock(&wdg_mutex);
    return food;
}

