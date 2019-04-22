#include "uds_request.h"
#include "uds_server.h"
#include  "uds_file.h"


//RoutineControl Type
#define   StartRoutine  0x01

//RoutineIdentifier
#define   CheckRoutine                     0x0202
#define   CheckProgrammingPreconditions    0x0203
#define   EraseMemory                      0xFF00
#define   CheckProgrammingDependencies     0xFF01
#define   CheckLeanTuKey                   0xA400

static unsigned int erasememoryflag;
extern void ap_do_report_request_tukey_qet(void);
extern unsigned char tu_key_check_flag_finish;

void UDS_SRV_RoutineControl(UDS_T *tUDS, uint8_t *p_u8PDU_Data, uint16_t u16PDU_DLC)
{
    uint8_t  Ar_u8RePDU_DATA[6], i;
    uint32_t funvalue = 0;
    uint8_t memoryAddressLength, memorySizeLength;
    extern unsigned int sdNewFile(unsigned char *fname, unsigned char *buf, unsigned int len);

    if (u16PDU_DLC < 4)
    {
        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_IncorrectMessageLengthOrInvailFormat);
    }
    else
    {
        if (p_u8PDU_Data[1] == StartRoutine)
        {
            funvalue = (p_u8PDU_Data[2] << 8) + p_u8PDU_Data[3];

            switch (funvalue)
            {
                case CheckRoutine:
                    Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                    Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                    Ar_u8RePDU_DATA[2] = p_u8PDU_Data[2];
                    Ar_u8RePDU_DATA[3] = p_u8PDU_Data[3];

                    #if 0
                    crcchecksum1 = (p_u8PDU_Data[4] << 24) + (p_u8PDU_Data[5] << 16) +
                                   (p_u8PDU_Data[6] << 8) + p_u8PDU_Data[7];

                    printk(PRINTK_LEVEL_LOG, "crc32 start ->uptime = %llu", driverGetUptime());
                    crcchecksum2 = crc32(0L, TransferDataConfig.data, TransferDataConfig.datasize);
                    printk(PRINTK_LEVEL_LOG, "crc32 finish->uptime = %llu", driverGetUptime());
                    printk(PRINTK_LEVEL_LOG, "crcchecksum1=0x%x,crcchecksum2=0x%x\r\n", crcchecksum1, crcchecksum2);

                    if (crcchecksum1 == (~crcchecksum2))
                    {
                        Ar_u8RePDU_DATA[4] = 0;
                        printk(PRINTK_LEVEL_LOG,
                               "TransferDataConfig.ProgrammemorySize=0x%x,TransferDataConfig.datasize=0x%x\r\n",
                               TransferDataConfig.ProgrammemorySize, TransferDataConfig.datasize);
                        printk(PRINTK_LEVEL_LOG, "TransferDataConfig.ProgrammemoryAddress=0x%x\r\n",
                               TransferDataConfig.ProgrammemoryAddress);

                        if (TransferDataConfig.ProgrammemorySize == TransferDataConfig.datasize)
                        {
                            //sdNewFile("/ecu/data.bin",TransferDataConfig.data,TransferDataConfig.datasize);
                            if ((TransferDataConfig.ProgrammemoryAddress == 0x10000000) && (erasememoryflag == 1))
                            {
                                //memcpy(Image.data,TransferDataConfig.data,TransferDataConfig.datasize);
                                //Image.pos = TransferDataConfig.datasize;
                                //RTData.imgUpgrade = 1;
                                UDS_NegativeResponse(tUDS, p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);
                                printk(PRINTK_LEVEL_ERROR, "Rcv DBC Success\r\n");
                                printk(PRINTK_LEVEL_LOG, "TerminalUpdatesCheck start ->uptime = %llu", driverGetUptime());

                                if (TerminalUpdatesCheck(TransferDataConfig.data, TransferDataConfig.datasize) == 0)
                                {
                                    printk(PRINTK_LEVEL_LOG, "loading dbc pkg.....\r\n");
                                    printk(PRINTK_LEVEL_LOG, "FlashWritePGK start ->uptime = %llu", driverGetUptime());
                                    UDS_NegativeResponse(tUDS, p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);

                                    if (!FlashWritePGK(TransferDataConfig.data, TransferDataConfig.datasize))
                                    {
                                        UDS_NegativeResponse(tUDS, p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);
                                        driverSetStartTag(0);
                                        printk(PRINTK_LEVEL_LOG, "FlashWritePGK finish ->uptime = %llu", driverGetUptime());
                                        //                                        UDS_NegativeResponse(tUDS,p_u8PDU_Data[0],NRC_RequestCorrectlyReceivedResponsePending);
                                        //                                        printk(PRINTK_LEVEL_LOG, "startTagCheck start ->uptime = %llu", driverGetUptime());
                                        //                                        startTagCheck();
                                        //                                        printk(PRINTK_LEVEL_LOG, "startTagCheck finish ->uptime = %llu", driverGetUptime());
                                    }
                                }
                            }
                            else if (TransferDataConfig.ProgrammemoryAddress == 0)
                            {
                                erasememoryflag = 1;
                            }
                        }

                    }
                    else
                    {
                        Ar_u8RePDU_DATA[4] = 1;
                    }

                    #endif
                    memset(&TransferDataConfig, 0x00, sizeof(TransferDataConfig));
                    uds_positive_response(tUDS, tUDS->can_id_res, 5, Ar_u8RePDU_DATA);
                    break;

                case CheckProgrammingPreconditions:
                    Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                    Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                    Ar_u8RePDU_DATA[2] = p_u8PDU_Data[2];
                    Ar_u8RePDU_DATA[3] = p_u8PDU_Data[3];
                    uds_positive_response(tUDS, tUDS->can_id_res, 4, Ar_u8RePDU_DATA);
                    break;

                case EraseMemory:
                    if (erasememoryflag == 0)
                    {
                        uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_GeneralProgrammingFailure);
                    }
                    else
                    {
                        memoryAddressLength = p_u8PDU_Data[4] & 0x0f;
                        memorySizeLength = (p_u8PDU_Data[4] >> 4) & 0x0f;

                        for (i = 0; i < memoryAddressLength; i++)
                        {
                            TransferDataConfig.ErasememoryAddress = (TransferDataConfig.ErasememoryAddress << (i * 8));
                            TransferDataConfig.ErasememoryAddress |= p_u8PDU_Data[5 + i];
                        }

                        for (i = 0; i < memorySizeLength; i++)
                        {
                            TransferDataConfig.ErasememorySize = (TransferDataConfig.ErasememorySize << (i * 8));
                            TransferDataConfig.ErasememorySize |= p_u8PDU_Data[5 + memoryAddressLength + i];
                        }

                        Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                        Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                        Ar_u8RePDU_DATA[2] = p_u8PDU_Data[2];
                        Ar_u8RePDU_DATA[3] = p_u8PDU_Data[3];
                        Ar_u8RePDU_DATA[4] = 0;
                        uds_positive_response(tUDS, tUDS->can_id_res, 5, Ar_u8RePDU_DATA);
                    }

                    break;

                case CheckProgrammingDependencies:
                    Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                    Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                    Ar_u8RePDU_DATA[2] = p_u8PDU_Data[2];
                    Ar_u8RePDU_DATA[3] = p_u8PDU_Data[3];

                    if (erasememoryflag == 1)
                    {
                        Ar_u8RePDU_DATA[4] = 0;
                        erasememoryflag = 0;
                    }
                    else
                    {
                        Ar_u8RePDU_DATA[4] = 1;
                    }

                    uds_positive_response(tUDS, tUDS->can_id_res, 5, Ar_u8RePDU_DATA);
                    break;
                case CheckLeanTuKey:

                     uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);
					 ap_do_report_request_tukey_qet();
					 int time_conut=0;
					 time_conut = tm_get_time();
                     while(1)
                     {
                       if( ((tm_get_time()-time_conut) > 5000) || tu_key_check_flag_finish == 1 )
                       {
                        break;
                       }
                       if((tm_get_time()-time_conut)%20 == 0)
                       {
                       uds_negative_response(tUDS,p_u8PDU_Data[0], NRC_RequestCorrectlyReceivedResponsePending);
                       }
                     }
                    Ar_u8RePDU_DATA[0] = p_u8PDU_Data[0] + POS_RESPOND_SID_MASK;
                    Ar_u8RePDU_DATA[1] = p_u8PDU_Data[1];
                    Ar_u8RePDU_DATA[2] = p_u8PDU_Data[2];
                    Ar_u8RePDU_DATA[3] = p_u8PDU_Data[3];                    
                    Ar_u8RePDU_DATA[4] =tu_key_check_flag_finish;//0:SUCCESS 1:FAIL
                    tu_key_check_flag_finish = 0;
                    uds_positive_response(tUDS, tUDS->can_id_res, 5, Ar_u8RePDU_DATA);
                    break;
                default:
                    uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_RequestOutOfRange);
                    break;
            }

        }
        else
        {
            uds_negative_response(tUDS, p_u8PDU_Data[0], NRC_SubFuncationNotSupported);
        }
    }
}

