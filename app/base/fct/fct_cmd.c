/****************************************************************
file:         fct_cmd.c
description:  the source file of fct implementation
date:         2018/1/3
author        liuzhongwen
****************************************************************/
#include <sys/vfs.h>
#include <string.h>
#include "com_app_def.h"
#include "fct.h"
#include "fct_cmd.h"
#include "diag.h"
#include "gpio.h"
#include "timer.h"
#include "scom_api.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "nm_api.h"
#include "dir.h"
#include "file.h"
#include "diag.h"
#include "audio.h"
#include "at_api.h"
#include "gps_api.h"
#include "at.h"
#include "scom_msg_def.h"


static timer_t      fct_in_ant_test_timer;
static timer_t      fct_audioloop_test_timer;

static unsigned int fct_in_ant_test_seq;
static unsigned int fct_audioloop_test_seq;

void fct_cmd_build_res(unsigned int seq, unsigned short cmd,
                       unsigned short len, unsigned char *data,
                       unsigned char *ack, unsigned short *olen)
{
    /* add seq into ack*/
    memcpy(ack, &seq,  sizeof(seq));
    *olen = sizeof(seq);

    /* add cmd into ack*/
    memcpy(ack + *olen, &cmd, FCT_CMD_LEN);
    *olen += FCT_CMD_LEN;

    /* add type into ack*/
    //memcpy( ack + *olen, FCT_CMD_RES, FCT_CMD_TYPE_LEN);
    *(ack + *olen) = FCT_CMD_RES;
    *olen += FCT_CMD_TYPE_LEN;

    /* add length into ack*/
    memcpy(ack + *olen, &len, sizeof(len));
    *olen += sizeof(len);

    /* add data into ack*/
    memcpy(ack + *olen, data, len);
    *olen += len;
}

void fct_cmd_timetout_proc(unsigned int msgid)
{
    int sig;
    unsigned short olen;
    unsigned char ack[128];
    unsigned char ret;

    if (FCT_IN_ANT_TEST_TIMER == msgid)
    {
        sig = at_get_signal();

        fct_cmd_build_res(fct_in_ant_test_seq, FCT_4G_IN_ANT, sizeof(sig), (unsigned char *)&sig, ack,
                          &olen);
    }
    else if (FCT_AUDIOLOOP_TEST_TIMER == msgid)
    {
        if ((1 == at_get_audioloop()) && (10000 == at_get_gain()))
        {
            ret = FCT_CMD_OK;
        }
        else
        {
            ret = FCT_CMD_NG;
        }

        fct_cmd_build_res(fct_audioloop_test_seq, FCT_4G_AUDIOLOOP, sizeof(ret), (unsigned char *)&ret, ack,
                          &olen);
    }

    ret = scom_tl_send_frame(SCOM_TL_CMD_FCT, SCOM_TL_SINGLE_FRAME, 0, ack, olen);

    log_buf_dump(LOG_FCT, "<<<<<<<<<<<<fct send<<<<<<<<<<<<<<", ack, olen);

    if (ret != 0)
    {
        log_e(LOG_FCT, "tm_create heart timer failed, ret:0x%08x", ret);
    }
}

int fct_scom_msg_proc(unsigned char *msg, unsigned int len)
{
    SCOM_TL_MSG_HDR *tl_hdr = (SCOM_TL_MSG_HDR *)msg;

    if (len < sizeof(SCOM_TL_MSG_HDR))
    {
        log_e(LOG_FCT, "invalid message,len:%u", len);
        return -1;
    }

    if (((tl_hdr->msg_type & 0xf0) >> 4) != SCOM_TL_CHN_FCT)
    {
        log_e(LOG_FCT, "invalid message,msgtype:%u, fct:%u", tl_hdr->msg_type, SCOM_TL_CHN_FCT);
        return -1;
    }

    switch (tl_hdr->msg_type)
    {
        case SCOM_TL_CMD_FCT:
            scom_forward_msg(MPU_MID_FCT, FCT_MSG_FCT_IND,
                             msg + sizeof(SCOM_TL_MSG_HDR), len - sizeof(SCOM_TL_MSG_HDR));
            break;

        case SCOM_TL_CMD_FCT_LOG:
            fct_save_log(msg + sizeof(SCOM_TL_MSG_HDR) + sizeof(int),
                         len - sizeof(SCOM_TL_MSG_HDR) - sizeof(int));
            break;

        default:
            log_e(LOG_FCT, "invalid message,msgtype:%u", tl_hdr->msg_type);
            break;
    }

    return 0;
}

