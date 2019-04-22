#ifndef __AP_DATA_H__
#define __AP_DATA_H__
#include "init.h"



typedef struct AP_SOCK_INFO
{
    int      sockfd;
    int      connect_st;
    int      waitcnt;
    int      network;
    uint8_t  url[256];
    uint16_t port;
} AP_SOCK_INFO;

int ap_data_init(INIT_PHASE phase);

int ap_recv_proc(AP_SOCK_INFO* info, unsigned char *msg, unsigned int *len);
void ap_msg_proc(AP_SOCK_INFO* info, unsigned char *msg, unsigned int len);

int ap_do_report(int fd);
int ap_do_report_hb(int fd);
int ap_do_report_ecu_info(int fd);

#endif
