/****************************************************************
 file:         at_parsing.c
 description:  the source file of AT parsing implementation
 date:         2017/02/19
 author        yuzhimin
 ****************************************************************/
#include "com_app_def.h"
#include "at_cmd.h"
#include "at_queue.h"
#include "at_packing.h"
#include "at_op.h"
#include "at_stat.h"
#include "at_data.h"
#include "at_task.h"
#include "at_api.h"
#include "pm_api.h"
#include "tcom_api.h"
#include "ql_mcm_sim.h"

extern sim_client_handle_type h_sim;
QL_SIM_APP_ID_INFO_T sim_info = {E_QL_MCM_SIM_SLOT_ID_1, E_QL_MCM_SIM_APP_TYPE_3GPP};

/****************************************************************
 function:     at_ring_wakeup
 description:  when there is message or ring, notify pm module
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
int at_ring_wakeup(void)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    msghdr.sender    = MPU_MID_AT;
    msghdr.receiver  = MPU_MID_PM;
    msghdr.msgid     = AT_MSG_ID_RING;
    msghdr.msglen    = 0;

    /* send msg id to pm */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_AT, "tcom_send_msg failed, ret:%u", ret);
    }

    
    msghdr.sender    = MPU_MID_AT;
    msghdr.receiver  = MPU_MID_FOTON;
    msghdr.msgid     = AT_MSG_ID_RING;
    msghdr.msglen    = 0;

    /* send msg id to foton thread */
    ret = tcom_send_msg(&msghdr, NULL);

    if (ret != 0)
    {
        log_e(LOG_AT, "tcom_send_msg failed, ret:%u", ret);
    }
    
    return ret;
}

/****************************************************************
 function:     rsp_ati_fn
 description:  Processing the results returned by the ATI command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_ati_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf(response, "Revision: %s", at_info.version))
    {
        log_i(LOG_AT, "Get $Revision=%s", at_info.version);
    }
}

/****************************************************************
 function:     rsp_qgsn_fn
 description:  Processing the results returned by the ATI command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_egmr_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf(response, "+EGMR: \"%[^\"]\"", at_info.imei))
    {
        log_i(LOG_AT, "Get IMEI=%s", at_info.imei);
    }
}

/****************************************************************
 function:     rsp_qimi_fn
 description:  Processing the results returned by the ATI command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qimi_fn(const char *response, AT_CMD_ID cmd_id)
{
#if 0
    if (sscanf(response, "+QIMI: %s", at_info.imsi))
    {
        log_i(LOG_AT, "Get IMSI=%s", at_info.imsi);
    }
#endif

    if (h_sim > 0)
    {
        QL_MCM_SIM_GetIMSI(h_sim, &sim_info, at_info.imsi, TBOX_IMSI_LEN);
		log_i(LOG_AT, "Get IMSI=%s", at_info.imsi);
    }
}

/****************************************************************
 function:     rsp_cops_fn
 description:  Processing the results returned by the COPS command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_cops_fn(const char *response, AT_CMD_ID cmd_id)
{
    int scanf_ret = 0;
    int net_type_temp = -1;
    int mode_temp = -1;

    memset(at_info.op_name, 0, sizeof(at_info.op_name));
    /*
     +COPS: <mode>[,<format>[,<oper>][,<Act>]]
     Example: +COPS: 0,0,¡°CHN-UNICOM¡±,0
     Act:
     0          GSM
     2          UTRAN
     3          GSM W/EGPRS
     4          UTRAN W/HSDPA
     5          UTRAN W/HSUPA
     6          UTRAN W/HSDPA and HSUPA
     7          E-UTRAN
     100        CDMA
     */

     scanf_ret = sscanf(response, "+COPS: %d,%*[^,],\"%[^\"]\",%d", &mode_temp,at_info.op_name,
                   &at_info.op_rat);

    if (scanf_ret == 3)
    {
        log_i(LOG_AT, "Get $Op_name=%s", at_info.op_name);
        log_i(LOG_AT, "Get $Op_rat=%d", at_info.op_rat);
        log_i(LOG_AT, "Get $Op_mode=%d", mode_temp);

        if(mode_temp==0)
        {
            net_type_temp = 0;
            at_info.op_mode = 0;
        }
        else
        {
            at_info.op_mode = 1;
            if(at_info.op_rat == 0)  net_type_temp = 1;
            else if(at_info.op_rat == 2)  net_type_temp = 2;
            else if(at_info.op_rat == 7)  net_type_temp = 3; 
        }        

        if( net_type_temp != at_para.net_type) 
        {
            at_set_net(at_para.net_type);
        }
    }
    else
    {
        memcpy(at_info.op_name, "UNKNOW", 6);
        at_info.op_rat = 0x10;
        log_i(LOG_AT, "SET $Op_name=%s", at_info.op_name);
        log_i(LOG_AT, "SET $Op_rat=%d", at_info.op_rat);
        log_i(LOG_AT, "Get $scanf_ret=%d", scanf_ret);
    }
}