int fct_cmd_init(INIT_PHASE phase)
{
    int ret;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret = tm_create(TIMER_REL, FCT_IN_ANT_TEST_TIMER, MPU_MID_FCT, &fct_in_ant_test_timer);

            if (ret != 0)
            {
                log_e(LOG_FCT, "tm_create heart timer failed, ret:0x%08x", ret);
                return ret;
            }

            ret = tm_create(TIMER_REL, FCT_AUDIOLOOP_TEST_TIMER, MPU_MID_FCT, &fct_audioloop_test_timer);

            if (ret != 0)
            {
                log_e(LOG_FCT, "tm_create heart timer failed, ret:0x%08x", ret);
                return ret;
            }

            ret = scom_tl_reg_proc_fun(SCOM_TL_CHN_FCT, fct_scom_msg_proc);

            if (ret != 0)
            {
                log_e(LOG_FCT, "reg scom proc failed, ret:0x%08x", ret);
                return ret;
            }

            break;

        default:
            break;
    }

    return 0;
}

int fct_emmc_test(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    struct statfs diskInfo;
    unsigned int res_data[2];

    if (!bfile_exists("/dev/mmcblk0p1"))
    {
        static int cnt = 1;

        if (cnt)
        {
            cnt = 0;
            system("/home/root/format.sh");
        }
    }

    statfs(COM_SDCARD_DIR, &diskInfo);

    unsigned long long blocksize = diskInfo.f_bsize;                    //ÿ��block��������ֽ���
    unsigned long long totalsize = blocksize *
                                   diskInfo.f_blocks;       //�ܵ��ֽ�����f_blocksΪblock����Ŀ

    unsigned long long availableDisk = diskInfo.f_bavail * blocksize;   //���ÿռ��С

    res_data[0] = totalsize >> 20;
    res_data[1] = availableDisk >> 20;

    *olen = sizeof(res_data);
    memcpy(res, res_data, *olen);

    return 0;
}

int fct_4G_iccid_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                      unsigned int *olen)
{
    int ret;

    ret = at_get_iccid((char *)res);

    if (0 == ret)
    {
        *olen = CCID_LEN;
    }

    return ret;
}

int fct_4G_signal_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                       unsigned int *olen)
{
    int sig;

    sig = at_get_signal();

    *olen = sizeof(sig);

    memcpy(res, &sig, sizeof(sig));

    return 0;
}

int fct_4G_net_test(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    #if 0
    int ret;
    unsigned int  len;
    unsigned char tsp_comm;

    ilen = sizeof(tsp_comm);
    ret  = st_get(ST_ITEM_TSP_COMM, &tsp_comm, &len);

    if (ret != 0)
    {
        log_e(LOG_FCT, "fct_4G_net_test, ret:0x%08x", ret);
        return ret;
    }

    #endif

    *olen = 1;

    if (true == nm_get_net_status())
    {
        res[0] = FCT_CMD_OK;
    }
    else
    {
        res[0] = FCT_CMD_NG;
    }

    return 0;
}

int fct_4G_ex_mant_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen)
{
    int ret;

    ret = dev_diag_get_ant_status(ANT_4G_MAIN);

    res[0] = ret & 0xff;
    *olen  = 1;

    return 0;
}

int fct_4G_ex_sant_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen)
{
    int ret;

    ret = dev_diag_get_ant_status(ANT_4G_VICE);

    res[0] = ret & 0xff;
    *olen  = sizeof(res[0]);

    return 0;
}

