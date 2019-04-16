/****************************************************************
file:         assist.h
description:  the header file of assist api definition
date:         2017/07/12
author        liuwei
****************************************************************/

#ifndef __ASSIST_H__
#define __ASSIST_H__

#include "mid_def.h"
#include "init.h"
#include <netinet/in.h>    // for sockaddr_in

//SOCKET
#define ASSIST_SERVER_PORT                  50000
#define BUF_SIZE                            1024
#define MAX_FILE_BUFFSIZE                   (500 * 1024)
#define TWO_SECONDS                         2
#define DISCONNECT_TIMEOUT                  30
#define MAX_SEND_BUF_SIZE                   2048
#define MAX_RECV_BUF_SIZE                   2048
#define MAX_CLIENT_NUM                      5
#define BACK_LOG MAX_CLIENT_NUM

//TIMER interval
#define RECREATE_INTERVAL                   5000
#define HB_INTERVAL                         (10 * 1000)


/*protocol define*/
#define IDENTIFIER                          0x7e
#define TRANSFER_CHAR                       0x7d

#define DATA_TYPE_U8                        0x01
#define DATA_TYPE_U16                       0x02
#define DATA_TYPE_U32                       0x03
#define DATA_TYPE_F32                       0x04
#define DATA_TYPE_D64                       0x05
#define DATA_TYPE_STRING                    0x06
#define DATA_TYPE_BIN                       0x07

//0x8005   语音通话
#define STATE_ICALL_COMEIN                  0x01
#define STATE_BCALL_COMEIN                  0x02
#define STATE_DIALING                       0x03
#define STATE_INCALL                        0x04
#define STATE_IDLE                          0x05

//0X8101 运营商
#define OPERATE_UNICOM                      0x01
#define OPERATE_CMCC                        0x02
#define OPERATE_CUCC                        0x03

//0x8102制式,   0X0105
#define STANDARD_2G                         0x01
#define STANDARD_3G                         0x02
#define STANDARD_4G                         0x03
#define STANDARD_4G                         0x03
#define STANDARD_AUTO                       0x04


//I/B-CALL
#define REQUEST_ICALL                       0x0001
#define REQUEST_BCALL                       0x0002
#define REQUEST_ACCEPT_CALL                 0x0003
#define REQUEST_HANGUP_CALL                 0x0004
#define REQUEST_CALL_STATE                  0x0005

#define RESPONSE_ICALL                      0x8001
#define RESPONSE_BCALL                      0x8002
#define RESPONSE_ACCEPT_CALL                0x8003
#define RESPONSE_HANGUP_CALL                0x8004
#define RESPONSE_CALL_STATE                 0x8005


//2G/3G/4G
#define REQUEST_OPERATOR                    0x0101
#define REQUEST_STANDARD                    0x0102
#define REQUEST_SIGNAL                      0x0103
#define REQUEST_DATA_OPERAT                 0x0104
#define REQUEST_CFG_STANDARD                0x0105
#define REQUEST_DATA_STATE                  0x0106
#define REQUEST_STANDARD_CFG                0x0107

#define RESPONSE_OPERATOR                   0x8101
#define RESPONSE_STANDARD                   0x8102
#define RESPONSE_SIGNAL                     0x8103
#define RESPONSE_DATA_OPERAT                0x8104
#define RESPONSE_CFG_STANDARD               0x8105
#define RESPONSE_DATA_STATE                 0x8106
#define RESPONSE_STANDARD_CFG               0x8107

//WLAN AP
#define REQUEST_TBOX_MAC                    0x0201
#define REQUEST_CLIENT_MAC                  0x0202
#define REQUEST_OPERATE_WLAN                0x0203
#define REQUEST_SET_SSID                    0x0204
#define REQUEST_SET_WIFI_PASSWORD           0x0205
#define REQUEST_SET_MAX_CLIENT              0x0206
#define REQUEST_WLAN_STATE                  0x0207
#define REQUEST_SSID                        0x0208
#define REQUEST_WIFI_PASSWORD               0x0209
#define REQUEST_MAX_CLIENT                  0x020A

#define RESPONSE_TBOX_MAC                   0x8201
#define RESPONSE_CLIENT_MAC                 0x8202
#define RESPONSE_OPERATE_WLAN               0x8203
#define RESPONSE_SET_SSID                   0x8204
#define RESPONSE_SET_WIFI_PASSWORD          0x8205
#define RESPONSE_SET_MAX_CLIENT             0x8206
#define RESPONSE_WLAN_STATE                 0x8207
#define RESPONSE_SSID                       0x8208
#define RESPONSE_WIFI_PASSWORD              0x8209
#define RESPONSE_MAX_CLIENT                 0x820A

//售后APP
#define REQUEST_SHELL                       0x0301
#define REQUEST_LOG                         0x0302
#define REQUEST_DOWNLOAD_START              0x0401
#define REQUEST_DOWNLOAD_DATA               0x0402
#define REQUEST_UPLOAD_START                0x0403
#define REQUEST_UPLOAD_DATA                 0x0404
#define REQUEST_UDP_DOWNLOAD_START          0x0405
#define REQUEST_UDP_DOWNLOAD_DATA           0x0406

#define RESPONSE_SHELL                      0x8301
#define RESPONSE_LOG                        0x8302
#define RESPONSE_DOWNLOAD_START             0x8401
#define RESPONSE_DOWNLOAD_DATA              0x8402
#define RESPONSE_UPLOAD_START               0x8403
#define RESPONSE_UPLOAD_DATA                0x8404
#define RESPONSE_UDP_DOWNLOAD_START         0x8405
#define RESPONSE_NO_DOWNLOAD_START          0x4405
#define RESPONSE_UDP_DOWNLOAD_DATA          0x8406
#define RESPONSE_NO_UPLOAD_START            0x4403

/*  data type*/

/* Protocol NG type */
typedef enum
{
    PROTOCOLNG_NOSUPPORT = 0,
    PROTOCOLNG_ILLEGAL,
} PROTOCOLNG_TYPE;

//TIMER
typedef enum
{
    TIMER_ASSIST_RECREATE    =  0x01,

    //heart beat timer
    TIMER_ASSIST_TIMEOUT_START = 0x02,
    TIMER_ASSIST_TIMEOUT_1   =  TIMER_ASSIST_TIMEOUT_START,
    TIMER_ASSIST_TIMEOUT_2  ,
    TIMER_ASSIST_TIMEOUT_3  ,
    TIMER_ASSIST_TIMEOUT_4  ,
    TIMER_ASSIST_TIMEOUT_5  ,

    TIMER_ASSIST_TIMEOUT_END = TIMER_ASSIST_TIMEOUT_5,


} enum_assist_timer;

typedef struct
{
    unsigned short timer_id;
    short client_fd_index;
} struct_fd_timer;

typedef struct
{
    unsigned char op_index;
    unsigned char op_name[20];
} struct_op_map;

typedef struct
{
    unsigned char filename[256];
    unsigned int filesize;
    unsigned char data[0];
} struct_file_info;

typedef struct
{
    int fd;
    unsigned int lasthearttime;
    struct sockaddr_in addr;
} struct_client;

/* initiaze assist module */
int assist_init(INIT_PHASE phase);

/* startup assist module */
int assist_run(void);

void UdpProcessProtocol(unsigned char *data, unsigned int datalen, int clientfd);
void TcpProcessProtocol(unsigned char *data, unsigned int datalen, void *para);


#endif