/****************************************************************
 function:     rsp_qnwinfo_fn
 description:  Processing the results returned by the QNWINFO command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qnwinfo_fn(const char *response, AT_CMD_ID cmd_id)
{
    if( sscanf(response, "+QNWINFO: \"%*[^\"]\",\"%d\",\"%*[^\"]\",%*d",&at_info.op_num) )
    {
        log_i(LOG_AT,"Get $the operator number is %d",at_info.op_num);
    }
}

/****************************************************************
 function:     rsp_csq_fn
 description:  Processing the results returned by the CSQ command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_csq_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf(response, "+CSQ: %d,%*d", &at_info.sig_level))
    {
        log_i(LOG_AT, "Get $sigStrength=%d", at_info.sig_level);
    }
}

/****************************************************************
 function:     rsp_clcc_fn
 description:  Processing the results returned by the CLCC command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_clcc_fn(const char *response, AT_CMD_ID cmd_id)
{
    //unsigned int whitelist_len = 0;
    //unsigned int n;
    //unsigned int start = 0;
    //char whitelist_buf[32];
    //static unsigned int whitelist_flag;
    unsigned int type, status;
    char incoming_num[CALL_NUM_SIZE];

    memset(incoming_num, 0, sizeof(incoming_num));

    if (sscanf(response, "+CLCC: %*[^,],%*[^,],%d,%d,%*[^,],\"%[^\"]\",%*s",
               &status, &type, incoming_num))
    {
        log_i(LOG_AT, "Get $CLCCstatNum=%d, type = %d INCOMENUM=%s , response=%s",
              status, type, incoming_num, response);

        /* if it is cs call */
        if (0 == type)
        {
            at_call.temp_status = status;
            memcpy(at_call.incoming_num, incoming_num, sizeof(at_call.incoming_num));
            log_i(LOG_AT, "Get $CLCCstatNum=%d INCOMENUM=%s", at_call.temp_status,
                  at_call.incoming_num);
			#if 0
            if ((at_call.temp_status == 4) || (at_call.temp_status == 5))
            {
                whitelist_len = strlen(at_para.whitelist);
                whitelist_flag = 1;

                for (n = 0; n < whitelist_len; n++)
                {
                    if ((at_para.whitelist[n] == ';'))
                    {
                        log_i(LOG_AT, "temp $whitelistbuf=%s", &at_para.whitelist[start]);

                        if (1 != sscanf(&at_para.whitelist[start], "%[^;]", whitelist_buf))
                        {
                            whitelist_buf[0] = 0;
                        }

                        start = n + 1;
                        log_i(LOG_AT, "temp $whitelistbuf=%s INCOMENUM=%s", whitelist_buf,
                              at_call.incoming_num);

                        if (strcmp(whitelist_buf, at_call.incoming_num) == 0)
                        {
                            whitelist_flag = 0;
                            break;
                        }
                    }
                }

                if (whitelist_flag == 1)
                {
                    at_call.temp_status = 6;
                    disconnectcall();
                    log_e(LOG_AT, "THE CALL Number is not in white list");
                }

                log_i(LOG_AT, "LAST $CLCCstatNum=%d INCOMENUM=%s", at_call.temp_status,
                      at_call.incoming_num);
            }
			#endif
        }
    }
    else
    {
        log_i(LOG_AT, "Get $CLCCstatNum=%d", at_call.status);
    }
}

