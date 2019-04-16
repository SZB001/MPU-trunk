/****************************************************************
 file:         at_op.h
 description:  the header file of AT status definition
 date:         2017/02/18
 author        yuzhimin
 ****************************************************************/
#ifndef __AT_OP_H__
#define __AT_OP_H__

void makecall(char *num);
void answercall(void);
void disconnectcall(void);
void at_set_net(unsigned char type);
void at_query_gps(void);
void at_set_wifi(unsigned char op);
void at_set_wifi_ssid(char *ssid);
void at_set_wifi_key(char *key);
void at_set_wifi_maxassoc(unsigned char num);
void at_set_wifi_mac(char *mac);
void at_wifi_recover_cfg(void);
void at_sms_del(unsigned int index);
void at_set_audioloop(unsigned int enable);
void at_set_audioloopGAIN(unsigned int gain);
void at_query_audioloop(void);
void at_query_audioloopGAIN(void);
void at_query_cfun(void);
void at_set_cfun(unsigned int enable);

void at_fota_update(void);
void at_test(const char *atcmdtest);

void at_set_qoos(unsigned char op);
void at_set_cid(void);

#endif
