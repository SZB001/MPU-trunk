#ifndef __J1939_H__
#define __J1939_H__
#include <stdint.h>
#include "init.h"
#include "uds.h"


/* Vehi-Fault */
#define  VEHI_FAULT_TABLE_NUM               (256)

/* BUS timeout : 60s */
#define  DTC_TIMEOUT                        (60 * 1000)

typedef enum
{
    J1939_HEART_TIMER = UDS_DIAG_TIMER + 1,
    J1939_REQ_TIMER,
} J1939_TIMER_E;

typedef enum
{
    J1939_STOP_TIMER = 0,
    J1939_START_TIMER,
} J1939_TIMER_OP;

typedef enum
{
    J1939_DM2_IDLE = 0,
    J1939_DM2_BUSY,
} J1939_DM2_STATUS;


typedef struct
{
    timer_t    timer_fd;
    uint8_t    timer_switch;
    uint32_t   timer_value;
} J1939_TIMER_T;

typedef enum
{
    VF_NULL = 0,
    VF_START,
    VF_END,
} VF_ST;

typedef enum
{
    ERR = 0,
    OK,
} J1939_RESULT;

typedef enum
{
    DM1 = 0,
    DM2,
} DM_TYPE;

typedef enum
{
    MSG_ID_CFG = 0xF1, /*mpu设置J1939传输层*/
    MSG_ID_REQ,        /*mpu发向ECU的数据*/
    MSG_ID_DM,         /*ECU发送出来的DMA数据*/
    MSG_ID_CFM,        /*J1939传输层的应答*/
    MSG_ID_RESULT,     /*设置J1939传输层的返回结果 */
} J1939_MSG_TYPE;

/* mcu transmit layer config */
typedef struct
{
    uint8_t     sa_tbox;
    uint16_t    u16N_T1;
    uint16_t    u16N_T2;
    uint16_t    u16N_T3;
    uint16_t    u16N_T4;
    uint16_t    u16N_Th;
    uint16_t    u16N_Tr;
} J1939_TL_CFG;


/* Vehi-Fault time structure */
typedef struct
{
    unsigned int sec: 6;
    unsigned int min: 6;
    unsigned int hour: 5;
    unsigned int day: 5;
    unsigned int mon: 4;
    unsigned int year: 6;
} VEHI_FAULT_TIME_T;

/* DTC structure */
typedef struct
{
    unsigned int  spn;
    unsigned char fmi;
    unsigned char oc : 7;
    unsigned char cm : 1;
    unsigned char sa;
    unsigned char port;
} DTC_T;

/* Vehi-Fault Recorder structure */
typedef struct
{
    unsigned char status;      /* VehiFault status */
    unsigned char startTime[5];
    unsigned char endTime[5];
    unsigned long long update_time;
    DTC_T    dtc;
} VEHI_FAULT_DATA_T;

#define J1939_DM1_MAX_REG   4

typedef int (*J1939_fault_notify)(VEHI_FAULT_DATA_T *dtc);
typedef struct J1939_DM1_REG_TBL
{
    unsigned char used_num;
    J1939_fault_notify changed[J1939_DM1_MAX_REG];
} J1939_DM1_REG_TBL;


typedef int (*J1939_dm2_notify)(uint8_t *result, uint16_t len);
typedef struct J1939_DM2_QUERY
{
    unsigned char da;
    unsigned char status;
    J1939_dm2_notify func;
} J1939_DM2_QUERY;

typedef struct
{
    unsigned short total_size;
    unsigned char  no;
    unsigned char  total_no;
    unsigned char  buff[256 * 7];
} DTC_MF_T;


int J1939_init(INIT_PHASE phase);
void J1939_timeout(J1939_TIMER_E timer_id);
int J1939_fault_save(void);

void J1939_msg_decode(uint32_t canid, uint8_t port, uint8_t *data, uint8_t dlc);

uint32_t J1939_dm1_register(J1939_fault_notify callback);
uint32_t J1939_dm2_request(uint8_t port, uint8_t da, J1939_dm2_notify callback);

int J1939_shell_dump_vhflt(unsigned int argc, unsigned char **argv);


#endif
