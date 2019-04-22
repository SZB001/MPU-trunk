/*****************************************************************************
*   Include Files
*****************************************************************************/
#include  <stdlib.h>
#include  <string.h>
#include  "uds_diag.h"
#include "uds_request.h"
#include "uds_server.h"

#define  reportNumberOfDTCByStatusMask              (0x01)
#define  reportDTCByStatusMask                      (0x02)
#define  reportDTCSnapshotIdentification            (0x03)
#define  reportDTCSnapshotRecordByDTCNumber         (0x04)
#define  readSupportedDTCs                          (0x0A)
#define  reportMirrorMemoryDTCByStatusMask          (0x0F)

#define  statusOfDTC_testfail                                   (1<<0)
#define  statusOfDTC_testFailedThisOperationCycle               (1<<1)
#define  statusOfDTC_pendingDTC                                 (1<<2)
#define  statusOfDTC_ConfirmedDTC                               (1<<3)
#define  statusOfDTC_testNotCompletedSinceLastClear             (1<<4)
#define  statusOfDTC_testFailedSinceLastClear                   (1<<5)
#define  statusOfDTC_testNotCompletedThisOperationCycle         (1<<6)
#define  statusOfDTC_warningIndicatorRequested                  (1<<7)

static unsigned int DTC_index[DIAG_ITEM_NUM] = { 0 } ;

static uint32_t ChangeDTCtoInt(uint8_t *dtcchar)
{
    uint8_t tempbuf[20];
    uint32_t result;

    memset(tempbuf, 0x00, sizeof(tempbuf));
    strcat((char *)tempbuf, "0x");
    strcat((char *)tempbuf, (const char *)dtcchar);
    result = strtoul((const char *)tempbuf, 0, 16);

    return result;

}

static void ReportNumberOfDTCByStatusMask(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[6];
    uint16_t DTCCounter = 0;

    if (u16PDU_DLC != 3)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    if ((p_u8PDU_Data[2] & (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC)) ==
        (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC))
    {
        DTCCounter =  uds_diag_get_dtc_num();
    }
    else if (p_u8PDU_Data[2] & statusOfDTC_testfail)
    {
        DTCCounter =  uds_diag_get_uncfm_dtc_num();
    }
    else if (p_u8PDU_Data[2] & statusOfDTC_ConfirmedDTC)
    {
        DTCCounter =  uds_diag_get_cfm_dtc_num();
    }

    Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[2] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);
    Ar_u8RePDU_DATA[3] =  0x00; //ISO15031-6DTCFormat
    Ar_u8RePDU_DATA[4] = (uint8_t)(DTCCounter >> 8);  //DTCCountHighByte
    Ar_u8RePDU_DATA[5] = (uint8_t)DTCCounter;  //DTCCountLowByte

    uds_positive_response(tUDS, tUDS->can_id_res, 6, Ar_u8RePDU_DATA);
}

static void ReportDTC_testfail_Or_Confirmed(UDS_T *tUDS, uint16_t u16PDU_DLC ,
        uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200];
    uint32_t DTC, StatusOfDTC;
    uint32_t i = 0, j;

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[i++] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);

    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        if (uds_diag_is_cfm_dtc_valid(j) || uds_diag_is_uncfm_dtc_valid(j))
        {
            DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 16);
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 8);
            Ar_u8RePDU_DATA[i++] = (uint8_t)DTC;

            StatusOfDTC = 0;

            if (uds_diag_is_cfm_dtc_valid(j))
            {
                StatusOfDTC |= statusOfDTC_ConfirmedDTC;
            }

            if (uds_diag_is_uncfm_dtc_valid(j))
            {
                StatusOfDTC |= statusOfDTC_testfail;
            }

            Ar_u8RePDU_DATA[i++] = StatusOfDTC;
        }
    }

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