/****************************************************************
 function:     rsp_qccid_fn
 description:  Processing the results returned by the CCIC command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qccid_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf(response, "+QCCID: %s", at_info.iccid_num))
    {
        log_i(LOG_AT, "Get $ICCID=%s", at_info.iccid_num);
    }
}

/****************************************************************
 function:     rsp_cnum_fn
 description:  Processing the results returned by the at_cnum command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_cnum_fn(const char *response, AT_CMD_ID cmd_id)
{
    char temp[100] = "";
    char *str;
    char *str2;

    if (sscanf(response, "+CNUM: \"%[^\"]\"", temp))
    {
        str = strstr(temp, ",");
        str2 = strstr(str + 1, ",");
        memset(at_info.telno, 0, sizeof(at_info.telno));
        memcpy(at_info.telno, str + 1, (str2 - str - 1) / sizeof(char));
        at_info.telno[(str2 - str - 1) / sizeof(char)] = 0;
    }
    else
    {
        memset(at_info.telno, 0, sizeof(at_info.telno));
    }

    log_i(LOG_AT, "Get $CNUM=%s", at_info.telno);
}

/****************************************************************
 function:     rsp_qgps_fn
 description:  Processing the results returned by the GPS command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qgps_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf((char const *) response, "+QGPS: %d", &at_info.gnss_use))
    {
        log_i(LOG_AT, "Get $QGPS=%d", at_info.gnss_use);

        /* if gps is disable */
        if (0 == at_info.gnss_use)
        {
            /* 4G is in wakeup status */
            if (0 == at_get_at_lock())
            {
                at_enable_gps(); /* enable gps */
            }
            else /* 4G is going to enter sleep status */
            {
                at_set_ready_sleep(1);
            }
        }
        else
        {
            if (1 == at_get_at_lock())
            {
                at_set_at_unlock();
                at_disable_gps(); /* disable gps */
                at_query_gps(); /* query gps status */
                at_set_at_lock();
            }
        }

    }
}

/****************************************************************
 function:     rsp_qwifi_fn
 description:  Processing the results returned by the QWIFI command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwifi_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf(response, "+QWIFI: %d", &at_wifi.enable))
    {
        log_i(LOG_AT, "Get $QWIFI=%d", at_wifi.enable);

        if (PM_EMERGENCY_MODE == at_get_pm_mode())
        {
            at_set_wifi(0);       //disable wifi
        }
        else
        {
            if (at_wifi.enable != at_para.wifi_enable)
            {
                if (0 == at_get_at_lock())
                {
                    at_set_wifi(at_para.wifi_enable);
                }
                else/* 4G is going to enter sleep status */
                {
                    /* if wifi is open*/
                    if (at_wifi.enable)
                    {
                        at_set_at_unlock();
                        at_set_wifi(0);       //disable wifi
                        at_set_wifi(2);       //query wifi
                        at_set_at_lock();
                    }
                }
            }
        }
    }
}

