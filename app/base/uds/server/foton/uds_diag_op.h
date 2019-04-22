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

#endif
