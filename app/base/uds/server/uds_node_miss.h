#ifndef __UDS_NODE_MISS_H__
#define __UDS_NODE_MISS_H__

#define CAN_NODE_MISS_MAX_SIGNAL        (1024)

typedef enum GDSE_MSG_ID
{
    CAN_NODE_MISS_TEST_TIMER = MPU_MID_CAN_NODE_MISS,
} GDSE_MSG_ID;


#define CAN_NODE_MISS_TEST_INTERVAL     (50)

#define CAN_NODE_MISS_CONFIRMED_TIME    (1000)

typedef enum
{
    CAN_NODE_MISS_FLAG_NORMAL = 0,
    CAN_NODE_MISS_FLAG_MISS = 1,
} CAN_NODE_MISS_STATE;

typedef enum DTC_CAN_NODE_MISS_ID
{
    CAN_NODE_MISS_ITEM_INVALID = 0,
    CAN_NODE_MISS_ITEM_ACU,
    CAN_NODE_MISS_ITEM_BMS,
    CAN_NODE_MISS_ITEM_CDU,
    CAN_NODE_MISS_ITEM_MCU,
    CAN_NODE_MISS_ITEM_VCU1,
    CAN_NODE_MISS_ITEM_EPS,
    CAN_NODE_MISS_ITEM_ESC,
    CAN_NODE_MISS_ITEM_EHB,
    CAN_NODE_MISS_ITEM_EACP,
    CAN_NODE_MISS_ITEM_PTC,
    CAN_NODE_MISS_ITEM_PLG,
    CAN_NODE_MISS_ITEM_CLM,
    CAN_NODE_MISS_ITEM_BDCM,
    CAN_NODE_MISS_ITEM_ALM,
    CAN_NODE_MISS_ITEM_ICU,
    CAN_NODE_MISS_ITEM_IHU,
    CAN_NODE_MISS_ITEM_TAP,
    CAN_NODE_MISS_ITEM_NUM,
} DTC_CAN_NODE_MISS_ID;

typedef enum
{
    VEHI_STATE_OFF = 0,
    VEHI_STATE_ACC,
    VEHI_STATE_ON,
    VEHI_STATE_START,
} VEHI_STATE_T;

int uds_node_miss_init(INIT_PHASE phase);
int uds_node_miss_run(void);
int get_can_node_miss_state(int can_node_miss_id);
int set_node_sig_time(int node_id, long long sig_time);
int set_can_node_miss_state(int can_node_miss_id, CAN_NODE_MISS_STATE miss_state);

#endif //__UDS_NODE_MISS_H__