static void ReportDTC_testfail(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200];
    uint32_t DTC, StatusOfDTC;
    uint32_t i = 0, j;

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[i++] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);

    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        if (uds_diag_is_uncfm_dtc_valid(j))
        {
            DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 16);
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 8);
            Ar_u8RePDU_DATA[i++] = (uint8_t)DTC;

            StatusOfDTC = 0;
            StatusOfDTC |= statusOfDTC_testfail;

            if (uds_diag_is_cfm_dtc_valid(j))
            {
                StatusOfDTC |= statusOfDTC_ConfirmedDTC;
            }

            Ar_u8RePDU_DATA[i++] = StatusOfDTC;
        }
    }

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

static void ReportDTC__Confirmed(UDS_T *tUDS, uint16_t u16PDU_DLC, uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200];
    uint32_t DTC, StatusOfDTC;
    uint32_t i = 0, j;

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[i++] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);


    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        if (uds_diag_is_cfm_dtc_valid(j))
        {
            DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 16);
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 8);
            Ar_u8RePDU_DATA[i++] = (uint8_t)DTC;

            StatusOfDTC = 0;
            StatusOfDTC |= statusOfDTC_ConfirmedDTC;

            if (uds_diag_is_uncfm_dtc_valid(j))
            {
                StatusOfDTC |= statusOfDTC_testfail;
            }

            Ar_u8RePDU_DATA[i++] = StatusOfDTC;
        }
    }

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

static void ReportDTCByStatusMask(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[3];

    if (u16PDU_DLC != 3)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    if (!(p_u8PDU_Data[2] & (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC)))
    {
        Ar_u8RePDU_DATA[0] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
        Ar_u8RePDU_DATA[1] =  p_u8PDU_Data[1] ;
        Ar_u8RePDU_DATA[2] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);
        uds_positive_response(tUDS, tUDS->can_id_res, 3, Ar_u8RePDU_DATA);
        return;
    }

    if ((p_u8PDU_Data[2] & (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC)) ==
        (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC))
    {
        ReportDTC_testfail_Or_Confirmed(tUDS, u16PDU_DLC, p_u8PDU_Data);
    }
    else if (p_u8PDU_Data[2] & statusOfDTC_testfail)
    {
        ReportDTC_testfail(tUDS, u16PDU_DLC, p_u8PDU_Data);
    }
    else if (p_u8PDU_Data[2] & statusOfDTC_ConfirmedDTC)
    {
        ReportDTC__Confirmed(tUDS, u16PDU_DLC, p_u8PDU_Data);
    }

}

static void GetDTCSnapshotRecordNumber(void)
{
    uint32_t i, start_index = 1;

    memset(DTC_index, 0xff, sizeof(DTC_index));

    for (i = 0; i < DIAG_ITEM_NUM; i++)
    {
        if (uds_diag_is_cfm_dtc_valid(i))
        {
            DTC_index[i] = start_index;
            start_index++;
        }
    }
}


static void ReportDTCSnapshotIdentification(UDS_T *tUDS, uint16_t u16PDU_DLC ,
        uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200];
    uint32_t DTC;
    uint32_t i = 0, j;

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;

    GetDTCSnapshotRecordNumber();

    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        if (uds_diag_is_cfm_dtc_valid(j))
        {
            DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 16);
            Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 8);
            Ar_u8RePDU_DATA[i++] = (uint8_t)DTC;
            Ar_u8RePDU_DATA[i++] = DTC_index[j];
        }
    }

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

static signed int DTCSnapshotRecordNumberAndDTCRecordIsValid
(uint32_t *number, uint32_t DTCRecord, uint8_t *DIDptr,
 uint32_t *len, uint32_t *DIDnumber)
{
    uint32_t DTC, j;
    signed int result = -1;

    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        if (uds_diag_is_cfm_dtc_valid(j))
        {
            DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));

            if ((DTC == DTCRecord) && ((DTC_index[j] == *number) || (*number == 0xFF)))
            {
                if (*number == 0xFF)
                {
                    GetDTCSnapshotRecordNumber();
                    *number = DTC_index[j];
                }

                uds_diag_get_freeze(j, DIDptr, len, DIDnumber);
                result = 0;
                break;
            }
        }
    }

    return result;
}

