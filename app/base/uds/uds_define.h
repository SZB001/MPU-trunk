#ifndef UDS_DEFINE_H
#define UDS_DEFINE_H

#include "timer.h"
#include "com_app_def.h"
#include "uds_timer.h"

#define  NULL_PTR           (void*) 0


#define  UDS_TYPE_CLIENT    1
#define  UDS_TYPE_SERVER    2
#define  UDS_TYPE_REMOTEDIAG    3


#define  UDS_SERVER_MAX_BYTE    4096
#define  UDS_FF_PUD_DLC_MAX     (6)
#define  UDS_SF_PDU_DLC_MAX     (7)
#define  CAN_DLC                (8)

#define     PhysicsAdd_Mask         0x01
#define     FunctionAdd_Mask        0x02
#define     DSC_STD_Mask            0x04
#define     DSC_EXT_Mask            0x08
#define     DSC_PRO_Mask            0x10

#define     SAL2_Mask               0x20
#define     SAL1_Mask               0x40
#define     SAL0_Mask               0x80


#define DIAGNOSE_MASK_SESSION_NUM       3u
#define DIAGNOSE_MAX_SERVICE            16u
#define MAX_INDEX_MASK                  80u


//physical address or functional address
#define PHYSICAL_ADDRESS            0u
#define FUNCTIONAL_ADDRESS          1u

// Suppress Positive Response Message Indication Bit
#define SPRMIB                          (0x80)

#define SESSION_TYPE_DEFAULT            (1)
#define SESSION_TYPE_PROGRAM            (2)
#define SESSION_TYPE_EXTENDED           (3)
#define SESSION_TYPE_UDSTEST            (0xAA)

#define SecurityAccess_LEVEL0           (0)
#define SecurityAccess_LEVEL1           (1)
#define SecurityAccess_LEVEL2           (2)

#define  POS_RESPOND_SID_MASK                    (0x40)

#define  SID_DiagnosticSeesionControl            (0x10)
#define  SID_ECUReset                            (0x11)
#define  SID_SecurityAccess                      (0x27)
#define  SID_CommunicationControl                (0x28)
#define  SID_TestPresent                         (0x3E)
#define  SID_AccessTimingParameter               (0x83)
#define  SID_SecuredDataTransmission             (0x84)
#define  SID_ControlDTCSetting                   (0x85)
#define  SID_ResponseOnEvent                     (0x86)
#define  SID_LinkControl                         (0x87)
#define  SID_ReadDataByIndentifier               (0x22)
#define  SID_ReadMemoryByAddress                 (0x23)
#define  SID_ReadScalingDataByIdentifier         (0x24)
#define  SID_ReadDataByPeriodicldentifier        (0x2A)
#define  SID_DynamicallyDefineDataIdentifier     (0x2C)
#define  SID_WriteDataByIdentifier               (0x2E)
#define  SID_WriteMemoryByAddress                (0x3D)
#define  SID_ClearDiagnosticInformation          (0x14)
#define  SID_ReadDTCInformation                  (0x19)
#define  SID_InputOutputControlByIdentifier      (0x2F)
#define  SID_RoutineControl                      (0x31)
#define  SID_RequestDownload                     (0x34)
#define  SID_RequestUpload                       (0x35)
#define  SID_TransferData                        (0x36)
#define  SID_RequestTransferExit                 (0x37)

#define  SID_NegativeResponse                    (0x7F)
#define  suppressPosRspMsgIndicationBit          (0x80)
#define  suppressPosRspMsgIndicationBitMask      (0x7F)

#define NRC_GeneralReject                                (0x10)
#define NRC_ServiceNotSupported                          (0x11)
#define NRC_SubFuncationNotSupported                     (0x12)
#define NRC_IncorrectMessageLengthOrInvailFormat         (0x13)
#define NRC_ResponseTooLong                              (0x14)
#define NRC_BusyRepeatRequest                            (0x21)
#define NRC_ConditionsNotCorrect                         (0x22)
#define NRC_RequstSequenceError                          (0x24)
#define NRC_NoResponseFromSubnetComponent                (0x25)
#define NRC_FailurePreventsExecutionOfRequestedAction    (0x26)
#define NRC_RequestOutOfRange                            (0x31)
#define NRC_SecurityAccessDenied                         (0x33)
#define NRC_InvalidKey                                   (0x35)
#define NRC_ExceedNumberOfAttempts                       (0x36)
#define NRC_RequiredTimeDelayNotExpired                  (0x37)
#define NRC_UploadDownloadNotAccepted                    (0x70)
#define NRC_TransferDataSuspended                        (0x71)
#define NRC_GeneralProgrammingFailure                    (0x72)
#define NRC_wrongBlockSequenceCounter                    (0x73)
#define NRC_RequestCorrectlyReceivedResponsePending      (0x78)
#define NRC_SubFunctionNotSupportedInActiveSession       (0x7E)
#define NRC_ServiceNotSupportedInActiveSession           (0x7F)