/****************************************************************
 function:     rsp_qwstatus_fn
 description:  Processing the results returned by the QWSTATUS command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwstatus_fn(const char *response, AT_CMD_ID cmd_id)
{
    unsigned int wifi_status = 0;
    static unsigned int error_cnt = 0;

    if (sscanf(response, "+QWSTATUS: %d", &wifi_status))
    {
        log_i(LOG_AT, "Get $WIFI STATUS =%d", wifi_status);

        if (wifi_status < 0)
        {
            error_cnt++;

            if (error_cnt >= 5)
            {
                at_wifi.status = 2;
            }

            at_wifi_recover_cfg();
        }
        else
        {
            error_cnt = 0;
            at_wifi.status = 1;
        }
    }
}

/****************************************************************
 function:     rsp_qwauth_fn
 description:  Processing the results returned by the QWAUTH command
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwauth_fn(const char *response, AT_CMD_ID cmd_id)
{
    memset(at_wifi.key, 0, sizeof(at_wifi.key));

    if (sscanf(response, "+QWAUTH: %*[^,],%*[^,],\"%[^'\"']", at_wifi.key))
    {
        log_i(LOG_AT, "Get $QWAUTH=%s", at_wifi.key);

        if (strcmp(at_para.wifi_key, at_wifi.key) != 0)
        {
            at_set_wifi_key(at_para.wifi_key);
        }
    }
}

/****************************************************************
 function:     rsp_qwssid_fn
 description:  Processing the results returned by the QWSSID
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwssid_fn(const char *response, AT_CMD_ID cmd_id)
{
    memset(at_wifi.ssid, 0, sizeof(at_wifi.ssid));

    if (sscanf(response, "+QWSSID: %s", at_wifi.ssid))
    {
        log_i(LOG_AT, "Get $QWSSID=%s", at_wifi.ssid);

        if (strcmp(at_para.wifi_ssid, at_wifi.ssid) != 0)
        {
            at_set_wifi_ssid(at_para.wifi_ssid);
        }
    }
}

/****************************************************************
 function:     rsp_qwsetmac_fn
 description:  Processing the results returned by the QWSETMAC
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwsetmac_fn(const char *response, AT_CMD_ID cmd_id)
{
    memset(at_wifi.wlan_mac, 0, sizeof(at_wifi.wlan_mac));

    if (sscanf(response, "+QWSETMAC: \"%[^'\"']", at_wifi.wlan_mac))
    {
        log_i(LOG_AT, "Get QWSETMAC=%s", at_wifi.wlan_mac);
    }
}

/****************************************************************
 function:     rsp_qwmaxsta_fn
 description:  Processing the results returned by the qwmaxsta
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwmaxsta_fn(const char *response, AT_CMD_ID cmd_id)
{
    at_wifi.max_assoc = 0;

    if (sscanf(response, "+QWMAXSTA: %d", &at_wifi.max_assoc))
    {
        log_i(LOG_AT, "Get QWMAXSTA=%d", at_wifi.max_assoc);

        if (at_para.wifi_maxassoc != at_wifi.max_assoc)
        {
            at_set_wifi_maxassoc(at_para.wifi_maxassoc);
        }
    }
}

/****************************************************************
 function:     rsp_qwsetmac_fn
 description:  Processing the results returned by the QWSTAINFO
 input:        const char *response
 output:       none
 return:       none
 ****************************************************************/
static void rsp_qwstainfo_fn(const char *response, AT_CMD_ID cmd_id)
{
    unsigned int i = at_wifi.cli_idx;

    if (i < WIFI_CLI_MAX)
    {
        memset(at_wifi.cli_mac[i], 0, MAC_SIZE);
        memset(at_wifi.cli_ip[i], 0, IP_SIZE);
        memset(at_wifi.cli_name[i], 0, HOSTNAME_SIZE);

        sscanf(response, "+QWSTAINFO: \"%[^\"]\",\"%[^\"]\",\"%[^\"]\"",
               at_wifi.cli_mac[i], at_wifi.cli_ip[i], at_wifi.cli_name[i]);
        log_i(LOG_AT, "Get $cli_no=%u, $cli_mac=%s, cli_ip=%s, cli_name=%s",
              i, at_wifi.cli_mac[i], at_wifi.cli_ip[i], at_wifi.cli_name[i]);
        at_wifi.cli_idx++;
    }
}

static void rsp_qaudloop_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf((char const *)response, "+QAUDLOOP: %d", &at_fct.audioloop))
    {
        log_i(LOG_AT, "QAUDLOOP=%d", at_fct.audioloop);
    }
}

