/****************************************************************
file:         uds_diag_def.h
description:  the source file of uds diagnose item definition
date:         2016/9/26
author        liuzhongwen
****************************************************************/
#include "uds_diag.h"
#include "uds_did_def.h"
#include "uds_dtc_def.h"
#include "uds_diag_item_def.h"
#include "uds_did_op.h"
#include "uds_diag_op.h"
#include "ft_uds_did_attr.h"
#include "ft_uds_did_cfg.h"

/*FOTON DID*/
DIAG_DID_TABLE_BEGIN()

/*did Logistics*/
DIAG_DID_ATTR("ECU_Identifier", FT_DID_ECU_IDENTIFIER,      DIAG_DATA_STRING,       8,
              ft_uds_did_get_ecu_id,            NULL)

DIAG_DID_ATTR("ECU_Code",       FT_DID_ECU_CODE,            DIAG_DATA_STRING,       14,
              ft_uds_did_get_ecu_code,          NULL)

DIAG_DID_ATTR("ECU_number",     FT_DID_ECU_NUMBER,          DIAG_DATA_STRING,       10,
              ft_uds_did_get_ecu_model_num,    NULL)

DIAG_DID_ATTR("Supplier_ID",    FT_DID_SYSTEM_SUPPLIER_IDENTIFIER, DIAG_DATA_STRING, 5,
              ft_uds_did_get_ecu_supplier_code,   NULL)

DIAG_DID_ATTR("Repair shop",    FT_DID_REPAIR_SHOP_CODE,    DIAG_DATA_STRING,       10,
              ft_uds_did_get_repair_shop_id,    NULL)

DIAG_DID_ATTR("Factory part",   FT_DID_VEHICLE_MANUFACTURER_SPARE_PART_NUMBER, DIAG_DATA_STRING, 13,
              ft_uds_did_get_manufacturer_part_id, NULL)

DIAG_DID_ATTR("VIN",            FT_DID_UDS_VIN,             DIAG_DATA_STRING,       17,
              ft_uds_did_get_vin,               ft_uds_did_set_vin)

DIAG_DID_ATTR("Pragram date",   FT_DID_PROGRAMMING_DATA,    DIAG_DATA_STRING,        4,
              ft_uds_did_get_program_date,      ft_uds_did_set_program_date)

DIAG_DID_ATTR("System name",    FT_DID_SYSTEM_NAME,         DIAG_DATA_STRING,       14,
              ft_uds_did_get_system_name,       NULL)

DIAG_DID_ATTR("HW version", FT_DID_ECU_HARDWARE_VERSION_NUMBER, DIAG_DATA_STRING,   14,
              ft_uds_did_get_hw_version,        NULL)

DIAG_DID_ATTR("HW version", FT_DID_ECU_SOFTWARE_VERSION_NUMBER, DIAG_DATA_STRING,   14,
              ft_uds_did_get_sw_version,        NULL)

DIAG_DID_ATTR("HW version", FT_DID_FINGERPRINTDATAIDENTIFIER,   DIAG_DATA_STRING,   14,
              ft_uds_did_get_finger_print_id,   NULL)


/*did  Configuration*/
DIAG_DID_ATTR("APN1 user",  FT_DID_APN1_USER,           DIAG_DATA_STRING,               64,
              ft_uds_did_get_apn1_user,                 ft_uds_did_set_apn1_user)

DIAG_DID_ATTR("APN1 addr",  FT_DID_APN1_ADDRESS,        DIAG_DATA_STRING,               32,
              ft_uds_did_get_apn1_address,              ft_uds_did_set_apn1_address)

DIAG_DID_ATTR("APN1 pass",  FT_DID_APN1_PASS,            DIAG_DATA_STRING,              32,
              ft_uds_did_get_apn1_pass,                 ft_uds_did_set_apn1_pass)

DIAG_DID_ATTR("APN2 user",  FT_DID_APN2_USER,           DIAG_DATA_STRING,               64,
              ft_uds_did_get_apn2_user,                 ft_uds_did_set_apn2_user)