int fct_4G_in_ant_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                       unsigned int *olen)
{
    int ret;

    log_o(LOG_DEV, "change 4G ant to internal");

    gpio_set_level(GPIO_MAIN_ANT_CTRL, PINLEVEL_HIGH);
    gpio_set_level(GPIO_SUB_ANT_CTRL,  PINLEVEL_HIGH);

    ret = tm_start(fct_in_ant_test_timer, FCT_IN_ANT_TEST_VAL, TIMER_TIMEOUT_REL_ONCE);

    if (0 != ret)
    {
        log_e(LOG_FCT, "start timer failed, ret:0x%08x", ret);
    }

    return ret;
}

int fct_gps_locate_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen)
{
    res[0] = gps_get_fix_status();
    *olen  = sizeof(res[0]);

    return 0;
}

int fct_csq_battvol_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                         unsigned int *olen)
{
    int sig, net_type;

    sig = at_get_signal();

    net_type = at_get_net_type();

    memcpy(res, &sig, sizeof(sig));
    *olen = sizeof(sig);

    memcpy(res + *olen, &net_type, sizeof(net_type));
    *olen += sizeof(net_type);

    return 0;
}

int fct_ring_wake_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                       unsigned int *olen)
{
    if (ilen != 1)
    {
        log_e(LOG_FCT, "invalid ilen:%u", ilen);
        return 1;
    }

    if (req[0] > 0)
    {
        gpio_set_level(GPIO_WAKEUP_MCU, PINLEVEL_LOW);
    }
    else if (req[0] == 0)
    {
        gpio_set_level(GPIO_WAKEUP_MCU, PINLEVEL_HIGH);
    }

    *olen = 1;
    res[0] = FCT_CMD_OK;

    return 0;
}

int fct_audio_loop_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen)
{
    int ret;

    audio_route_4G_to_ols(false);
    //audio_route_4G_to_ils(false);
    audio_route_mic_bypass(false);

    at_set_audioloop(1);
    at_set_audioloopGAIN(10000);

    at_query_audioloop();
    at_query_audioloopGAIN();

    ret = tm_start(fct_audioloop_test_timer, FCT_AUDIOLOOP_TEST_VAL, TIMER_TIMEOUT_REL_ONCE);

    if (0 != ret)
    {
        log_e(LOG_FCT, "start timer failed, ret:0x%08x", ret);
    }

    return ret;
}

int fct_4G_cfun_test(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    at_set_cfun(4);

    at_query_cfun();

    res[0] = FCT_CMD_OK;

    *olen = 1;

    return 0;
}

int fct_wifi_ssid_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                       unsigned int *olen)
{
    unsigned char ssid[128];

    if (ilen == 0 || ilen >= 128)
    {
        log_e(LOG_FCT, "invalid ilen:%u", ilen);
        return 1;
    }

    memcpy(ssid, req, ilen);
    ssid[ilen] = 0;

    //at_set_wifi_ssid(ssid);

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;
}

int fct_ble_name_test(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    res[0] = FCT_CMD_NG;
    *olen = 1;

    return 0;
}

