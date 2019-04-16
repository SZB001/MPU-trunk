#include "J1939.h"
#include "rds.h"
#include "log.h"
#include "scom_api.h"
#include "can_api.h"

static J1939_DM2_QUERY dm2;

static J1939_DM1_REG_TBL  J1939_tbl;
static J1939_TIMER_T J1939_timer[3];
static pthread_mutex_t J1939_data_mutex;
static unsigned char is_need_save = 0;
static DTC_MF_T dtc_buff[256];
static VEHI_FAULT_DATA_T  dtc_table[VEHI_FAULT_TABLE_NUM];

static void J1939_vehi_fault_scan(void);

/****************************************************************
function:     uds_shell_dump_vhflt
description:  dump J1939 fault
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int J1939_shell_dump_vhflt(unsigned int argc, unsigned char **argv)
{
    int i;

    for (i = 0; i < VEHI_FAULT_TABLE_NUM; i++)
    {
        if (dtc_table[i].status == VF_START)
        {
            shellprintf("index:%d\r\n", i);
            shellprintf("port:%d, sa:%d, spn:%d, fmi:%d \r\n",
                        dtc_table[i].dtc.port, dtc_table[i].dtc.sa, dtc_table[i].dtc.spn, dtc_table[i].dtc.fmi);
        }
    }

    shellprintf("dump J1939 ok\r\n");
    return 0;
}

/****************************************************************
function:     uds_set_timer
description:  set the timer on/off by id
input:        J1939_TIMER_E  id
              uint8_t switch_value
output:       none
return:       NULL
****************************************************************/
static void  J1939_set_timer(J1939_TIMER_E  id, uint8_t switch_value)
{
    if (id >= J1939_HEART_TIMER)
    {
        J1939_timer[id - J1939_HEART_TIMER].timer_switch = switch_value;

        if (J1939_START_TIMER == switch_value)
        {
            tm_stop(J1939_timer[id - J1939_HEART_TIMER].timer_fd);
            tm_start(J1939_timer[id - J1939_HEART_TIMER].timer_fd,
                     J1939_timer[id - J1939_HEART_TIMER].timer_value, TIMER_TIMEOUT_REL_ONCE);
        }
        else
        {
            tm_stop(J1939_timer[id - J1939_HEART_TIMER].timer_fd);
        }
    }
    else
    {
        log_e(LOG_UDS, "J1939 set timer id error");
    }
}

/****************************************************************
function:     J1939_timeout
description:  process the timeout event
input:        J1939_TIMER_E  timer_id
output:       none
return:       NULL
****************************************************************/
void J1939_timeout(J1939_TIMER_E timer_id)
{
    if (timer_id >= J1939_HEART_TIMER)
    {
        switch (timer_id)
        {
            case J1939_HEART_TIMER:
                J1939_vehi_fault_scan();
                break;

            case J1939_REQ_TIMER:
                log_e(LOG_UDS, "J1939 request timeout");
                break;
        }
    }
    else
    {
        log_e(LOG_UDS, "J1939 timer id error");
    }
}

int J1939_fault_save(void)
{
    return rds_update_once(RDS_J1939_FAULT, (uint8_t *)dtc_table, sizeof(dtc_table));
}