static void rsp_qsidet_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf((char const *)response, "+QSIDET: %d", &at_fct.gain))
    {
        log_i(LOG_AT, "QSIDET=%d", at_fct.gain);
    }
}

static void rsp_cfun_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (sscanf((char const *)response, "+CFUN: %d", &at_fct.cfun))
    {
        log_i(LOG_AT, "CFUN=%d", at_fct.cfun);
        at_info.cfun = at_fct.cfun;
    }
}

static void rsp_qcfg_fn(const char *response, AT_CMD_ID cmd_id)
{
    int qoos_en;
    if (sscanf((char const *)response, "+QCFG: \"%*[^\"]\",%d", &qoos_en))
    {
        log_i(LOG_AT, "Get QOOS_EN=%d", qoos_en);

        if (PM_EMERGENCY_MODE == at_get_pm_mode())
        {
            if(qoos_en)
            {
                at_set_qoos(0);       //disable qoos mode
            }
        }
        else
        {
            if (0 == at_get_at_lock())
            {
                /*if QOOS_EN is 1, we should shutdown this mode*/
                if(qoos_en)
                {
                    at_set_qoos(0);   //disable qoos mode
                }
            }
            else /* 4G is going to enter sleep status */
            {                            
                if(!qoos_en)
                {
                    at_set_at_unlock();
                    at_set_qoos(2);       //enable qoos mode
                    at_set_at_lock();
                }
            }
            
        }
        
    }

}

static void rsp_cgdcont_fn(const char *response, AT_CMD_ID cmd_id)
{
    int cid = 0;
    int len=0;
    char tempstr[50];
    static int cnt=0;

    if (sscanf((char const *)response, "+CGDCONT: %d,", &cid))
    {
        len = sprintf(tempstr,"+CGDCONT: %d,",cid);      
        log_i(LOG_AT, "Get cid=%d, the param is %s", cid, response+len);
        
        if( cid ==1 )
        {
            cnt=0;
        }
        else
        {
            cnt++;
            if(cnt>=10)
            {
                cnt=0;
                at_set_cid();
            }
        }
    }
       
}


/****************************************************************
 function:     rst_ok_fn
 description:  Processing the results of OK
 input:        const char *response
 const char cmd_id
 output:       none
 return:       none
 ****************************************************************/
static void rst_ok_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (cmd_id == ATD)
    {
        at_call.atd_err_cnt = 0;
    }

    else if (cmd_id == AT_CLCC)
    {
        at_set_call_status(at_call.temp_status);
    }

    else if (cmd_id == AT_QWSTAINFO)
    {
        at_wifi.cli_cnt = at_wifi.cli_idx;
    }

    else if (cmd_id == AT_QCCID && strlen(at_info.iccid_num) != 0)
    {
        at_set_sim_status(1);
    }
    else if (cmd_id == AT_QFOTA)
    {
        fota_set_status(FOTA_OK);
    }

    return;
}

/****************************************************************
 function:     rst_error_fn
 description:  Processing the results of error
 input:        const char *response
 const char cmd_id
 output:       none
 return:       none
 ****************************************************************/
static void rst_error_fn(const char *response, AT_CMD_ID cmd_id)
{
    if (cmd_id == ATD)
    {
        at_call.atd_err_cnt++;
    }
    else if (cmd_id == AT_QWIFI)
    {
        at_wifi_recover_cfg();
    }
    else if (cmd_id == AT_QFOTA)
    {
        fota_set_status(FOTA_ERROR);
    }

    return;
}

/****************************************************************
 function:     rst_cms_error_fn
 description:  Processing the results of cms error
 input:        const char *response
 const char cmd_id
 output:       none
 return:       none
 ****************************************************************/