int fct_wifi_enale_test(unsigned char *req, unsigned int ilen, unsigned char *res,
                        unsigned int *olen)
{
    if (ilen != 1)
    {
        log_e(LOG_FCT, "invalid ilen:%u", ilen);
        return 1;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    at_set_wifi(req[0]);

    return 0;
}

int fct_local_apn_set(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    if ((APN_SIZE < ilen) || (ilen != strlen((char *)req)))
    {
        log_e(LOG_FCT, "invalid ilen, localapn:%s,ilen:%u", req, ilen);
        return 1;
    }

    if (0 != cfg_set_para(CFG_ITEM_LOCAL_APN, req, APN_SIZE))
    {
        res[0] = FCT_CMD_NG;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;
}

int fct_wan_apn_set(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    if ((APN_SIZE < ilen) || (ilen != strlen((char *)req)))
    {
        log_e(LOG_FCT, "invalid ilen, wanapn:%s,ilen:%u", req, ilen);
        return 1;
    }

    if (0 != cfg_set_para(CFG_ITEM_WAN_APN, req, APN_SIZE))
    {
        res[0] = FCT_CMD_NG;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;

}

int fct_url_set(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    if ((256 < ilen) || (ilen != strlen((char *)req)))
    {
        log_e(LOG_FCT, "invalid ilen, url:%s,ilen:%u", req, ilen);
        return 1;
    }

    if (0 != cfg_set_para(CFG_ITEM_FOTON_URL, req, 256))
    {
        res[0] = FCT_CMD_NG;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;
}

int fct_port_set(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    unsigned short port;

    memcpy(&port, req, sizeof(port));

    if (sizeof(port) != ilen)
    {
        log_e(LOG_FCT, "invalid ilen, ilen:%u", ilen);
        return 1;
    }

    if (0 != cfg_set_para(CFG_ITEM_FOTON_PORT, &port, sizeof(port)))
    {
        res[0] = FCT_CMD_NG;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;
}

int fct_sn_set(unsigned char *req, unsigned int ilen, unsigned char *res, unsigned int *olen)
{
    unsigned int sn;

    memcpy(&sn, req, sizeof(sn));

    if (sizeof(sn) != ilen)
    {
        log_e(LOG_FCT, "invalid ilen, ilen:%u", ilen);
        return 1;
    }

    if (0 != cfg_set_para(CFG_ITEM_SN_NUM, &sn, sizeof(sn)))
    {
        res[0] = FCT_CMD_NG;
    }

    res[0] = FCT_CMD_OK;
    *olen = 1;

    return 0;
}

int fct_save_log(unsigned char *msg, unsigned int len)
{
    int ret;
    static FILE *fp = NULL;
    char path[32];
    RTCTIME time;

    /* emmc is umount */
    if (DIAG_EMMC_OK != dev_diag_get_emmc_status())
    {
        log_e(LOG_FCT, "emmc is can not save log");
        return 0;
    }

    memset(path, 0, sizeof(path));
    tm_get_abstime(&time);
    sprintf(path, "/media/sdcard/fctlog/%04d%02d%02d%02d", time.year, time.mon, time.mday, time.hour);

    if (!file_exists(path))
    {
        ret = dir_make_path(path, S_IRUSR | S_IWUSR | S_IXUSR, true);

        if (0 != ret)
        {
            log_e(LOG_FCT, "make fctlog dir failed, ret:%08x", ret);
            return ret;
        }

        if (NULL != fp)
        {
            fflush(fp);
            fclose(fp);
            fp = NULL;
        }

        fp = fopen(path, "a+");

        if (fp == NULL)
        {
            log_e(LOG_FCT, "open %s failed", path);
            return -1;
        }
    }
    else
    {
        if (NULL == fp)
        {
            fp = fopen(path, "a+");

            if (fp == NULL)
            {
                log_e(LOG_FCT, "open %s failed", path);
                return -1;
            }
        }
    }

    fwrite(msg, sizeof(char), len, fp);

    return 0;
}


FCT_CMD fct_cmd_table[] =
{
    { FCT_CMD_EMMC,          fct_emmc_test          },
    { FCT_4G_ICCID,          fct_4G_iccid_test      },
    { FCT_4G_SIGNAL,         fct_4G_signal_test     },
    { FCT_4G_NET,            fct_4G_net_test        },
    { FCT_4G_EX_M_ANT,       fct_4G_ex_mant_test    },
    { FCT_4G_EX_S_ANT,       fct_4G_ex_sant_test    },
    { FCT_4G_IN_ANT,         fct_4G_in_ant_test     },
    { FCT_GPS,               fct_gps_locate_test    },
    { FCT_4G_ACT,            fct_csq_battvol_test   },
    { FCT_4G_RING,           fct_ring_wake_test     },
    { FCT_4G_AUDIOLOOP,      fct_audio_loop_test    },
    { FCT_4G_CFUN,           fct_4G_cfun_test       },
    { FCT_WIFI_SSID,         fct_wifi_ssid_test     },
    { FCT_BLE_NAME,          fct_ble_name_test      },
    { FCT_WIFI_ENBALE,       fct_wifi_enale_test    },
    { FCT_LOCAL_APN,         fct_local_apn_set      },
    { FCT_WAN_APN,           fct_wan_apn_set        },
    { FCT_URL,               fct_url_set            },
    { FCT_PORT,              fct_port_set           },
    { FCT_SET_SN,            fct_sn_set             },
};

int fct_cmd_proc(unsigned char *msg, int len)
{
    int ret;
    unsigned int   seq, i, olen;
    unsigned char  ack[128], buff[128];
    unsigned short ilen, ack_len;
    unsigned short cmd;

    log_buf_dump(LOG_FCT, ">>>>>>>>>>>>>fct recv>>>>>>>>>>>>>", msg, len);

    if (len < sizeof(seq) + FCT_CMD_LEN + FCT_CMD_TYPE_LEN + sizeof(ilen))
    {
        log_e(LOG_FCT, "invalid msg len, len:0x%08x", len);

        ret = 0x1234;

        seq  = msg[0] + (msg[1] << 8) + (msg[2] << 16) + (msg[3] << 24);
        fct_cmd_build_res(seq, FCT_CMD_NG_RES, sizeof(ret), (unsigned char *)&ret, ack, &ack_len);

        ret = scom_tl_send_frame(SCOM_TL_CMD_FCT, SCOM_TL_SINGLE_FRAME, 0, ack, ack_len);
        log_buf_dump(LOG_FCT, "<<<<<<<<<<<<fct send<<<<<<<<<<<<<<", ack, ack_len);

        if (0 != ret)
        {
            log_e(LOG_FCT, "send fct response failed, ret:0x%08x", ret);
        }

        return 0;
    }

    seq  = msg[0] + (msg[1] << 8) + (msg[2] << 16) + (msg[3] << 24);
    cmd  = msg[4] + (msg[5] << 8);
    ilen = msg[7] + (msg[8] << 8);

    //log_i(LOG_FCT, "seq:%08x, cmd:%04x, type:%u, ilen:%d", seq,cmd,type,ilen);

    for (i = 0; i < sizeof(fct_cmd_table) / sizeof(FCT_CMD); i++)
    {
        if (cmd == fct_cmd_table[i].cmd)
        {
            break;
        }
    }

    if (i < sizeof(fct_cmd_table) / sizeof(FCT_CMD))
    {
        ret = fct_cmd_table[i].proc(msg + sizeof(seq) + FCT_CMD_LEN + FCT_CMD_TYPE_LEN + sizeof(ilen),
                                    ilen, buff, &olen);

        if (ret != 0)
        {
            fct_cmd_build_res(seq, FCT_CMD_NG_RES, sizeof(ret), (unsigned char *)&ret, ack, &ack_len);
        }
        else
        {
            if (FCT_4G_AUDIOLOOP == fct_cmd_table[i].cmd)
            {
                fct_audioloop_test_seq = seq;
                return 0;
            }
            else if (FCT_4G_IN_ANT == fct_cmd_table[i].cmd)
            {
                fct_in_ant_test_seq = seq;
                return 0;
            }
            else
            {
                fct_cmd_build_res(seq, cmd, olen, buff, ack, &ack_len);
            }
        }
    }
    else
    {
        log_e(LOG_FCT, "invalid msg cmd, cmd:0x%04x", cmd);

        ret = 0x1235;

        fct_cmd_build_res(seq, FCT_CMD_NG_RES, sizeof(ret), (unsigned char *)&ret, ack, &ack_len);
    }

    ret = scom_tl_send_frame(SCOM_TL_CMD_FCT, SCOM_TL_SINGLE_FRAME, 0, ack, ack_len);

    log_buf_dump(LOG_FCT, "<<<<<<<<<<<<fct send<<<<<<<<<<<<<<", ack, ack_len);

    if (0 != ret)
    {
        log_e(LOG_FCT, "send fct response failed, ret:0x%08x", ret);
    }

    return ret;
}