/*****************************************************************************
*   Global Data Declarations
*****************************************************************************/
extern uint8_t                          g_u8CurrentSessionType;
extern uint8_t                          g_u8SecurityAccess;
extern uint8_t                          g_u8suppressPosRspMsgIndicationFlag;
extern uint32_t                         g_u32DiagID;

#define Get_Session_Current()           g_u8CurrentSessionType
#define Set_Seesion_Default()           {g_u8CurrentSessionType = SESSION_TYPE_DEFAULT ;}while(0)
#define Set_Seesion_Program()           {g_u8CurrentSessionType = SESSION_TYPE_PROGRAM ;}while(0)
#define Set_Seesion_Extend()            {g_u8CurrentSessionType = SESSION_TYPE_EXTENDED;}while(0)

#define Get_SecurityAccess()            g_u8SecurityAccess
#define Clear_SecurityAccess()          {g_u8SecurityAccess = SecurityAccess_LEVEL0 ;} while(0)
#define Set_SecurityAccess_LEVEL0()     {g_u8SecurityAccess = SecurityAccess_LEVEL0 ;} while(0)
#define Set_SecurityAccess_LEVEL1()     {g_u8SecurityAccess = SecurityAccess_LEVEL1 ;} while(0)
#define Set_SecurityAccess_LEVEL2()     {g_u8SecurityAccess = SecurityAccess_LEVEL2 ;} while(0)


/********************timer function*********************/
typedef enum
{
    P2SERVER = 0,
    P2CLINT,
    P2EXT_SERVER,
    P2EXT_CLIENT,
    S3SERVER,
    S3CLIENT,
    SERVER_SET,
    CLIENT_SET,
    TIMER_MAX,
} UDS_TIMER_E;

/**********spi data between mcu and mpu- MSG ID*********/
typedef enum
{
    MSG_ID_UDS_SET_S = 0xF1, /*mpu设置mcu传输层*/
    MSG_ID_UDS_SET_C,
    MSG_ID_UDS_REQ,          /*mpu发送数据到mcu传输层*/
    MSG_ID_UDS_IND,          /*mcu接收数据上传mpu应用层*/
    MSG_ID_UDS_CFM,          /* 传输层应答 */
    MSG_ID_UDS_UPG,          /*mpu传输刷写数据*/
    MSG_ID_UDS_S_ACK,
    MSG_ID_UDS_C_ACK,
    MSG_ID_UDS_TIMER,
} UDS_TL_ID;

/*********************uds result***********************/
typedef enum
{
    N_RESULT_NULL,
    N_OK,
    N_TIMEOUT_A,
    N_TIMEOUT_Bs,
    N_TIMEOUT_Cr,
    N_WRONG_SN,
    N_INVALID_FS,
    N_UNEXP_PDU,
    N_WFT_OVRN,
    N_BUFFER_OVFLW,
    N_ERROR,
    N_DRIVER_ERR,
} UDS_RESULT_E;

typedef struct
{
#if 0    
    timer_t    timer_fd;
#else
    uds_timer_t timer_fd;
#endif
    uint8_t    timer_switch;
    uint32_t   timer_value;
} UDS_TIMER_T;


typedef struct UDS_T
{
    uint8_t     mode;
    uint8_t     canport;
    uint8_t     sid ;
    uint8_t     nrc ;
    uint32_t    can_id_phy;// (本地物理地址，设备被诊断时使用)
    uint32_t    can_id_fun;// (本地功能地址，设备被诊断时使用)
    uint32_t    can_id_req;// (本地请求地址，刷写ECU时使用)
    uint32_t    can_id_res;// (本地响应地址，设备被诊断时使用)
    UDS_TIMER_T timer_t[TIMER_MAX];
} UDS_T;

#pragma pack(1)
typedef struct UDS_TL_CFG_T
{
    uint8_t     canport;
    uint8_t     fill_value;
    uint8_t     fc_stmin;
    uint8_t     fc_bs;
    uint16_t    n_bs;
    uint16_t    n_cr;
    uint32_t    can_id_phy;
    uint32_t    can_id_fun;
    uint32_t    can_id_res;
} UDS_TL_CFG_T;
#pragma pack(0)

#endif