static void rst_cms_error_fn(const char *response, AT_CMD_ID cmd_id)
{
    unsigned int code;

    /*
     310  SIM not inserted
     313  SIM failure
     314  SIM busy
     315  SIM wrong
     */
    if (sscanf((char const *) response, "+CMS ERROR: %u", &code) == 1)
    {
        log_e(LOG_AT, "+CMS ERROR: %u", code);

        if (310 == code || 313 == code || 315 == code)
        {
            // 2: sim fault
            at_set_sim_status(2);
        }
    }
}

/****************************************************************
 function:     rst_cme_error_fn
 description:  Processing the results of the cme error
 input:        const char *response
 const char cmd_id
 output:       none
 return:       none
 ****************************************************************/
static void rst_cme_error_fn(const char *response, AT_CMD_ID cmd_id)
{
    unsigned int code;

    /*
     10  SIM not inserted
     13  SIM failure
     14  SIM busy
     15  SIM wrong
     */
    if (sscanf(response, "+CME ERROR: %u", &code) == 1)
    {
        log_e(LOG_AT, "+CME ERROR: %u", code);

        if (10 == code || 13 == code || 15 == code || 30 == code)
        {
            // 2: sim fault
            at_set_sim_status(2);
        }
    }
}

/****************************************************************
 function:     at_recv_urc
 description:  Processing the results of urc
 input:        const char *buf
 output:       none
 return:       none
 ****************************************************************/
void at_recv_urc(const char *buf)
{
    if ((strstr(buf, "RING") != 0))
    {
        /*wakeup module*/
        at_ring_wakeup();

        /* if no call and the incoming call is coming */
        if (6 == at_get_call_status())
        {
            at_set_call_status(7);
        }

        log_e(LOG_AT, "Get CALL");
    }

    else if (strstr(buf, "+CMTI:") != 0)
    {
        int index = 0;
        /*wakeup module*/
        at_ring_wakeup();
        log_e(LOG_AT, "Get a short message");

        if (sscanf(buf, "+CMTI: \"ME\", %u", &index) == 1)
        {
            at_sms_del(index);
        }
    }

    else if (strstr(buf, "+STARTUP") != 0)
    {
        log_o(LOG_AT, "4G startup ok, send GPRS_INIT_IDLE_ST");
    }
}


