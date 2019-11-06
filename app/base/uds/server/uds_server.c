#include "uds_define.h"
#include "uds_server.h"
#include "uds_request.h"

uint8_t   g_u8CurrentSessionType = SESSION_TYPE_DEFAULT ;
uint8_t   g_u8SecurityAccess = SecurityAccess_LEVEL0 ;

uint8_t   g_u8suppressPosRspMsgIndicationFlag = 0;
uint32_t  g_u32DiagID;


/*************************************************************************************************/
void UDS_SRV_WriteDataByID(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC);
void uds_service_not_support(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_dlc);
void uds_diagnostic_session_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_ecu_reset(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_clear_diagnostic_information(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_read_dtc_information(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_read_data_by_identifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_security_access(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_communication_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_write_data_by_identifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_IOcontrol_by_indentifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_rountine_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_test_present(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_control_dtc_setting(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_request_download(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_transfer_data(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
void uds_request_transfer_exit(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len);
/***********************************************************************************************/

const uint8_t uds_index_mask[MAX_INDEX_MASK] =
{
  /*x0  x1  x2  x3  x4  x5  x6  x7  x8  x9  xA  xB  xC  xD  xE   xF           */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   0,     /* 0x */
    1,  2,  0,  0,  3,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,   0,     /* 1x */
    0,  0,  5,  0,  0,  0,  0,  6,  7,  0,  0,  0,  0,  0,  8,   0,     /* 2x */
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  11,  0,     /* 3x */
    0,  0,  0,  0,  0,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,   0      /* 8x */
};

const uint8_t server_supper_table[MAX_INDEX_MASK] =
{
    /* x0    x1    x2    x3    x4    x5    x6    x7    x8    x9    xA    xB    xC    xD    xE    xF           */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 0x */
    0xFF, 0xFB, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0xED, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 1x */
    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xE9, 0xEB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00,   /* 2x */
    0x00, 0xF9, 0x00, 0x00, 0x31, 0x00, 0x31, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,   /* 3x */
    0x00, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   /* 8x */
};

const  uds_server_func  uds_appl_table[MAX_INDEX_MASK] =
{
    &uds_service_not_support,                         /*   xx -> 0  */
    &uds_diagnostic_session_control,                    /*   10 -> 1  */
    &uds_ecu_reset,                                    /*   11 -> 2  */
    &uds_clear_diagnostic_information,                  /*   14 -> 3  */
    &uds_read_dtc_information,                          /*   19 -> 4  */
    &uds_read_data_by_identifier,                        /*   22 -> 5  */
    &uds_security_access,                              /*   27 -> 6  */
    &uds_communication_control,                        /*   28 -> 7  */
    &uds_write_data_by_identifier,                       /*   2E -> 8  */
    &uds_IOcontrol_by_indentifier,                      /*   2F -> 9  */
    &uds_rountine_control,                             /*   31 -> 10 */
    &uds_test_present,                                 /*   3E -> 11 */
    &uds_control_dtc_setting,                           /*   85 -> 12 */
    &uds_request_download,                             /*   34 -> 13 */
    &uds_transfer_data,                                /*   36 -> 14 */
    &uds_request_transfer_exit,                         /*   37 -> 15 */
};

/*****************************************************************************
*   Function:     uds_get_table_index
*   Description:
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
uint8_t   uds_get_table_index(uint8_t  sid)
{
    uint8_t index = 79;
    uint8_t h4bit = sid & 0xF0;

    if ((h4bit == 0x00) || (h4bit == 0x10) || (h4bit == 0x20) || (h4bit == 0x30))
    {
        index = h4bit + (sid & 0x0F);
    }
    else if (h4bit == 0x80)
    {
        index = 0x40 + (sid & 0x0F);
    }

    return index;
}

/*****************************************************************************
*   Function:     uds_service_not_support
*   Description:  UDS negtive type
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_service_not_support(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_dlc)
{
    uds_negative_response(uds, pdu_data[0], NRC_ServiceNotSupported);
}

/*****************************************************************************
*   Function:     uds_diagnostic_session_control
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_diagnostic_session_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_DiagSessionCtrl(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     UDS_ECUReset__
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_ecu_reset(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_EcuReset(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_clear_diagnostic_information
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_clear_diagnostic_information(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_ClearDTC(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_read_dtc_information
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_read_dtc_information(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_ReadDTC(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_read_data_by_identifier
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_read_data_by_identifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_ReadDataByID(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_security_access
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_security_access(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_SecrityAcess(uds , pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_communication_control
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_communication_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_CommunicationControl(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     uds_write_data_by_identifier
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_write_data_by_identifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_WriteDataByID(uds, pdu_data, pdu_len);
}

/*****************************************************************************
*   Function:     UDS_IOControlByIndentifier__
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_IOcontrol_by_indentifier(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{

}

/*****************************************************************************
*   Function:     UDS_RountineControl__
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_rountine_control(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_RoutineControl(uds, pdu_data, pdu_len); //added by Cindy 20180322
}

/*****************************************************************************
*   Function:     uds_test_present
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_test_present(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_TesterPresent(uds, pdu_data, pdu_len);  //added by CINDY 20180322
}

/*****************************************************************************
*   Function:     uds_control_dtc_setting
*   Description:  UDS service
*   Inputs:       None
*   Outputs:      None
*   Notes:
*****************************************************************************/
void uds_control_dtc_setting(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{
    UDS_SRV_ControlDTCSetting(uds, pdu_data, pdu_len);  //added by Cindy
}


void uds_request_download(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{

}


void uds_transfer_data(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{

}

void uds_request_transfer_exit(UDS_T *uds, uint8_t *pdu_data, uint16_t pdu_len)
{

}

/****************************************************************
function:     uds_server_proc
description:  the server message handle
input:        UDS_T *uds,
              unsigned int can_id,
              unsigned char* pdu_data,
              unsigned short lenth
output:       none
return:       NULL
****************************************************************/
void uds_server_proc(UDS_T *uds, unsigned int can_id, unsigned char *pdu_data, unsigned short lenth)
{
    uint8_t   sid_index = 0;
    uint8_t   table_index  = 0;
    uint8_t   support_mask = 0;

    table_index = uds_get_table_index(pdu_data[0]);
    sid_index = uds_index_mask[table_index];

    g_u32DiagID = can_id;

    if (sid_index == 0)
    {
        uds_negative_response(uds, pdu_data[0], NRC_ServiceNotSupported);
        return ;
    }

    if (uds->can_id_phy == can_id)
    {
        support_mask = server_supper_table[table_index] & PhysicsAdd_Mask;
    }
    else
    {
        support_mask = server_supper_table[table_index] & FunctionAdd_Mask;
    }

    if (support_mask > 0)
    {
        switch (g_u8CurrentSessionType)
        {
            case SESSION_TYPE_DEFAULT :
                support_mask = server_supper_table[table_index] & DSC_STD_Mask ;
                break;

            case SESSION_TYPE_PROGRAM :
                support_mask = server_supper_table[table_index] & DSC_PRO_Mask  ;
                break;

            case SESSION_TYPE_EXTENDED :
                support_mask = server_supper_table[table_index] & DSC_EXT_Mask ;
                break;
        }
    }
    else
    {
        //uds_negative_response(uds, pdu_data[0], NRC_ConditionsNotCorrect);
        return ;
    }

    if (support_mask > 0)
    {
        switch (g_u8SecurityAccess)
        {
            case SecurityAccess_LEVEL0 :
                support_mask = server_supper_table[table_index] & SAL0_Mask ;
                break;

            case SecurityAccess_LEVEL1 :
                support_mask = server_supper_table[table_index] & SAL1_Mask ;
                break;

            case SecurityAccess_LEVEL2 :
                support_mask = server_supper_table[table_index] & SAL2_Mask ;
                break;
        }
    }
    else
    {
        uds_negative_response(uds, pdu_data[0], NRC_ServiceNotSupportedInActiveSession);
        return ;
    }

    if (support_mask > 0)
    {
        if (sid_index < DIAGNOSE_MAX_SERVICE)
        {
            (*uds_appl_table[sid_index])(uds, pdu_data, lenth);
        }
        else
        {
            uds_negative_response(uds , pdu_data[0], NRC_ServiceNotSupported);
        }
    }
    else
    {
        uds_negative_response(uds , pdu_data[0], NRC_SecurityAccessDenied);
    }
}

