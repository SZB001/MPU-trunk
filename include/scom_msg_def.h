#ifndef __SCOM_MSG_DEF_H__
#define __SCOM_MSG_DEF_H__

typedef enum
{
    SCOM_TL_CHN_MGR = 0,
    SCOM_TL_CHN_MCU_UPG,   /* reserved */
    SCOM_TL_CHN_ECU_UPG,
    SCOM_TL_CHN_DEV,
    SCOM_TL_CHN_PM,
    SCOM_TL_CHN_CAN,
    SCOM_TL_CHN_FCT,
    SCOM_TL_CHN_UDS,
    SCOM_TL_CHN_CFG,
    SCOM_TL_CHN_SHELL,
    SCOM_TL_CHN_NUM,
} SCOM_TL_CHAN;

/* spi msg about MGR */
typedef enum
{
    SCOM_TL_CMD_CON_REQ = (SCOM_TL_CHN_MGR << 4),
    SCOM_TL_CMD_CON_RES,

    SCOM_TL_DATA_REQ,
    SCOM_TL_DATA_OK_RES,
    SCOM_TL_DATA_NG_RES,

    SCOM_TL_CMD_RESERVED1,        /* reserved1 */
    SCOM_TL_CMD_RESERVED2,        /* reserved2 */

    SCOM_TL_CMD_RESERVED3,

    SCOM_TL_CMD_MCU_BTL_VER,
    SCOM_TL_CMD_UPG_MCU_REQ,
    SCOM_TL_CMD_UPG_MCU_RES,
    SCOM_TL_CMD_UPG_MCU_FIN,
    SCOM_TL_CMD_UPG_MCU_FIN_RES,
    SCOM_TL_CMD_UPG_MCU_VER,

    SCOM_TL_CMD_HEARTBEAT,
} SCOM_TL_MGR_TYPE;

/* spi msg about UDS */
typedef enum
{
    SCOM_TL_CMD_UDS_MSG = (SCOM_TL_CHN_UDS << 4),
    SCOM_TL_CMD_UDS_CFG,
} SCOM_TL_UDS_TYPE;

/* spi msg about DEV */
typedef enum
{
    SCOM_TL_CMD_MCU_FAULT_SYN = (SCOM_TL_CHN_DEV << 4),
    SCOM_TL_CMD_MCU_STATUS_SYN,
    SCOM_TL_CMD_PARA_REPORT,
    SCOM_TL_CMD_SET_TIME,
    SCOM_TL_CMD_GET_TIME,
    SCOM_TL_CMD_REPORT_TIME,
    SCOM_TL_CMD_MCU_CFG_SYN,
    SCOM_TL_CMD_CUSTOM_PARA,
    SCOM_TL_CMD_MCU_SHELL,
    SCOM_TL_CMD_MPU_STATUS_SYN,
    SCOM_TL_CMD_MPU_FAULT_SYN,
    SCOM_TL_CMD_PARA_CFG,
} SCOM_TL_DEV_TYPE;

/* spi msg about PM */
typedef enum
{
    SCOM_TL_CMD_SLEEP_REQ = (SCOM_TL_CHN_PM << 4),
    SCOM_TL_CMD_WAKE_TIME,
} SCOM_TL_PM_TYPE;

/* spi msg about CAN */
typedef enum
{
    SCOM_TL_CMD_CAN_BAUD = (SCOM_TL_CHN_CAN << 4),
    SCOM_TL_CMD_CAN_DEFAULT_BAUD,
    SCOM_TL_CMD_CAN_SEND_CFG,
    SCOM_TL_CMD_CAN_AUTO_BAUD,
    SCOM_TL_CMD_CAN_FILER_CANID,
    SCOM_TL_CMD_CAN_SEND,
}SCOM_TL_CAN_TYPE;

/* spi msg about FCT */
typedef enum
{
    SCOM_TL_CMD_FCT = (SCOM_TL_CHN_FCT << 4),
    SCOM_TL_CMD_FCT_LOG,
} SCOM_TL_FCT_TYPE;

#endif
