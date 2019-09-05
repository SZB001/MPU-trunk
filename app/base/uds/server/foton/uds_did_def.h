#ifndef UDS_DID_DEF_H
#define UDS_DID_DEF_H



/* DynamicData DID */
#define DID_POWER_VOLTAGE                               (0x0112)	
#define DID_TIME                                        (0xF020)
#define DID_ODOMETER_READING                            (0xE101)
#define DID_VEHICLE_SPEED                               (0xB100)
#define DID_IGNITION_STATUS                             (0xD001)
#define DID_GPS_MODULE_STATUS                           (0x9004)
#define DID_GPS_POSITIONING_STATUS                      (0x9005)
#define DID_SD_CARD_STATUS                              (0x9006)
#define DID_4G_ANTENNA_STATUS                           (0x9007)
#define DID_PLATFORM_CONNECTION_STATUS                  (0x9009)
#define DID_4G_ANT_SIGNAL_STRENGTH                      (0x900C)
#define DID_CAN1_STATUS                                 (0x900D)
#define DID_CAN2_STATUS                                 (0x900E)

/*20190627 caoml*/
#define DID_ESK                                         (0x900F)
#define DID_PKI_STATUS                                  (0x9010)

/* DynamicData DID Length*/
#define DID_LEN_POWER_VOLTAGE                               (1)	
#define DID_LEN_TIME                                        (7)
#define DID_LEN_ODOMETER_READING                            (3)
#define DID_LEN_VEHICLE_SPEED                               (2)
#define DID_LEN_IGNITION_STATUS                             (1)
#define DID_LEN_GPS_MODULE_STATUS                           (1)
#define DID_LEN_GPS_POSITIONING_STATUS                      (1)
#define DID_LEN_SD_CARD_STATUS                              (1)
#define DID_LEN_4G_ANTENNA_STATUS                           (1)
#define DID_LEN_PLATFORM_CONNECTION_STATUS                  (1)
#define DID_LEN_4G_ANT_SIGNAL_STRENGTH                      (1)
#define DID_LEN_CAN1_STATUS                                 (1)
#define DID_LEN_CAN2_STATUS                                 (1)


/*20190627 caoml*/
#define DID_LEN_ESK                                         (16)
#define DID_LEN_PKI_STATUS                                  (1)


/*  StoredData DID */
#define DID_BOOTLOADER_IDENTIFIER                       (0XF180)
#define DID_DIAGNOSTIC_SESSION                          (0XF186)
#define DID_SPARE_PART_NUMBER                           (0XF187)
#define DID_SOFTWARE_UPGRADE_VERSION                    (0XF188)
#define DID_SOFTWARE_FIXED_VERSION                      (0XF1B0)
#define DID_CALIBRATION_SOFTWARE_NUMBER                 (0XF1A2)
#define DID_SUPPLIER_CODE                               (0XF18A)
#define DID_MANUFACTURE_DATE                            (0XF18B)
#define DID_SN                                          (0XF18C)
#define DID_VIN                                         (0XF190)
#define DID_HW_VERSION                                  (0XF191)
#define DID_TESTER_SN                                   (0XF198)
#define DID_PROGRAMMING_DATE                            (0XF199)
#define DID_INSTALLATION_DATE                           (0XF19D)
#define DID_CONFIGURATION_CODE                          (0XF170)
#define DID_PHONE                                       (0X460A)
#define DID_ICCID                                       (0X460B)
#define DID_IMSI                                        (0X460C)
#define DID_IMEI                                        (0X460D)
#define DID_HARD_NO                                     (0xF1BF)
#define DID_SOFT_NO                                     (0xF1C0)

/*  StoredData DID Length*/
#define DID_LEN_BOOTLOADER_IDENTIFIER                       (8)
#define DID_LEN_DIAGNOSTIC_SESSION                          (1)
#define DID_LEN_SPARE_PART_NUMBER                           (13)
#define DID_LEN_SOFTWARE_UPGRADE_VERSION                    (8)
#define DID_LEN_SOFTWARE_FIXED_VERSION                      (8)
#define DID_LEN_CALIBRATION_SOFTWARE_NUMBER                 (4)
#define DID_LEN_SUPPLIER_CODE                               (3)
#define DID_LEN_MANUFACTURE_DATE                            (4)
#define DID_LEN_SN                                          (18)
#define DID_LEN_VIN                                         (17)
#define DID_LEN_HW_VERSION                                  (5)
#define DID_LEN_TESTER_SN                                   (10)
#define DID_LEN_PROGRAMMING_DATE                            (4)
#define DID_LEN_INSTALLATION_DATE                           (4)
#define DID_LEN_CONFIGURATION_CODE                          (1)
#define DID_LEN_PHONE                                       (15)
#define DID_LEN_ICCID                                       (20)
#define DID_LEN_IMSI                                        (15)
#define DID_LEN_IMEI                                        (15)
#define DID_LEN_HARD_NO                                     (5)
#define DID_LEN_SOFT_NO                                     (8)

#endif
