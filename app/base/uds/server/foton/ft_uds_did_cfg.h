#ifndef FT_UDS_DID_CFG_H
#define FT_UDS_DID_CFG_H

int ft_uds_did_get_apn1_user(unsigned char *did, unsigned int len);
int ft_uds_did_get_apn1_address(unsigned char *did, unsigned int len);
int ft_uds_did_get_apn1_pass(unsigned char *did, unsigned int len);
int ft_uds_did_get_apn2_user(unsigned char *did, unsigned int len);
int ft_uds_did_get_apn2_address(unsigned char *did, unsigned int len);
int ft_uds_did_get_apn2_pass(unsigned char *did, unsigned int len);
int ft_uds_did_get_tsp_ip(unsigned char *did, unsigned int len);
int ft_uds_did_get_http(unsigned char *did, unsigned int len);
int ft_uds_did_get_tsp_ip_num(unsigned char *did, unsigned int len);
int ft_uds_did_get_avn_sn(unsigned char *did, unsigned int len);
int ft_uds_did_get_phone(unsigned char *did, unsigned int len);
int ft_uds_did_get_iccid(unsigned char *did, unsigned int len);
int ft_uds_did_get_imsi(unsigned char *did, unsigned int len);
int ft_uds_did_get_imei(unsigned char *did, unsigned int len);
int ft_uds_did_get_vehicle_type(unsigned char *did, unsigned int len);

int ft_uds_did_get_reg_status(unsigned char *did, unsigned int len);
int ft_uds_did_get_brand_type(unsigned char *did, unsigned int len);

int ft_uds_did_set_apn1_user(unsigned char *did, unsigned int len);
int ft_uds_did_set_apn1_address(unsigned char *did, unsigned int len);
int ft_uds_did_set_apn1_pass(unsigned char *did, unsigned int len);
int ft_uds_did_set_apn2_user(unsigned char *did, unsigned int len);
int ft_uds_did_set_apn2_address(unsigned char *did, unsigned int len);
int ft_uds_did_set_apn2_pass(unsigned char *did, unsigned int len);
int ft_uds_did_set_tsp_ip(unsigned char *did, unsigned int len);
int ft_uds_did_set_http(unsigned char *did, unsigned int len);
int ft_uds_did_set_tsp_ip_num(unsigned char *did, unsigned int len);
int ft_uds_did_set_avn_sn(unsigned char *did, unsigned int len);
int ft_uds_did_set_vehicle_type(unsigned char *did, unsigned int len);

int ft_uds_did_set_brand_type(unsigned char *did, unsigned int len);
#endif
