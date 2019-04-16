/****************************************************************
 file:         at_cmd.h
 description:  the header file of AT command definition
 date:         2017/02/18
 author        yuzhimin
 ****************************************************************/
#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#define AT_CMD_QUEUE_NUM        (10)
#define AT_CMD_SIZE_MAX         (100)

typedef enum
{
    AT_NOR_PRI = 0,
    AT_HIGH_PRI,
} AT_PRIORITY;

typedef enum
{
    RST_OK = 0,
    RST_CMS_ERR,
    RST_CME_ERR,
    RST_ERR,
    RST_MAX,
} AT_CMD_RET;

typedef enum
{
    ATE = 0,
    ATI,
    ATD,
    ATA,
    AT_IMEI,
    AT_IMSI,
    AT_CHUP,
    AT_QNWINFO,
    AT_COPS,    
    AT_CSQ,
    AT_CLCC,
    AT_CNMI,
    AT_CPMS,
    AT_CMGD,
    AT_QCCID,
    AT_QDAI,
    AT_QAUDMOD,         // audio
    AT_QTONE,
    AT_QGPSCFG,         // gnss
    AT_QGPS,
    AT_QGPSEND,
    AT_QWIFI,           // wifi
    AT_QWRSTD,
    AT_QWSTATUS,
    AT_QWSSID,
    AT_QWAUTH,
    AT_QWSETMAC,
    AT_QWMAXSTA,
    AT_QWSTAINFO,
    AT_QAUDLOOP,
    AT_QSIDET,
    AT_CFUN,
    AT_QFOTA,
    AT_CNUM,
    AT_TEST,
    AT_QCFG,
    AT_CGDCONT,
    AT_CMD_ID_MAX,
} AT_CMD_ID;

typedef void (*RSP_FN)(const char *str, AT_CMD_ID cmd_id);
typedef void (*RST_FN)(const char *str, AT_CMD_ID cmd_id);

typedef struct
{
    AT_CMD_ID id;
    unsigned int len;
    char str[AT_CMD_SIZE_MAX];
} AT_CMD_T;

typedef struct
{
    AT_CMD_ID id;
    char *name;
    char *rsp_str;
    unsigned int timeout;        //recv rsp timeout
    RSP_FN rsp_fn;
} AT_CMD_RSP;

typedef struct
{
    AT_CMD_RET id;
    char *rst_str;
    RST_FN rst_fn;
} AT_CMD_RST;

typedef struct
{
    AT_CMD_ID id;
    char *name;
} AT_CMD_SEND;

extern AT_CMD_RSP at_map[];
extern AT_CMD_RST at_rst[];

void at_recv_urc(const char *buf);

#endif

