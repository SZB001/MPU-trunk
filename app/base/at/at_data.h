/****************************************************************
 file:         at_data.h
 description:  the header file of at data manage module
 date:         2017/07/14
 author        wangqinglong
 ****************************************************************/
#ifndef __AT_DATA_H__
#define __AT_DATA_H__

#include "at.h"
#include "init.h"
#include "at_stat.h"

#define AT_MAX_REG_TBL      6

typedef struct AT_REG_TBL
{
    unsigned char used_num;
    at_sms_arrived changed[AT_MAX_REG_TBL];
} AT_REG_TBL;

typedef struct
{
    unsigned char net_type;
    unsigned char wifi_enable;
    unsigned char wifi_maxassoc;
    char wifi_mac[MAC_SIZE];
    char wifi_ssid[WIFI_SSID_SIZE];
    char wifi_key[WIFI_PASSWD_SIZE];
    char whitelist[WHITELIST_MAX_SIZE];
} AT_CFG_T;

typedef struct
{
    unsigned int op_rat;
    unsigned int op_mode;
    unsigned int op_num;      //oper number
    unsigned int sim_status;    // 0: sim inserted; 1: sim not inserted
    unsigned int cfun;
    unsigned int gnss_use;
    unsigned int at_lock;      // 1: locked, can't send at cmd; 0: unlocked, can send at cmd;
    unsigned int sig_level;
    unsigned char is_ready_sleep;
    unsigned char at_timeout;
    char iccid_num[CCID_LEN];
    char op_name[OP_NAME_LEN];
    char version[TBOX_FW_VER_LEN];
    char imei[TBOX_IMEI_LEN];
    char imsi[TBOX_IMSI_LEN];
    char telno[TBOX_TEL_LEN];
} AT_INFO_T;

/* AT_call.status:
 <stat>  State of the call
 0    Active
 1    Held
 2    Dialing (MO call)
 3    Alerting (MO call)
 4    Incoming (MT call)
 5    Waiting (MT call)
 6    No call
 7    RING
 */
typedef struct
{
    unsigned int atd_err_cnt;
    unsigned int status;
    unsigned int temp_status;
    char outgoing_num[CALL_NUM_SIZE];
    char incoming_num[CALL_NUM_SIZE];
} AT_CALL_T;

typedef struct
{
    unsigned int enable;
    unsigned int status;    //0 UNKNOW ; 1 OK; 2 FAULT
    unsigned int cli_cnt;
    unsigned int cli_idx;
    unsigned int max_assoc;
    char key[WIFI_PASSWD_SIZE];
    char ssid[WIFI_SSID_SIZE];
    char wlan_mac[MAC_SIZE];
    char cli_ip[WIFI_CLI_MAX][IP_SIZE];
    char cli_mac[WIFI_CLI_MAX][MAC_SIZE];
    char cli_name[WIFI_CLI_MAX][HOSTNAME_SIZE];
} AT_WIFI_T;


typedef struct
{
    unsigned int audioloop;
    unsigned int gain;
    unsigned int cfun;
} AT_FCT_T;


extern AT_REG_TBL at_tbl;
extern AT_CFG_T   at_para;
extern AT_INFO_T  at_info;
extern AT_CALL_T  at_call;
extern AT_WIFI_T  at_wifi;
extern AT_FCT_T   at_fct;

int at_para_init(void);
int at_para_register(void);

void at_set_sim_status(unsigned int status);
void at_set_call_status(unsigned char status);
int at_get_call_status(void);
int at_is_call_busy(unsigned char status);

int at_get_cfun(void);
int at_get_gain(void);
int at_get_audioloop(void);


#endif

