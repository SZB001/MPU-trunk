#ifndef __SCOM_API_H__
#define __SCOM_API_H__
#include "scom_tl.h"
#include "mid_def.h"

#define SCOM_HEARTER_INTERVAL               (7*1000)
#define SCOM_OPEN_INTERVAL                  (50)
#define SCOM_DELAY_CLOSE_SCOM_INTERVAL      (3*1000)

/* msg id definition */
typedef enum SCOM_MSG_ID
{
    SCOM_MSG_ID_TIMER_NAME_RESEND = MPU_MID_SCOM,
    SCOM_MSG_ID_TIMER_HEARTER,
    SCOM_MSG_ID_TIMER_OPEN,
    SCOM_MSG_ID_TIMER_CLOSE_SPI,
} SCOM_MSG_ID;

/* errcode definition */
typedef enum SCOM_ERROR_CODE
{
    SCOM_INVALID_PARA = (MPU_MID_SCOM << 16) | 0x01,
    SCOM_OPEN_FAILED,
    SCOM_SEND_MSG_FAILED,
    SCOM_INIT_RINF_BUF_FAILED,
    SCOM_INIT_QUE_BUF_FAILED,
    SCOM_MSG_BUF_OVERFLOW,
    SCOM_RING_BUF_OVERFLOW,
    SCOM_MSG_QUE_EMPTY,
    SCOM_GET_MSG_FAILED,
} SCOM_ERROR_CODE;

typedef struct SCOM_HEART_T
{
    unsigned int  pwdg;
    char version[32];
} SCOM_HEART_T;

/* get transport layer status*/
int scom_tl_get_state(void);

/* send commmand to peer */
int scom_tl_send_cmd(unsigned short sender,  unsigned short receiver,
                     unsigned char *data, unsigned int len);

/* send data to peer */
int scom_tl_send_data(unsigned char *data, unsigned int len);

/* get data */
int scom_tl_get_data(unsigned char *buffer, unsigned int len, unsigned char *fin);

/* send frame */
int scom_tl_send_frame(unsigned char msg_type, unsigned char frame_type,
                       unsigned short frame_no,
                       unsigned char *data, unsigned int len);

int scom_forward_msg(unsigned short receiver, unsigned int msg_id, unsigned char *msg,
                     unsigned int len);


#endif