/****************************************************************
function:     J1939_request
description:  request data
input:        unsigned char *data,
              int len
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static uint32_t J1939_request(unsigned char *data, int len)
{
    log_buf_dump(LOG_UDS, "<<<<<<<<<<<<<<<<<<J1939 send<<<<<<<<<<<<<<<<<<<", data, len);
    //return scom_tl_send_frame(SCOM_TL_CMD_DATA_J1939, SCOM_TL_SINGLE_FRAME, 0, data, len);
    return 0;
}

/****************************************************************
function:     J1939_dm2_request
description:  request the dm2 frame
input:        uint8_t port,
              uint8_t da,
              J1939_dm2_notify callback
output:       none
return:       0 indicates success;
              1 indicates is busy;
****************************************************************/
uint32_t J1939_dm2_request(uint8_t port, uint8_t da, J1939_dm2_notify callback)
{
    uint32_t can_id = 0;
    uint8_t buff[128] = {0};
    uint16_t pos = 0;

    if (J1939_DM2_BUSY == dm2.status)
    {
        log_e(LOG_UDS, "waiting DM2 response");
        return J1939_DM2_BUSY;
    }

    can_id = can_id | 0xff;      //src address
    can_id = can_id | (da << 8); //des address
    can_id = can_id | (0xEA << 16); //pf data

    buff[pos++] = MSG_ID_REQ;
    buff[pos++] = port;
    memcpy(&buff[pos], &can_id, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    /*data len*/
    buff[pos++] = 3;
    buff[pos++] = 0;

    /* data domain: PGN */
    buff[pos++] = 0x00;
    buff[pos++] = 0xEA;
    buff[pos++] = 0x00;

    dm2.da     = da;
    dm2.status = J1939_DM2_BUSY;
    dm2.func   = callback;
    J1939_set_timer(J1939_REQ_TIMER, J1939_START_TIMER);
    return J1939_request(buff, pos);
}

static int J1939_can_callback(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    if (CAN_EVENT_DATAIN == event)
    {
        int i;
        CAN_MSG *canmsg;

        for (i = 0, canmsg = (CAN_MSG *)arg1; i < arg2; i++, canmsg++)
        {
            if ((canmsg->type != 'T') && (canmsg->exten))
            {
                J1939_msg_decode(canmsg->MsgID, canmsg->port, canmsg->Data, canmsg->len);
            }
        }
    }

    return 0;
}

/****************************************************************
function:     J1939_init
description:  init J1939 module
input:        INIT_PHASE phase
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
int J1939_init(INIT_PHASE phase)
{
    int ret = 0;
    uint32_t len;
    uint8_t ver[32];

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&J1939_data_mutex, NULL);
            memset(&dm2, 0, sizeof(dm2));
            memset(&J1939_tbl, 0, sizeof(J1939_tbl));
            J1939_timer[0].timer_value = 60 * 1000;
            J1939_timer[1].timer_value = 3000;  /* refer to the timeout value in hualing tbox */
            J1939_timer[2].timer_value = 1000;
            break;

        case INIT_PHASE_RESTORE:
            len = sizeof(dtc_table);
            ret = rds_get(RDS_J1939_FAULT, (uint8_t *)dtc_table, &len, (char *)ver);

            if (0 != ret)
            {
                memset(dtc_table, 0, sizeof(dtc_table));
            }

            break;

        case INIT_PHASE_OUTSIDE:
            ret |= tm_create(TIMER_REL, J1939_HEART_TIMER, MPU_MID_UDS, &J1939_timer[0].timer_fd);
            ret |= tm_create(TIMER_REL, J1939_REQ_TIMER, MPU_MID_UDS,  &J1939_timer[1].timer_fd);

            ret |= can_register_callback(J1939_can_callback);

            if (0 != ret)
            {
                log_e(LOG_UDS, "J1939 init timer failed");
                return ret;
            }

            tm_start(J1939_timer[0].timer_fd, J1939_timer[0].timer_value, TIMER_TIMEOUT_REL_PERIOD);
            break;

        default:
            break;
    }

    return ret;
}

/****************************************************************
function:     J1939_vehi_fault_register
description:  if recv message,notify callback
input:        nm_status_changed callback
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
uint32_t J1939_dm1_register(J1939_fault_notify callback)
{
    /* the paramerter is invalid */
    if (NULL == callback)
    {
        log_e(LOG_UDS, "callback is NULL");
        return UDS_INVALID_PARA;
    }

    pthread_mutex_lock(&J1939_data_mutex);

    if (J1939_tbl.used_num >= J1939_DM1_MAX_REG)
    {
        pthread_mutex_unlock(&J1939_data_mutex);
        log_e(LOG_UDS, "j1939 register table overflow");
        return UDS_PARA_OVERFLOW;
    }

    J1939_tbl.changed[J1939_tbl.used_num] = callback;
    J1939_tbl.used_num++;
    pthread_mutex_unlock(&J1939_data_mutex);

    return 0;
}

/****************************************************************
function:     J1939_vehi_fault_start
description:  add a new fault
input:        DTC_T* dtc_p,
              unsigned int index
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static void J1939_vehi_fault_start(DTC_T *dtc_p, unsigned int index)
{
    static unsigned int misc_time = 0;
    RTCTIME local_time ;
    VEHI_FAULT_TIME_T vf_time;

    memset((void *)&dtc_table[index], 0, sizeof(VEHI_FAULT_DATA_T));

    /* update status */
    dtc_table[index].status = VF_START;

    /* dtc */
    dtc_table[index].dtc.spn = dtc_p->spn;
    dtc_table[index].dtc.fmi = dtc_p->fmi;
    dtc_table[index].dtc.cm  = dtc_p->cm;
    dtc_table[index].dtc.oc  = dtc_p->oc;
    dtc_table[index].dtc.sa  = dtc_p->sa;
    dtc_table[index].dtc.port  = dtc_p->port;

    dtc_table[index].update_time = tm_get_time();

    /* counts of vf */
    if (misc_time >= 100)
    {
        misc_time = 0;
    }

    tm_get_abstime(&local_time);

    /* if local time is not set ,use 00-00-00 00:00:00, tmpMisctime */
    if (local_time.year <= 2000)
    {
        memset((void *)dtc_table[index].startTime, 0, sizeof(dtc_table[index].startTime) - 1);
        dtc_table[index].startTime[sizeof(dtc_table[index].startTime) - 1] = misc_time ++;
    }
    else
    {
        vf_time.year = local_time.year;
        vf_time.mon = local_time.mon;
        vf_time.day = local_time.mday;
        vf_time.hour = local_time.hour;
        vf_time.min = local_time.min;
        vf_time.sec = local_time.sec;
        memcpy((void *)dtc_table[index].startTime, (void *)&vf_time,
               sizeof(dtc_table[index].startTime) - 1);
        dtc_table[index].startTime[sizeof(dtc_table[index].startTime) - 1] = misc_time ++;
    }
}

