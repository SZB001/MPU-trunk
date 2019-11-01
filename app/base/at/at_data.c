/****************************************************************
 file:         at_para_manage.c
 description:  the source file of parameter definition at
 date:         2017/07/17
 author        wangqinglong
 ****************************************************************/
#include "com_app_def.h"
#include "cfg_api.h"
#include "dev_api.h"
#include "tcom_api.h"
#include "at_api.h"
#include "nm_api.h"
#include "tbox_limit.h"
#include "at_op.h"
#include "at.h"
#include "at_data.h"

static pthread_mutex_t at_data_mutex;

AT_REG_TBL at_tbl;
AT_CFG_T   at_para;
AT_INFO_T  at_info;
AT_CALL_T  at_call;
AT_WIFI_T  at_wifi;
AT_FCT_T   at_fct;

unsigned char foto_status = 0;

/******************************************************************
 function:     at_para_init
 description:  init the parameter by config file
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 ******************************************************************/
int at_para_init(void)
{
    int ret;
    unsigned int len;

    foto_status = FOTA_IDLE;
    memset(&at_para, 0, sizeof(AT_CFG_T));
    memset(&at_info, 0, sizeof(AT_INFO_T));
    memset(&at_call, 0, sizeof(AT_CALL_T));
    memset(&at_wifi, 0, sizeof(AT_WIFI_T));
    memset(&at_fct, 0, sizeof(AT_FCT_T));

    pthread_mutex_init(&at_data_mutex, NULL);

    at_call.temp_status = 6;
    at_set_call_status(at_call.temp_status);

    len = sizeof(at_para.wifi_enable);
    ret = cfg_get_para(CFG_ITEM_WIFI_SET, &at_para.wifi_enable, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi enable failed,ret:0x%08x", ret);
        return ret;
    }
	
    if(at_para.wifi_enable > 1)
    {
        at_para.wifi_enable = 0;
        shellprintf(" get the worng value of wifi enable, so set the param of wifi_enable to 0\r\n");
    }

    len = sizeof(at_para.net_type);
    ret = cfg_get_para(CFG_ITEM_NET_TYPE, &at_para.net_type, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get net type failed,ret:0x%08x", ret);
        return ret;
    }
	
    if(at_para.net_type > 3)
    {        
        at_para.net_type = 0;
        shellprintf(" get the worng value of net type, so set net to auto\r\n");
    }

    len = sizeof(at_para.wifi_ssid);
    ret = cfg_get_para(CFG_ITEM_WIFI_SSID, (unsigned char *) at_para.wifi_ssid, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi ssid failed,ret:0x%08x", ret);
        return ret;
    }
	
    if( strlen(at_para.wifi_ssid)==0 )
    {        
        memcpy(at_para.wifi_ssid, DEFAULT_SSID, strlen(DEFAULT_SSID));
        shellprintf(" get the ssid is null, so set ssid the %s\r\n", DEFAULT_SSID);
    }

    len = sizeof(at_para.wifi_key);
    ret = cfg_get_para(CFG_ITEM_WIFI_KEY, (unsigned char *) at_para.wifi_key, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi key failed,ret:0x%08x", ret);
        return ret;
    }

    len = sizeof(at_para.wifi_maxassoc);
    ret = cfg_get_para(CFG_ITEM_WIFI_MAXASSOC, &at_para.wifi_maxassoc, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi maxassoc failed,ret:0x%08x", ret);
        return ret;
    }
	
    if( at_para.wifi_maxassoc > 8)
    {        
        at_para.wifi_maxassoc = 8;
        shellprintf(" get the maxassoc of wifi is more than 8, so set maxassoc to 8\r\n");
    }

    len = sizeof(at_para.whitelist);
    ret = cfg_get_para(CFG_ITEM_WHITE_LIST, at_para.whitelist, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get white list failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

void fota_set_status(FOTA_ST status)
{
    pthread_mutex_lock(&at_data_mutex);
    foto_status = status;
    pthread_mutex_unlock(&at_data_mutex);
}

unsigned char fota_get_status(void)
{
    unsigned status;
    pthread_mutex_lock(&at_data_mutex);
    status = foto_status;
    pthread_mutex_unlock(&at_data_mutex);
    return status;
}

/****************************************************************
 function:     wifi_set_ssid
 description:  set wifi ssid name
 input:        unsigned  *ssid
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_set_ssid(const char *ssid)
{
    int ret;

    if (strlen(ssid) > WIFI_SSID_SIZE)
    {
        log_e(LOG_AT, "wifi ssid name length < 32");
        return AT_PARA_TOO_LONG;
    }

    pthread_mutex_lock(&at_data_mutex);
    memcpy(at_para.wifi_ssid, ssid, strlen(ssid));
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_WIFI_SSID, (unsigned char *) ssid, WIFI_SSID_SIZE);

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi ssid failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_get_ssid
 description:  get wifi ssid name
 input:        none
 output:       unsigned char *ssid
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_get_ssid(char *ssid)
{
    int ret;
    unsigned int len = WIFI_SSID_SIZE;

    ret = cfg_get_para(CFG_ITEM_WIFI_SSID, (unsigned char *) ssid, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi ssid failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_set_key
 description:  set wifi password
 input:        char  *key
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_set_key(const char *key)
{
    int ret;

    if (strlen(key) > WIFI_PASSWD_SIZE)
    {
        log_e(LOG_AT, "wifi key length < %d", WIFI_PASSWD_SIZE);
        return AT_PARA_TOO_LONG;
    }

    pthread_mutex_lock(&at_data_mutex);
    memcpy(at_para.wifi_key, key, strlen(key));
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_WIFI_KEY, (unsigned char *) key, WIFI_PASSWD_SIZE);

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi key failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_get_key
 description:  get wifi password
 input:        none
 output:       char  *key
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_get_key(char *key)
{
    int ret;
    unsigned int len = WIFI_PASSWD_SIZE;

    ret = cfg_get_para(CFG_ITEM_WIFI_KEY, (unsigned char *) key, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi key failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_set_max_user
 description:  set wifi max user
 input:        unsigned char num
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_set_max_user(unsigned char num)
{
    int ret;
    unsigned char maxassoc = num;

    if (num > WIFI_CLI_MAX)
    {
        log_e(LOG_AT, "maxassoc < %d", WIFI_CLI_MAX);
        return AT_PARA_TOO_LONG;
    }

    pthread_mutex_lock(&at_data_mutex);
    at_para.wifi_maxassoc = num;
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_WIFI_MAXASSOC, &maxassoc, sizeof(maxassoc));

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi maxassoc failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_get_max_user
 description:  get wifi max user
 input:        none
 output:       unsigned char *num
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_get_max_user(unsigned char *num)
{
    int ret;
    unsigned int len = sizeof(unsigned char);

    ret = cfg_get_para(CFG_ITEM_WIFI_MAXASSOC, num, &len);

    if (0 != ret)
    {
        log_e(LOG_AT, "get wifi maxassoc failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_enable
 description:  open wifi
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_enable(void)
{
    int ret;
    unsigned char enable = 1;

    pthread_mutex_lock(&at_data_mutex);
    at_para.wifi_enable = 1;
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_WIFI_SET, &enable, sizeof(enable));

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi on failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_disable
 description:  close wifi
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_disable(void)
{
    int ret;
    unsigned char enable = 0;

    pthread_mutex_lock(&at_data_mutex);
    at_para.wifi_enable = 0;
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_WIFI_SET, &enable, sizeof(enable));

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi off failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     wifi_get_ap_mac
 description:  get wifi mac
 input:        none
 output:       char* mac
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_get_ap_mac(char *mac)
{
    memcpy(mac, at_wifi.wlan_mac, MAC_SIZE);

    return 0;
}

/****************************************************************
 function:     wifi_get_sta
 description:  get the wifi station information
 input:        none
 output:       STA_INFO *info,
 unsigned int *num
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int wifi_get_sta(char *data, unsigned int *length)
{
    int i;
    int pos = 0, len = 0;

    pthread_mutex_lock(&at_data_mutex);

    for (i = 0; i < at_wifi.cli_cnt; i++)
    {
        len = strlen(at_wifi.cli_ip[i]);
        strcpy(data + pos, at_wifi.cli_ip[i]);
        pos += len;

        strcpy(data + pos, ",");
        pos += 1;

        len = strlen(at_wifi.cli_mac[i]);
        strcpy(data + pos, at_wifi.cli_mac[i]);
        pos += len;

        strcpy(data + pos, ";");
        pos += 1;
    }

    pthread_mutex_unlock(&at_data_mutex);
    *length = pos;

    return 0;
}

/****************************************************************
 function:     at_get ati
 description:  get EC20 firmware version
 input:        none
 output:       char *ver
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_get_ati(char *ver, unsigned int size)
{
    pthread_mutex_lock(&at_data_mutex);
    strncpy(ver, at_info.version, size);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_wifi_get_status
 description:  get the wifi status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int at_get_status(void)
{
    int status = 0;

    pthread_mutex_lock(&at_data_mutex);
    status = at_info.at_timeout;
    pthread_mutex_unlock(&at_data_mutex);

    return status;
}

/****************************************************************
 function:     at_wifi_get_status
 description:  get the wifi status
 input:        none
 output:       none
 return:       0 others unknow
 1 indicates normal;
 2 indicates fault;
 *****************************************************************/
int at_wifi_get_status(void)
{
    int status = 0;

    pthread_mutex_lock(&at_data_mutex);
    status = at_wifi.status;
    pthread_mutex_unlock(&at_data_mutex);

    return status;
}

/****************************************************************
 function:     at_network_switch
 description:  0x00:auto, 0x01:2G, 0x02:3G, 0x03:4G
 input:        unsigned char type
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_network_switch(unsigned char type)
{
    int ret;
    unsigned char net_type = type;

    if ((type >= 4) || (type < 0))
    {
        log_e(LOG_AT, "net type:0x00:auto,0x01:2G,0x02:3G,0x03:4G");
        return AT_INVALID_PARA;
    }

    pthread_mutex_lock(&at_data_mutex);
    at_para.net_type = type;
    pthread_mutex_unlock(&at_data_mutex);

    ret = cfg_set_para(CFG_ITEM_NET_TYPE, &net_type, sizeof(net_type));

    if (ret != 0)
    {
        log_e(LOG_AT, "set wifi maxassoc failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:     at_get_signal
 description:  get the signal of 4G module
 input:        none
 output:       none
 return:       signal value
 *****************************************************************/
int at_get_signal(void)
{
    int signal;
    pthread_mutex_lock(&at_data_mutex);
    signal = at_info.sig_level;
    pthread_mutex_unlock(&at_data_mutex);
    return signal;
}

/****************************************************************
 function:     at_get_iccid
 description:  get the ccid of sim
 input:        none
 output:       char *iccid
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_get_iccid(char *iccid)
{
    pthread_mutex_lock(&at_data_mutex);
    memcpy(iccid, at_info.iccid_num, CCID_LEN);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_get_imei
 description:  get the imei
 input:        none
 output:       char *imei
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int at_get_imei(char *imei)
{
    pthread_mutex_lock(&at_data_mutex);
    memcpy(imei, at_info.imei, TBOX_IMEI_LEN);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_get_cfun_status
 description:  get cfun status
 input:        none
 output:       none
 return:       cfun status
               0 indicates Minimum functionality;
               1 indicates Full functionality (Default)
               4 Disable phone both transmit and receive RF circuits
 *****************************************************************/
int at_get_cfun_status(void)
{
    int status;
    
    pthread_mutex_lock(&at_data_mutex);
    status = at_info.cfun;
    pthread_mutex_unlock(&at_data_mutex);
    
    return status;
}

/****************************************************************
 function:     at_get_imsi
 description:  get the imsi
 input:        none
 output:       char *imsi
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int at_get_imsi(char *imsi)
{
    pthread_mutex_lock(&at_data_mutex);
    memcpy(imsi, at_info.imsi, TBOX_IMSI_LEN);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_get_telno
 description:  get the tel no.
 input:        none
 output:       char *telno
 return:       0 indicates success;
               others indicates failed
 *****************************************************************/
int at_get_telno(char *telno)
{
    pthread_mutex_lock(&at_data_mutex);
    memcpy(telno, at_info.telno, TBOX_TEL_LEN);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_get_operator
 description:  get wifi operator
 input:        none
 output:       unsigned char *operator
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_get_operator(unsigned char *operator)
{
    pthread_mutex_lock(&at_data_mutex);
    memcpy(operator, at_info.op_name, OP_NAME_LEN);
    pthread_mutex_unlock(&at_data_mutex);
    return 0;
}

/****************************************************************
 function:     at_get_net_type
 description:  get network type
 input:        none
 output:       none
 return:       network type
 *****************************************************************/
int at_get_net_type(void)
{
    int type;
    pthread_mutex_lock(&at_data_mutex);
    type = at_info.op_rat;
    pthread_mutex_unlock(&at_data_mutex);
    return type;
}

/******************************************************************
 function:     at_set_sim_status
 description:  set the sim status
 input:        unsigned int status
 output:       none
 return:       none
 ******************************************************************/
void at_set_sim_status(unsigned int status)
{
    pthread_mutex_lock(&at_data_mutex);
    at_info.sim_status = status;
    pthread_mutex_unlock(&at_data_mutex);
}

/****************************************************************
 function:     at_get_sim_status
 description:  get sim status
 input:        none
 output:       unsigned char *status
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_get_sim_status(void)
{
    int status = 0;
    pthread_mutex_lock(&at_data_mutex);
    status = at_info.sim_status;
    pthread_mutex_unlock(&at_data_mutex);
    return status;
}

/****************************************************************
 function:   at_set_call_status
 description:get wifi operator
 input:      unsigned char status;
 7 indicates incoming call not in white list;
 6 indicates idle;
 5 indicates waiting;
 4 indicates incoming in white list;
 3 indicates altering;
 2 indicates dialing;
 1 indicates Held;
 0 indicates active;
 output:     none
 return:     0 indicates success;
 others indicates failed
 *****************************************************************/
void at_set_call_status(unsigned char status)
{
    pthread_mutex_lock(&at_data_mutex);
    at_call.status = status;
    pthread_mutex_unlock(&at_data_mutex);

    st_set(ST_ITEM_ICALL, &status, sizeof(status));
    st_set(ST_ITEM_BCALL, &status, sizeof(status));
    st_set(ST_ITEM_ECALL, &status, sizeof(status));
}

/****************************************************************
 function:     at_get_call_status
 description:  get wifi operator
 input:        none
 output:       unsigned char *status
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_get_call_status(void)
{
    int status;
    pthread_mutex_lock(&at_data_mutex);
    status = at_call.status;
    pthread_mutex_unlock(&at_data_mutex);
    return status;
}

/****************************************************************
 function:     at_is_call_busy
 description:  whether the call is busy
 input:        none
 output:       unsigned char *status
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_is_call_busy(unsigned char status)
{
    /* incoming call */
    if ((status == 4) || (status == 5))
    {
        return 1;
    }
    /* outgoing call */
    else if ((status == 2) || (status == 3))
    {
        return 1;
    }
    /* on line */
    else if ((status == 0) || (status == 1))
    {
        return 1;
    }
    /* idle */
    else if (status == 6 || status == 7)
    {
        return 0;
    }
    else
    {
        return 0;
    }
}

/****************************************************************
 function:     at_net_set_changed
 description:  when net type changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_net_set_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                              unsigned char *new_para,
                              unsigned int len)
{
    unsigned char net_type = 0;

    if (CFG_ITEM_NET_TYPE != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    if (*old_para == *new_para)
    {
        log_e(LOG_AT, "the set has no changed!");
        return 0;
    }

    net_type = *new_para;
    at_set_net(net_type);

    return 0;
}

/****************************************************************
 function:     at_get_wifi_status
 description:  get wifi status(opened or closed)
 input:        none
 output:       none
 return:      wifi status(opened or closed)
 *****************************************************************/
int at_get_wifi_status(void)
{
    unsigned int status;
    pthread_mutex_lock(&at_data_mutex);
    status = at_wifi.enable;
    pthread_mutex_unlock(&at_data_mutex);
    return status;
}

/****************************************************************
 function:     at_wifi_set_changed
 description:  when wifi on-off changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_wifi_set_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                               unsigned char *new_para,
                               unsigned int len)
{
    unsigned char wifi_enable = 0;

    if (CFG_ITEM_WIFI_SET != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    if (*old_para == *new_para)
    {
        log_e(LOG_AT, "the set has no changed!");
        return 0;
    }

    wifi_enable = *new_para;

    if (0 == wifi_enable)
    {
        at_set_wifi(0);
    }
    else if (1 == wifi_enable)
    {
        at_set_wifi(1);
    }
    else
    {
        log_e(LOG_AT, "para error!");
        return AT_INVALID_PARA;
    }

    at_para.wifi_enable = wifi_enable;

    return 0;
}

/****************************************************************
 function:     at_wifi_ssid_changed
 description:  when wifi ssid changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_wifi_ssid_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                unsigned char *new_para,
                                unsigned int len)
{
    char ssid[WIFI_SSID_SIZE + 2];

    if (CFG_ITEM_WIFI_SSID != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    if (0 == strcmp((char *) old_para, (char *) new_para))
    {
        log_e(LOG_AT, "the ssid has no changed!");
        return 0;
    }

    memset(ssid, 0, sizeof(ssid));
    memset(at_para.wifi_ssid, 0, sizeof(at_para.wifi_ssid));
    memcpy(ssid, new_para, len);
    memcpy(at_para.wifi_ssid, ssid, len);
    strncat((char *) ssid, "\r\n", 2);

    at_set_wifi_ssid(ssid);

    return 0;
}

/****************************************************************
 function:     at_wifi_key_changed
 description:  when wifi key changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_wifi_key_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                               unsigned char *new_para,
                               unsigned int len)
{
    char key[WIFI_PASSWD_SIZE + 2];

    if (CFG_ITEM_WIFI_KEY != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    if (0 == strcmp((char *) old_para, (char *) new_para))
    {
        log_e(LOG_AT, "the wifi key has no changed!");
        return 0;
    }

    memset(key, 0, sizeof(key));
    memset(at_para.wifi_key, 0, sizeof(at_para.wifi_key));
    memcpy(key, new_para, len);
    memcpy(at_para.wifi_key, key, len);
    strncat((char *) key, "\r\n", 2);

    at_set_wifi_key(key);

    return 0;
}

/****************************************************************
 function:     at_wifi_maxassoc_changed
 description:  when wifi max user changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_wifi_maxassoc_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                    unsigned char *new_para,
                                    unsigned int len)
{
    unsigned char maxassoc = 0;

    if (CFG_ITEM_WIFI_MAXASSOC != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    if (*old_para == *new_para)
    {
        log_e(LOG_AT, "para has no changed!");
        return 0;
    }

    maxassoc = *new_para;
    at_para.wifi_maxassoc = maxassoc;
    at_set_wifi_maxassoc(maxassoc);

    return 0;
}

/****************************************************************
 function:     at_whitelist_changed
 description:  when wifi max user changed
 input:        CFG_PARA_ITEM_ID id,
 unsigned char *old_para,
 unsigned char *new_para,
 unsigned int len
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
static int at_whitelist_changed(CFG_PARA_ITEM_ID id, unsigned char *old_para,
                                unsigned char *new_para,
                                unsigned int len)
{
    if (CFG_ITEM_WHITE_LIST != id)
    {
        log_e(LOG_AT, "invalid id, id:%u", id);
        return AT_INVALID_PARA;
    }

    memcpy(at_para.whitelist, new_para, len);

    return 0;
}

/****************************************************************
 function:     at_para_register
 description:  register the config which will be change
 input:        none
 output:       none
 return:       0 indicates success;
 others indicates failed
 *****************************************************************/
int at_para_register(void)
{
    int ret;
    ret = cfg_register(CFG_ITEM_WIFI_SET, at_wifi_set_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register wifi on-off changed callback failed,ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_NET_TYPE, at_net_set_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register net type callback failed,ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_WIFI_SSID, at_wifi_ssid_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register wifi ssid callback failed,ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_WIFI_KEY, at_wifi_key_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register wifi key changed callback failed,ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_WIFI_MAXASSOC, at_wifi_maxassoc_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register wifi maxassoc changed callback failed,ret:0x%08x", ret);
        return ret;
    }

    ret = cfg_register(CFG_ITEM_WHITE_LIST, at_whitelist_changed);

    if (ret != 0)
    {
        log_e(LOG_AT, "register whitelist changed callback failed,ret:0x%08x", ret);
        return ret;
    }

    return 0;
}

/****************************************************************
 function:    at_get_audioloop
 description: get audioloop
 input:       none
 output:      none
 return:      audio loop
              0 indicates disable;
              1 indicates enable;
 *****************************************************************/
int at_get_audioloop(void)
{
    unsigned int audioloop;

    pthread_mutex_lock(&at_data_mutex);
    audioloop = at_fct.audioloop;
    pthread_mutex_unlock(&at_data_mutex);

    return audioloop;
}

/****************************************************************
 function:    at_get_gain
 description: get gain
 input:       none
 output:      none
 return:      audio gain
 *****************************************************************/
int at_get_gain(void)
{
    unsigned int gain;

    pthread_mutex_lock(&at_data_mutex);
    gain = at_fct.gain;
    pthread_mutex_unlock(&at_data_mutex);

    return gain;
}

/****************************************************************
 function:    at_get_cfun
 description: get gain
 input:       none
 output:      none
 return:      cfun
 *****************************************************************/
int at_get_cfun(void)
{
    unsigned int cfun;

    pthread_mutex_lock(&at_data_mutex);
    cfun = at_fct.cfun;
    pthread_mutex_unlock(&at_data_mutex);

    return cfun;
}

void at_get_call_incoming_telno(char *incoming_num_str)
{
    pthread_mutex_lock(&at_data_mutex);
 	if((at_call.status == 6)||(at_call.status == 7))
 		 memset(incoming_num_str,0,CALL_NUM_SIZE);
 	else
  		strcpy(incoming_num_str,at_call.incoming_num);
    pthread_mutex_unlock(&at_data_mutex);
}