AT_CMD_RSP at_map[AT_CMD_ID_MAX] =
{
    /*CMD_ID*/      /*CMD_NAME*/    /*RSP_STR*/     /*timeout*/  /*rsp_fn*/
    {ATE,           "ATE0",         NULL,           AT_TIMEOUT1, NULL,          },
    {ATI,           "ATI",          "Revision:",    AT_TIMEOUT1, rsp_ati_fn,    },
    {ATD,           "ATD",          NULL,           AT_TIMEOUT1, NULL,          },
    {ATA,           "ATA",          NULL,           AT_TIMEOUT1, NULL,          },
    {AT_IMEI,       "AT+EGMR",      "+EGMR:",       AT_TIMEOUT1, rsp_egmr_fn,   },
    {AT_IMSI,       "AT+QIMI",      "+QIMI:",       AT_TIMEOUT1, rsp_qimi_fn,   },
    {AT_CHUP,       "AT+CHUP",      NULL,           AT_TIMEOUT1, NULL,          },
    {AT_QNWINFO,    "AT+QNWINFO",   "+QNWINFO:",    AT_TIMEOUT1, rsp_qnwinfo_fn,},
    {AT_COPS,       "AT+COPS",      "+COPS:",       AT_TIMEOUT4, rsp_cops_fn,   },    
    {AT_CSQ,        "AT+CSQ",       "+CSQ:",        AT_TIMEOUT1, rsp_csq_fn,    },
    {AT_CLCC,       "AT+CLCC",      "+CLCC:",       AT_TIMEOUT1, rsp_clcc_fn,   },
    {AT_CNMI,       "AT+CNMI",      "+CNMI:",       AT_TIMEOUT1, NULL,          },
    {AT_CPMS,       "AT+CPMS",      "+CPMS:",       AT_TIMEOUT1, NULL,          },
    {AT_CMGD,       "AT+CMGD",      NULL,           AT_TIMEOUT1, NULL,          },
    {AT_QCCID,      "AT+QCCID",     "+QCCID:",      AT_TIMEOUT1, rsp_qccid_fn,  },
    {AT_QDAI,       "AT+QDAI",      "+QDAI:",       AT_TIMEOUT1, NULL,          },
    {AT_QAUDMOD,    "AT+QAUDMOD",   "+QAUDMOD:",    AT_TIMEOUT1, NULL,          },
    {AT_QTONE,      "AT+QCFG",      NULL,           AT_TIMEOUT1, NULL,          },
    {AT_QGPSCFG,    "AT+QGPSCFG",   "+QGPSCFG:",    AT_TIMEOUT2, NULL,          },
    {AT_QGPS,       "AT+QGPS",      "+QGPS:",       AT_TIMEOUT2, rsp_qgps_fn,   },
    {AT_QGPSEND,    "AT+QGPSEND",   NULL,           AT_TIMEOUT2, NULL,          },
    {AT_QWIFI,      "AT+QWIFI",     "+QWIFI:",      AT_TIMEOUT3, rsp_qwifi_fn,  },
    {AT_QWRSTD,     "AT+QWRSTD",    NULL,           AT_TIMEOUT1, NULL,          },
    {AT_QWSTATUS,   "AT+QWSTATUS",  "+QWSTATUS:",   AT_TIMEOUT2, rsp_qwstatus_fn, },
    {AT_QWSSID,     "AT+QWSSID",    "+QWSSID:",     AT_TIMEOUT1, rsp_qwssid_fn, },
    {AT_QWAUTH,     "AT+QWAUTH",    "+QWAUTH:",     AT_TIMEOUT2, rsp_qwauth_fn, },
    {AT_QWSETMAC,   "AT+QWSETMAC",  "+QWSETMAC:",   AT_TIMEOUT1, rsp_qwsetmac_fn, },
    {AT_QWMAXSTA,   "AT+QWMAXSTA",  "+QWMAXSTA:",   AT_TIMEOUT2, rsp_qwmaxsta_fn, },
    {AT_QWSTAINFO,  "AT+QWSTAINFO", "+QWSTAINFO:",  AT_TIMEOUT1, rsp_qwstainfo_fn,},
    {AT_QAUDLOOP,   "AT+QAUDLOOP",  "+QAUDLOOP:",   AT_TIMEOUT1, rsp_qaudloop_fn, },
    {AT_QSIDET,     "AT+QSIDET",    "+QSIDET:",     AT_TIMEOUT1, rsp_qsidet_fn,   },
    {AT_CFUN,       "AT+CFUN",      "+CFUN:",       AT_TIMEOUT1, rsp_cfun_fn,     },
    {AT_QFOTA,      "AT+QFOTADL",   NULL,           AT_TIMEOUT1, NULL,            },
    {AT_CNUM,       "AT+CNUM",      "+CNUM:",       AT_TIMEOUT1, rsp_cnum_fn,     },
    {AT_TEST,       "AT+TEST",      NULL,           AT_TIMEOUT1, NULL,            },
    {AT_QCFG,       "AT+QCFG",      "+QCFG:",       AT_TIMEOUT1, rsp_qcfg_fn,     },
    {AT_CGDCONT,    "AT+CGDCONT",   "+CGDCONT:",    AT_TIMEOUT1, rsp_cgdcont_fn,  }
};

AT_CMD_RST at_rst[RST_MAX] =
{
    /*RST_ID*/      /*RST_STR*/     /*rst_fn*/
    {RST_OK,        "OK",           rst_ok_fn,          },
    {RST_CMS_ERR,   "+CMS ERROR:",  rst_cms_error_fn,   },
    {RST_CME_ERR,   "+CME ERROR:",  rst_cme_error_fn,   },
    {RST_ERR,       "ERROR",        rst_error_fn,       },
};

