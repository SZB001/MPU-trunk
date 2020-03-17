#ifndef __UDS_DID_H__
#define __UDS_DID_H__

#define SUPPLYNAME   "A027E00428"
#define DID_F18A_SUPPLIER_IDENTIFIER "9AV"
#define DID_F187_SPARE_PART_NO "S30-7925010"
#define DID_F1B0_SW_FIXED_VER "05.00.00"//固定版本号，过检版本号
#define DID_F1B0_SW_UPGRADE_VER "05.02.00"

#define DID_F1BF_HW_VERSION "H1.10"

int uds_did_get_wakup_src(unsigned char *did, unsigned int len);
int uds_did_get_pow_voltage(unsigned char *did, unsigned int len);
int uds_did_get_time(unsigned char *did, unsigned int len);
int uds_did_get_4G_main_ant(unsigned char *did, unsigned int len);
int uds_did_get_4G_vice_ant(unsigned char *did, unsigned int len);
int uds_did_get_gps_ant(unsigned char *did, unsigned int len);
int uds_did_get_gps_module(unsigned char *did, unsigned int len);
int uds_did_get_fix_status(unsigned char *did, unsigned int len);
int uds_did_get_emmc(unsigned char *did, unsigned int len);
int uds_did_get_4G_module(unsigned char *did, unsigned int len);
int uds_did_get_4G_comm(unsigned char *did, unsigned int len);
int uds_did_get_ccid(unsigned char *did, unsigned int len);
int uds_did_get_apn1(unsigned char *did, unsigned int len);
int uds_did_get_4G_sig(unsigned char *did, unsigned int len);
int uds_did_get_can1(unsigned char *did, unsigned int len);
int uds_did_get_can2(unsigned char *did, unsigned int len);
int uds_did_get_pm_mode(unsigned char *did, unsigned int len);
int uds_did_get_can3(unsigned char *did, unsigned int len);
int uds_did_get_car_mode(unsigned char *did, unsigned int len);
int uds_did_set_car_mode(unsigned char *did, unsigned int len);
int uds_did_get_apn2(unsigned char *did, unsigned int len);
int uds_did_get_wifi(unsigned char *did, unsigned int len);
int uds_did_get_dbc_name(unsigned char *did, unsigned int len);
int uds_did_get_vin(unsigned char *did, unsigned int len);
int uds_did_set_vin(unsigned char *did, unsigned int len);
int uds_did_get_dev_no(unsigned char *did, unsigned int len);
int uds_did_get_btl_soft_finger(unsigned char *did, unsigned int len);
int uds_did_get_app_soft_finger(unsigned char *did, unsigned int len);
int uds_did_set_app_soft_finger(unsigned char *did, unsigned int len);
int uds_did_get_supply_code(unsigned char *did, unsigned int len);
int uds_did_get_ecu_hard_ver(unsigned char *did, unsigned int len);
int uds_did_get_ecu_soft_ver(unsigned char *did, unsigned int len);
int uds_did_get_pro_date(unsigned char *did, unsigned int len);
int uds_did_set_pro_date(unsigned char *did, unsigned int len);
int uds_did_get_tuid(unsigned char *did, unsigned int len);
int uds_did_get_checksum_status(unsigned char *did, unsigned int len);
int uds_did_get_tbox_status(unsigned char *did, unsigned int len);

/* DynamicData DID add by caoml*/
int uds_did_get_odometer_reading(unsigned char *did, unsigned int len);
int uds_did_get_vehicle_speed(unsigned char *did, unsigned int len);
int uds_did_get_ignition_status(unsigned char *did, unsigned int len);
int uds_did_get_platform_connection_status(unsigned char *did, unsigned int len);

/*  StoredData DID add by caoml*/
int uds_did_get_bootloader_identifier(unsigned char *did, unsigned int len);
int uds_did_get_diagnostic_session(unsigned char *did, unsigned int len);
int uds_did_get_spare_part_number(unsigned char *did, unsigned int len);
int uds_did_get_software_upgrade_version(unsigned char *did, unsigned int len);
int uds_did_get_software_fixed_version(unsigned char *did, unsigned int len);
int uds_did_get_calibration_software_number(unsigned char *did, unsigned int len);
int uds_did_get_supplier_code(unsigned char *did, unsigned int len);
int uds_did_get_manufacture_date(unsigned char *did, unsigned int len);
int uds_did_get_sn(unsigned char *did, unsigned int len);
int uds_did_get_hw_version(unsigned char *did, unsigned int len);
int uds_did_get_tester_sn(unsigned char *did, unsigned int len);
int uds_did_get_programming_date(unsigned char *did, unsigned int len);
int uds_did_get_installation_date(unsigned char *did, unsigned int len);
int uds_did_get_configuration_code(unsigned char *did, unsigned int len);
int uds_did_get_phone(unsigned char *did, unsigned int len);
int uds_did_get_iccid(unsigned char *did, unsigned int len);
int uds_did_get_imsi(unsigned char *did, unsigned int len);
int uds_did_get_imei(unsigned char *did, unsigned int len);


int uds_did_set_tester_sn(unsigned char *did, unsigned int len);
int uds_did_set_installation_date(unsigned char *did, unsigned int len);
int uds_did_set_configuration_code(unsigned char *did, unsigned int len);


int uds_did_get_esk(unsigned char *did, unsigned int len);
int uds_did_get_pki_status(unsigned char *did, unsigned int len);

int uds_did_set_esk(unsigned char *did, unsigned int len);
int uds_did_set_pki_status(unsigned char *did, unsigned int len);
int uds_did_get_hard_no(unsigned char *did, unsigned int len);
int uds_did_get_soft_no(unsigned char *did, unsigned int len);
int set_cfg_code(unsigned char cfg_code);

#if 0
int uds_did_set_manufacture_date(unsigned char *did, unsigned int len);
#endif

#endif