/****************************************************************
function:     J1939_vehi_fault_end
description:  end a fault
input:        unsigned int index
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static void J1939_vehi_fault_end(unsigned int index)
{
    static unsigned int misc_time = 0;
    RTCTIME local_time ;
    VEHI_FAULT_TIME_T vf_time;

    /* update status */
    dtc_table[index].status = VF_END;

    /* counts of vf */
    if (misc_time >= 100)
    {
        misc_time = 0;
    }

    tm_get_abstime(&local_time);

    /* if local time is not set ,use 00-00-00 00:00:00, tmpMisctime */
    if (local_time.year <= 2000)
    {
        memset((void *)dtc_table[index].endTime, 0, sizeof(dtc_table[index].endTime) - 1);
        dtc_table[index].endTime[sizeof(dtc_table[index].endTime) - 1] = misc_time ++;
    }
    else
    {
        vf_time.year = local_time.year;
        vf_time.mon = local_time.mon;
        vf_time.day = local_time.mday;
        vf_time.hour = local_time.hour;
        vf_time.min = local_time.min;
        vf_time.sec = local_time.sec;
        memcpy((void *)dtc_table[index].endTime, (void *)&vf_time, sizeof(dtc_table[index].endTime) - 1);
        dtc_table[index].endTime[sizeof(dtc_table[index].endTime) - 1] = misc_time ++;
    }
}

/****************************************************************
function:     J1939_vehi_fault_scan
description:  end a fault
input:        unsigned int index
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static void J1939_vehi_fault_scan(void)
{
    int i, j, ret = 0;
    static int  cnt = 0;

    for (i = 0; i < VEHI_FAULT_TABLE_NUM; i++)
    {
        if (dtc_table[i].status == VF_START)
        {
            if ((tm_get_time() - dtc_table[i].update_time) > DTC_TIMEOUT)
            {
                J1939_vehi_fault_end(i);

                for (j = 0; j < J1939_tbl.used_num; j++)
                {
                    ret = J1939_tbl.changed[j](&dtc_table[i]);

                    if (0 != ret)
                    {
                        log_e(LOG_UDS, "module motify failed");
                        break;
                    }
                }
            }

        }
    }

    cnt++;

    if ((cnt >= 5) && is_need_save)
    {
        cnt = 0;
        is_need_save = 0;
        J1939_fault_save();
    }
}

/****************************************************************
function:     J1939_dm1_pro
description:  process dm1 frame
input:        unsigned int index
output:       none
return:       0 indicates success;
              others indicates failed
****************************************************************/
static void J1939_dm1_pro(DTC_T *dtc_p, unsigned int num)
{
    int i, j, k;

    for (i = 0; i < num; i++)
    {
        for (j = 0; j < VEHI_FAULT_TABLE_NUM; j++)
        {
            if ((dtc_table[j].dtc.spn == dtc_p[i].spn) && (dtc_table[j].status == VF_START))
            {
                if (dtc_table[j].dtc.fmi != dtc_p[i].fmi)
                {
                    int n;
                    /* if fmi is changed, end it */
                    J1939_vehi_fault_end(j);

                    for (n = 0; n < J1939_tbl.used_num; n++)
                    {
                        J1939_tbl.changed[n](&dtc_table[j]);
                    }

                    /* start a new fault */
                    J1939_vehi_fault_start(&dtc_p[i], j);

                    for (n = 0; n < J1939_tbl.used_num; n++)
                    {
                        J1939_tbl.changed[n](&dtc_table[j]);
                    }

                    is_need_save = 1;
                    log_i(LOG_UDS, "fmi changed, port:%d, sa:%d, spn:%d, fmi:%d ",
                          dtc_table[j].dtc.port, dtc_table[j].dtc.sa, dtc_table[j].dtc.spn, dtc_table[j].dtc.fmi);
                }
                else
                {
                    dtc_table[j].update_time = tm_get_time();
                }

                break;
            }
        }

        /* if no found the spn, start it */
        if (VEHI_FAULT_TABLE_NUM == j)
        {
            for (k = 0; k < VEHI_FAULT_TABLE_NUM; k++)
            {
                if (dtc_table[k].status != VF_START)
                {
                    int n;
                    J1939_vehi_fault_start(&dtc_p[i], k);
                    is_need_save = 1;
                    log_i(LOG_UDS, "new spn, port:%d, sa:%d, spn:%d, fmi:%d ",
                          dtc_table[k].dtc.port, dtc_table[k].dtc.sa, dtc_table[k].dtc.spn, dtc_table[k].dtc.fmi);

                    for (n = 0; n < J1939_tbl.used_num; n++)
                    {
                        J1939_tbl.changed[n](&dtc_table[k]);
                    }

                    break;
                }
            }
        }
    }
}

