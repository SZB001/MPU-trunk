/****************************************************************
 file:         at_packing.h
 description:  the header file of GPRS AT CMD Pack IF definition
 date:         2017/04/18
 author        yuzhimin
 ****************************************************************/
#ifndef __AT_PACKING_H__
#define __AT_PACKING_H__

#include "at_cmd.h"

void at_atd(AT_CMD_T *at, char *num);
void at_ata(AT_CMD_T *at);
void at_chup(AT_CMD_T *at);
void at_cops(AT_CMD_T *at, unsigned char op);
void at_csq(AT_CMD_T *at);
void at_clcc(AT_CMD_T *at);
void at_cmgr(AT_CMD_T *at, unsigned int index);
void at_cmgd(AT_CMD_T *at, unsigned int index);
void at_qccid(AT_CMD_T *at);
void at_qgps(AT_CMD_T *at, unsigned char op);
void at_qgpsend(AT_CMD_T *at);
void at_qwifi(AT_CMD_T *at, unsigned char op);
void at_qwstatus(AT_CMD_T *at);
void at_qwssid(AT_CMD_T *at, char *ssid);
void at_qwauth(AT_CMD_T *at, char *passwd);
void at_qwsetmac(AT_CMD_T *at, char *mac);
void at_qwmaxsta(AT_CMD_T *at, unsigned char op);
void at_qwrstd(AT_CMD_T *at);
void at_qwstainfo(AT_CMD_T *at);
void at_qaudloop(AT_CMD_T *at, unsigned int *op);
void at_qsidet(AT_CMD_T *at, unsigned int *op);
void at_cfun(AT_CMD_T *at, unsigned int *op);
void at_qfotal(AT_CMD_T *at);
void at_qoos(AT_CMD_T *at, unsigned char op);
void at_cid(AT_CMD_T *at);

#endif