static void ReportDTCSnapshotRecordByDTCNumber(UDS_T *tUDS, uint16_t u16PDU_DLC,
        uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200], DIDBuf[200];
    uint32_t DTC, DTCSnapshotRecordNumber;
    uint32_t DIDnumber;
    uint32_t i = 0, len = 0;

    if (u16PDU_DLC != 6)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    DTC = ((uint32_t)p_u8PDU_Data[2] << 16) + ((uint32_t)p_u8PDU_Data[3] << 8) + p_u8PDU_Data[4];
    DTCSnapshotRecordNumber = p_u8PDU_Data[5];
    len = sizeof(DIDBuf);

    if (DTCSnapshotRecordNumberAndDTCRecordIsValid(&DTCSnapshotRecordNumber, DTC, DIDBuf, &len,
            &DIDnumber))
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
        return;
    }

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[2] ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[3] ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[4] ;
    Ar_u8RePDU_DATA[i++] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);
    Ar_u8RePDU_DATA[i++] =  DTCSnapshotRecordNumber ;
    Ar_u8RePDU_DATA[i++] =  DIDnumber ;
    memcpy(&Ar_u8RePDU_DATA[i], DIDBuf, len);
    i += len;

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

static void ReadSupportedDTCs(UDS_T *tUDS, uint16_t u16PDU_DLC , uint8_t *p_u8PDU_Data)
{
    uint8_t  Ar_u8RePDU_DATA[200];
    uint32_t DTC, StatusOfDTC;
    uint32_t i = 0, j;

    if (u16PDU_DLC != 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[0] + POS_RESPOND_SID_MASK ;
    Ar_u8RePDU_DATA[i++] =  p_u8PDU_Data[1] ;
    Ar_u8RePDU_DATA[i++] = (statusOfDTC_testfail | statusOfDTC_ConfirmedDTC);

    for (j = 0; j < DIAG_ITEM_NUM; j++)
    {
        DTC = ChangeDTCtoInt(uds_diag_get_dtc(j));
        Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 16);
        Ar_u8RePDU_DATA[i++] = (uint8_t)(DTC >> 8);
        Ar_u8RePDU_DATA[i++] = (uint8_t)DTC;

        StatusOfDTC = 0;

        if (uds_diag_is_cfm_dtc_valid(j))
        {
            StatusOfDTC |= statusOfDTC_ConfirmedDTC;
        }

        if (uds_diag_is_uncfm_dtc_valid(j))
        {
            StatusOfDTC |= statusOfDTC_testfail;
        }

        Ar_u8RePDU_DATA[i++] = StatusOfDTC;
    }

    uds_positive_response(tUDS, tUDS->can_id_res, i, Ar_u8RePDU_DATA);
}

/*****************************************************************************
*   Function   :    UDS_SRV_ClearDTC
*   Description:
*   Inputs     :    None
*   Outputs    :    NULL
*   Notes      :
*****************************************************************************/
void UDS_SRV_ReadDTC(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    if (u16PDU_DLC < 2)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
        return;
    }

    switch (p_u8PDU_Data[1])
    {
        case reportNumberOfDTCByStatusMask:
            ReportNumberOfDTCByStatusMask(tUDS, u16PDU_DLC, p_u8PDU_Data);
            break;

        case reportDTCByStatusMask:
            ReportDTCByStatusMask(tUDS, u16PDU_DLC, p_u8PDU_Data);
            break;

        case reportDTCSnapshotIdentification:
            ReportDTCSnapshotIdentification(tUDS, u16PDU_DLC, p_u8PDU_Data);
            break;

        case reportDTCSnapshotRecordByDTCNumber:
            ReportDTCSnapshotRecordByDTCNumber(tUDS, u16PDU_DLC, p_u8PDU_Data);
            break;

        case readSupportedDTCs:
            ReadSupportedDTCs(tUDS, u16PDU_DLC, p_u8PDU_Data);
            break;

        default :
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
            break;
    }

}