/****************************************************************
function:     J1939_add_dm_data
description:  parse dm data frame
input:        DM_TYPE dm,
              unsigned int port,
              unsigned int sa,
              unsigned char *data,
              unsigned int len
output:       none
return:       NULL
****************************************************************/
static void J1939_add_dm_data(unsigned int port, unsigned int sa,
                              unsigned char *data, unsigned int len)
{
    unsigned int i, j, length;
    unsigned char *buff = NULL;
    static DTC_T temp_dtc[256];

    if (len <= 2)
    {
        return;
    }

    length = len - 2;
    buff = data + 2;

    for (i = 0; (i * 4 + 4) <= length; i++)
    {
        j = i * 4;

        /* get spn : use version 4 */
        temp_dtc[i].spn   = buff[j] + buff[j + 1] * 256 + (buff[j + 2] >> 5) * 256 * 256;
        temp_dtc[i].fmi   = buff[j + 2] & (0xFF >> 3);
        temp_dtc[i].cm    = buff[j + 3] >> 7;
        temp_dtc[i].oc    = buff[j + 3] & (0xFF >> 1);
        temp_dtc[i].sa    = sa;
        temp_dtc[i].port  = port;
    }

    /* i is the num of DTC */
    if (i)
    {
        J1939_dm1_pro(&temp_dtc[0], i);
    }
}

/****************************************************************
function:     J1939_msg_decode
description:  decode J1939 message
input:        unsigned char *msg,
              int len
output:       none
return:       NULL
****************************************************************/
void J1939_msg_decode(uint32_t canid, uint8_t port, uint8_t *data, uint8_t dlc)
{
    uint8_t  sa;
    uint32_t pgn;
    uint8_t  cur_no = 0;

    sa  = canid & 0xff;
    pgn = (canid >> 8) & 0x3ffff;

    log_i(LOG_UDS, " sa:%02x, pgn = %04x, dlc:%d", sa, pgn, dlc);

    switch (pgn)
    {
        case 0xFECA://dm1,single frame
            J1939_add_dm_data(port, sa, data, dlc);
            break;

        case 0xECFF://TP.CM_RTS
            dtc_buff[sa].total_size = (data[2] << 8) | data[1];
            dtc_buff[sa].total_no = data[3];
            dtc_buff[sa].no = 0;
            break;

        case 0xEBFF://dm1,multiple frame
            cur_no = data[0];

            if ((cur_no > dtc_buff[sa].total_no) || (cur_no != (dtc_buff[sa].no + 1)))
            {
                dtc_buff[sa].no = 0;
                dtc_buff[sa].total_no = 0;
                dtc_buff[sa].total_size = 0;
                log_e(LOG_UDS, "dm1 multiple frame error, no:%d", cur_no);
                return;
            }

            dtc_buff[sa].no = cur_no;
            memcpy(dtc_buff[sa].buff + 7 * (dtc_buff[sa].no - 1), &data[1], 7);

            if (dtc_buff[sa].no == dtc_buff[sa].total_no)
            {
                J1939_add_dm_data(port, sa, dtc_buff[sa].buff, dtc_buff[sa].total_size);
                dtc_buff[sa].total_size = 0;
                dtc_buff[sa].total_no = 0;
                dtc_buff[sa].no = 0;
            }

            break;

        case 0xFECB://dm2,single and multiple frame
            if (dm2.da != sa)
            {
                log_e(LOG_UDS, "dm2 respond error,dm2.da:%u, sa:%u", dm2.da, sa);
            }

            dm2.status = J1939_DM2_IDLE;
            J1939_set_timer(J1939_REQ_TIMER, J1939_STOP_TIMER);

            if (NULL != dm2.func)
            {
                dm2.func(data, dlc);
            }

            break;

        default:
            break;
    }

}