DIAG_DID_ATTR("APN2 addr",  FT_DID_APN2_ADDRESS,        DIAG_DATA_STRING,               32,
              ft_uds_did_get_apn2_address,              ft_uds_did_set_apn2_address)

DIAG_DID_ATTR("APN2 pass",  FT_DID_APN2_PASS,           DIAG_DATA_STRING,               32,
              ft_uds_did_get_apn2_pass,                 ft_uds_did_set_apn2_pass)

DIAG_DID_ATTR("tsp ip",     FT_DID_TSP_IP,              DIAG_DATA_STRING,               32,
              ft_uds_did_get_tsp_ip,                    ft_uds_did_set_tsp_ip)

DIAG_DID_ATTR("http",       FT_DID_HTTP,                DIAG_DATA_STRING,               32,
              ft_uds_did_get_http,                      ft_uds_did_set_http)

DIAG_DID_ATTR("tsp ip num", FT_DID_TSP_IP_PORT_NUMBER,  DIAG_DATA_STRING,               6,
              ft_uds_did_get_tsp_ip_num,                ft_uds_did_set_tsp_ip_num)

DIAG_DID_ATTR("avn sn",     FT_DID_AVN_SERIAL_NUMBER,   DIAG_DATA_STRING,               16,
              ft_uds_did_get_avn_sn,                    ft_uds_did_set_avn_sn)

DIAG_DID_ATTR("phone",      FT_DID_PHONE,               DIAG_DATA_STRING,               15,
              ft_uds_did_get_phone,                     NULL)

DIAG_DID_ATTR("ICCID",      FT_DID_ICCID,               DIAG_DATA_STRING,               20,
              ft_uds_did_get_iccid,                     NULL)

DIAG_DID_ATTR("IMSI",       FT_DID_IMSI,                DIAG_DATA_STRING,               15,
              ft_uds_did_get_imsi,                      NULL)

DIAG_DID_ATTR("IMEI",       FT_DID_IMEI,                DIAG_DATA_STRING,               15,
              ft_uds_did_get_imei,                      NULL)

DIAG_DID_ATTR("IMEI",       FT_DID_VEHICLE_TYPE,        DIAG_DATA_UCHAR,
              sizeof(unsigned char),
              ft_uds_did_get_vehicle_type,              ft_uds_did_set_vehicle_type)

DIAG_DID_ATTR("Brand port", FT_DID_BRAND_PROT,        	DIAG_DATA_UCHAR,
              sizeof(unsigned char),
              ft_uds_did_get_brand_type,              	ft_uds_did_set_brand_type)

DIAG_DID_ATTR("reg status", FT_DID_REGISTER_STATUS,     DIAG_DATA_UCHAR,
              sizeof(unsigned char),
              ft_uds_did_get_reg_status,              	NULL)
DIAG_DID_TABLE_END()

