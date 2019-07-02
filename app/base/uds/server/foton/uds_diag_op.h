#ifndef UDS_DIAG_OP_H
#define UDS_DIAG_OP_H

int ft_uds_diag_dev_ecall(void);
int ft_uds_diag_gps_ant_gnd(void);
int ft_uds_diag_gps_ant_power(void);
int ft_uds_diag_gps_ant_open(void);
int ft_uds_diag_dev_gps_module(void);
int ft_uds_diag_wan_ant_gnd(void);
int ft_uds_diag_wan_ant_power(void);
int ft_uds_diag_wan_ant_open(void);
int ft_uds_diag_gsm(void);
int ft_uds_diag_mic_gnd(void);
int ft_uds_diag_sim(void);
int ft_uds_diag_bat_low(void);
int ft_uds_diag_bat_high(void);
int ft_uds_diag_bat_aged(void);
int ft_uds_diag_gsense(void);
int ft_uds_diag_power_high(void);
int ft_uds_diag_power_low(void);
int uds_diag_dev_missing_gw(void);

int uds_diag_dev_can_busoff(void);


int uds_diag_dev_acu_can_bus_miss(void);
int uds_diag_dev_bms_can_bus_miss(void);
int uds_diag_dev_cdu_can_bus_miss(void);
int uds_diag_dev_mcu_can_bus_miss(void);
int uds_diag_dev_vcu1_can_bus_miss(void);

int uds_diag_dev_eps_can_bus_miss(void);
int uds_diag_dev_esc_can_bus_miss(void);
int uds_diag_dev_ehb_can_bus_miss(void);
int uds_diag_dev_eacp_can_bus_miss(void);
int uds_diag_dev_ptc_can_bus_miss(void);

int uds_diag_dev_plg_can_bus_miss(void);
int uds_diag_dev_clm_can_bus_miss(void);
int uds_diag_dev_bdcm_can_bus_miss(void);
int uds_diag_dev_alm_can_bus_miss(void);
int uds_diag_dev_icu_can_bus_miss(void);

int uds_diag_dev_ihu_can_bus_miss(void);
int uds_diag_dev_tap_can_bus_miss(void);




#endif
