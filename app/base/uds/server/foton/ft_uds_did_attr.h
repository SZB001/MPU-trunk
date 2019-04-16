#ifndef FT_UDS_DID_ATTR_H
#define FT_UDS_DID_ATTR_H

int ft_uds_did_get_ecu_id(unsigned char *did, unsigned int len);
int ft_uds_did_get_ecu_code(unsigned char *did, unsigned int len);
int ft_uds_did_get_ecu_model_num(unsigned char *did, unsigned int len);
int ft_uds_did_get_ecu_supplier_code(unsigned char *did, unsigned int len);
int ft_uds_did_get_repair_shop_id(unsigned char *did, unsigned int len);
int ft_uds_did_get_manufacturer_part_id(unsigned char *did, unsigned int len);
int ft_uds_did_get_vin(unsigned char *did, unsigned int len);
int ft_uds_did_get_program_date(unsigned char *did, unsigned int len);
int ft_uds_did_get_system_name(unsigned char *did, unsigned int len);
int ft_uds_did_get_hw_version(unsigned char *did, unsigned int len);
int ft_uds_did_get_sw_version(unsigned char *did, unsigned int len);
int ft_uds_did_get_finger_print_id(unsigned char *did, unsigned int len);

int ft_uds_did_set_vin(unsigned char *did, unsigned int len);
int ft_uds_did_set_program_date(unsigned char *did, unsigned int len);

#endif