DIAG_TABLE_BEGIN()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_ECALL_SWITCH)
DIAG_ITEM_DTC(FT_DTC_ECALL_SWITCH)
DIAG_ITEM_NAME("ECALL")
DIAG_ITEM_FUN(ft_uds_diag_dev_ecall)
DIAG_ITEM_THSD(45)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GPS_ANTENNA_SHORT_TO_GND)
DIAG_ITEM_DTC(FT_DTC_GPS_ANTENNA_SHORT_TO_GND)
DIAG_ITEM_NAME("GPS ANT TO GND")
DIAG_ITEM_FUN(ft_uds_diag_gps_ant_gnd)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GPS_ANTENNA_SHORT_TO_BAT)
DIAG_ITEM_DTC(FT_DTC_GPS_ANTENNA_SHORT_TO_BAT)
DIAG_ITEM_NAME("GPS ANT TO BAT")
DIAG_ITEM_FUN(ft_uds_diag_gps_ant_power)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GPS_ANTENNA_OPEN)
DIAG_ITEM_DTC(FT_DTC_GPS_ANTENNA_OPEN)
DIAG_ITEM_NAME("GPS ANT OPEN")
DIAG_ITEM_FUN(ft_uds_diag_gps_ant_open)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GPS_MODULE_FAULT)
DIAG_ITEM_DTC(FT_DTC_GPS_MODULE_FAULT)
DIAG_ITEM_NAME("GPS MODULE FAULT")
DIAG_ITEM_FUN(ft_uds_diag_dev_gps_module)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_WAN_ANTENNA_SHORT_TO_GND)
DIAG_ITEM_DTC(FT_DTC_WAN_ANTENNA_SHORT_TO_GND)
DIAG_ITEM_NAME("WAN ANT GND")
DIAG_ITEM_FUN(ft_uds_diag_wan_ant_gnd)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_WAN_ANTENNA_SHORT_TO_BAT)
DIAG_ITEM_DTC(FT_DTC_WAN_ANTENNA_SHORT_TO_BAT)
DIAG_ITEM_NAME("WAN ANT SHORT BAT")
DIAG_ITEM_FUN(ft_uds_diag_wan_ant_power)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_WAN_ANTENNA_OPEN)
DIAG_ITEM_DTC(FT_DTC_WAN_ANTENNA_OPEN)
DIAG_ITEM_NAME("WAN ANT OPEN")
DIAG_ITEM_FUN(ft_uds_diag_wan_ant_open)
DIAG_ITEM_THSD(12)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GSM_MODULE)
DIAG_ITEM_DTC(FT_DTC_GSM_MODULE)
DIAG_ITEM_NAME("GSM")
DIAG_ITEM_FUN(ft_uds_diag_gsm)
DIAG_ITEM_THSD(60)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_MIC_OUTPUT_SHORT_TO_GROUND)
DIAG_ITEM_DTC(FT_DTC_MIC_OUTPUT_SHORT_TO_GROUND)
DIAG_ITEM_NAME("MIC TO GND")
DIAG_ITEM_FUN(ft_uds_diag_mic_gnd)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_SIM_FAULT)
DIAG_ITEM_DTC(FT_DTC_SIM_FAULT)
DIAG_ITEM_NAME("SIM")
DIAG_ITEM_FUN(ft_uds_diag_sim)
DIAG_ITEM_THSD(60)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_BATTERY_TOO_LOW)
DIAG_ITEM_DTC(FT_DTC_BATTERY_TOO_LOW)
DIAG_ITEM_NAME("BAT LOW")
DIAG_ITEM_FUN(ft_uds_diag_bat_low)
DIAG_ITEM_THSD(10)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_BATTERY_TOO_HIGH)
DIAG_ITEM_DTC(FT_DTC_BATTERY_TOO_HIGH)
DIAG_ITEM_NAME("BAT HIGH")
DIAG_ITEM_FUN(ft_uds_diag_bat_high)
DIAG_ITEM_THSD(10)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_BATTERY_AGED)
DIAG_ITEM_DTC(FT_DTC_BATTERY_AGED)
DIAG_ITEM_NAME("BAT AGED")
DIAG_ITEM_FUN(ft_uds_diag_bat_aged)
DIAG_ITEM_THSD(10)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_GSENS_MODULE)
DIAG_ITEM_DTC(FT_DTC_GSENS_MODULE)
DIAG_ITEM_NAME("GSENS")
DIAG_ITEM_FUN(ft_uds_diag_gsense)
DIAG_ITEM_THSD(4)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_POWER_VOLTAGE_HIGH)
DIAG_ITEM_DTC(FT_DTC_POWER_VOLTAGE_HIGH)
DIAG_ITEM_NAME("power high")
DIAG_ITEM_FUN(ft_uds_diag_power_high)
DIAG_ITEM_THSD(1)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_ITEM_BEGIN()
DIAG_ITEM_ID(FT_ITEM_DIAG_POWER_VOLTAGE_LOW)
DIAG_ITEM_DTC(FT_DTC_POWER_VOLTAGE_LOW)
DIAG_ITEM_NAME("power low")
DIAG_ITEM_FUN(ft_uds_diag_power_low)
DIAG_ITEM_THSD(1)
DIAG_FREEZE_BEGIN()
//DIAG_DID(DID_WAKEUP_SRC)
DIAG_FREEZE_END()
DIAG_ITEM_END()

DIAG_TABLE_END()


