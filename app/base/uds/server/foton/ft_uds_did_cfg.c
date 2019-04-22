#include "cfg_api.h"
#include "cfg_para_api.h"
#include "log.h"
#include "uds.h"
#include "at.h"
#include "nm_api.h"
#include "tbox_limit.h"
#include "can_api.h"
#include "ft_uds_tbox_rule.h"

int ft_uds_did_get_apn1_address(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (64 > len)
    {
        log_e(LOG_UDS, "get apn1 address error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0, len);

    length = 32;
    return cfg_get_para(CFG_ITEM_LOCAL_APN, did, &length);
}

int ft_uds_did_get_apn1_user(unsigned char *did, unsigned int len)
{
    int ret;
    unsigned int length;
    CFG_DIAL_AUTH auth;

    if (32 > len)
    {
        log_e(LOG_UDS, "get apn1 user error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_LOC_APN_AUTH, &auth, &length);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return ret;
    }


    memset(did, 0, len);
    memcpy(did, auth.user, sizeof(auth.user));

    return 0;
}

int ft_uds_did_get_apn1_pass(unsigned char *did, unsigned int len)
{
    int ret;
    unsigned int length;
    CFG_DIAL_AUTH auth;

    if (32 > len)
    {
        log_e(LOG_UDS, "get apn1 pwd error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_LOC_APN_AUTH, &auth, &length);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return ret;
    }

    memset(did, 0, len);
    memcpy(did, auth.pwd, sizeof(auth.pwd));

    return 0;
}


int ft_uds_did_get_apn2_address(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (64 > len)
    {
        log_e(LOG_UDS, "get apn2 address error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0, len);

    length = 32;
    return cfg_get_para(CFG_ITEM_WAN_APN, did, &length);
}

int ft_uds_did_get_apn2_user(unsigned char *did, unsigned int len)
{
    int ret;
    unsigned int length;
    CFG_DIAL_AUTH auth;

    if (32 > len)
    {
        log_e(LOG_UDS, "get apn2 user error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_WAN_APN_AUTH, &auth, &length);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return ret;
    }

    memset(did, 0, len);
    memcpy(did, auth.user, sizeof(auth.user));

    return 0;
}

int ft_uds_did_get_apn2_pass(unsigned char *did, unsigned int len)
{
    int ret;
    unsigned int length;
    CFG_DIAL_AUTH auth;

    if (32 > len)
    {
        log_e(LOG_UDS, "get apn1 pwd error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = sizeof(auth);
    ret = cfg_get_para(CFG_ITEM_WAN_APN_AUTH, &auth, &length);

    if (ret != 0)
    {
        log_e(LOG_NM, "get apn auth failed, ret:0x%08x", ret);
        return ret;
    }

    memset(did, 0, len);
    memcpy(did, auth.pwd, sizeof(auth.pwd));

    return 0;
}

int ft_uds_did_get_tsp_ip(unsigned char *did, unsigned int len)
{
    if (32 > len)
    {
        log_e(LOG_UDS, "get tsp ip error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0, 32);

    /* Resvered TBD */
    log_e(LOG_UDS, "get tsp ip is not supported!");

    return 0;
}

int ft_uds_did_get_http(unsigned char *did, unsigned int len)
{
    if (32 > len)
    {
        log_e(LOG_UDS, "get http error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0, 32);

    /* Resvered TBD */
    log_e(LOG_UDS, "get http is not supported!");

    return 0;
}

int ft_uds_did_get_tsp_ip_num(unsigned char *did, unsigned int len)
{
    if (6 > len)
    {
        log_e(LOG_UDS, "get tsp ip port number error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memset(did, 0, 32);

    /* Resvered TBD */
    log_e(LOG_UDS, "get ip port is not supported!");

    return 0;
}

int ft_uds_did_get_avn_sn(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (16 > len)
    {
        log_e(LOG_UDS, "get avn sn error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 16;
    return cfg_get_para(CFG_ITEM_FT_UDS_AVN_SERIAL_NUMBER, did, &length);
}

int ft_uds_did_get_phone(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char tel[TBOX_TEL_LEN];

    if (15 > len)
    {
        log_e(LOG_UDS, "get phone error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_telno(tel);
    memset(did, 0, len);

	length = 15;
	if(strlen(tel))
	{
   		memcpy(did, tel, length);
	}else
	{
		length = 13;
		cfg_get_para(CFG_ITEM_FT_SIM, did, &length);
	}
	
    return 0;
}

int ft_uds_did_get_iccid(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readiccid[CCID_LEN];

    if (20 > len)
    {
        log_e(LOG_UDS, "get iccid error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    nm_get_iccid(readiccid);
    memset(did, 0, len);

    length = 20;
    memcpy(did, readiccid, length);

    return 0;
}

int ft_uds_did_get_imsi(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readimsi[TBOX_IMSI_LEN];

    if (15 > len)
    {
        log_e(LOG_UDS, "get IMSI error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_imsi(readimsi);
    memset(did, 0, len);

    length = 15;
    memcpy(did, readimsi, length);

    return 0;
}

int ft_uds_did_get_imei(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char readimei[TBOX_IMEI_LEN];

    if (15 > len)
    {
        log_e(LOG_UDS, "get IMEI error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    at_get_imei(readimei);

    memset(did, 0, len);

    length = 15;
    memcpy(did, readimei, length);

    return 0;
}

int ft_uds_did_get_vehicle_type(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get vehicle type error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 1;
    return cfg_get_para(CFG_ITEM_FT_UDS_VEHICLE_TYPE, did, &length);
}

int ft_uds_did_get_reg_status(unsigned char *did, unsigned int len) 
{	
	unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get reg status error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 1;
    return cfg_get_para(CFG_ITEM_FT_REGISTER, did, &length);
}

int ft_uds_did_get_brand_type(unsigned char *did, unsigned int len) 
{
	unsigned int length;

    if (1 > len)
    {
        log_e(LOG_UDS, "get reg status error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 1;
    return cfg_get_para(CFG_ITEM_FT_PORT, did, &length);
}

int ft_uds_did_set_brand_type(unsigned char *did, unsigned int len)
{
	if (1 > len)
    {
        log_e(LOG_UDS, "set brand type len error, len:%d", len);
        return UDS_INVALID_PARA;
    }
   	return cfg_set_para(CFG_ITEM_FT_PORT, did, 1);
}

/*SET*/
int ft_uds_did_set_apn1_user(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn1 user is not supported!");
    return 0;
}

int ft_uds_did_set_apn1_address(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn1 address is not supported!");
    return 0;
}

int ft_uds_did_set_apn1_pass(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn1 pwd is not supported!");
    return 0;
}

int ft_uds_did_set_apn2_user(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn2 user is not supported!");
    return 0;
}

int ft_uds_did_set_apn2_address(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn2 address is not supported!");
    return 0;
}

int ft_uds_did_set_apn2_pass(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set apn1 pwd is not supported!");
    return 0;
}

int ft_uds_did_set_tsp_ip(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set tsp ip is not supported!");
    return 0;
}

int ft_uds_did_set_http(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set http is not supported!");
    return 0;
}

int ft_uds_did_set_tsp_ip_num(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set tsp ip is not supported!");
    return 0;
}

int ft_uds_did_set_avn_sn(unsigned char *did, unsigned int len)
{
    if (16 > len)
    {
        log_e(LOG_UDS, "set vehicle type len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    return cfg_set_para(CFG_ITEM_FT_UDS_AVN_SERIAL_NUMBER, did, 16);
}

int ft_uds_did_set_vehicle_type(unsigned char *did, unsigned int len)
{
    int ret;

    if (1 > len)
    {
        log_e(LOG_UDS, "set vehicle type len error, len:%d", len);
        return UDS_INVALID_PARA;
    }


    ret = dbc_set_by_type(*did, 1);

    if (ret == 0)
    {
        ret = cfg_set_para(CFG_ITEM_FT_UDS_VEHICLE_TYPE, did, 1);
    }
    else
    {
        log_e(LOG_UDS, "set vehicle type  failture type:0x%x", ret);
    }

    return ret;
}

