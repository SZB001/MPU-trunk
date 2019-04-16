#include "cfg_api.h"
#include "log.h"
#include "uds.h"
#include "dev_api.h"
#include "gps_api.h"
#include "tbox_limit.h"
#include "ft_uds_tbox_rule.h"

#include <string.h>
#include <stdio.h>


int ft_uds_did_get_ecu_id(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned int ecu_id = 0;
    char string[32] = {0};
    int ret = 0;

    if (8 > len)
    {
        log_e(LOG_UDS, "get ecu id len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 4;
    ret = cfg_get_para(CFG_ITEM_SN_NUM, (void *)&ecu_id, &length);

    if (ret == 0)
    {
        sprintf(string, "%d", ecu_id);
        memset(did, 0, 8);
        memcpy(did, string, 8);
    }
    else
    {
        log_e(LOG_UDS, "get ecu_id error");
    }

    return ret;
}

int ft_uds_did_get_ecu_code(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (14 > len)
    {
        log_e(LOG_UDS, "get ecu code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 14;

    memset(did, 0, length);

    memcpy(did, TBOX_CODE, strlen(TBOX_CODE));

    return 0;
}

int ft_uds_did_get_ecu_model_num(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (10 > len)
    {
        log_e(LOG_UDS, "get ecu model num len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 10;

    memset(did, 0, length);
 //   memcpy(did, MODEL_NUM, strlen(MODEL_NUM));
	cfg_get_para(CFG_ITEM_FT_DEV_TYPE, did, &length);

    return 0;
}

int ft_uds_did_get_ecu_supplier_code(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (5 > len)
    {
        log_e(LOG_UDS, "get ecu supplier code len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 5;
    memset(did, 0, length);
    memcpy(did, SUPPLIER_CODE, length);

    return 0;
}

int ft_uds_did_get_repair_shop_id(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (10 > len)
    {
        log_e(LOG_UDS, "get repair shop len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 10;

    return cfg_get_para(CFG_ITEM_FT_UDS_REPAIR_SHOP_CODE, did, &length);
}

int ft_uds_did_get_manufacturer_part_id(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char ft_part_num[14] = {0};
    int ret = -1;

    if (13 > len)
    {
        log_e(LOG_UDS, "get manufacturer part id len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 13;

    memset(did, 0, length);

    length = 14;

    ret = cfg_get_para(CFG_ITEM_PART_NUM, ft_part_num, &length);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get_manufacturer_part_id error");
    }

    memcpy(did, ft_part_num, 13);

    return ret;
}

int ft_uds_did_get_vin(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned char vin[18];
    int ret;

    if (17 > len)
    {
        log_e(LOG_UDS, "get vin len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 18;

    ret = cfg_get_para(CFG_ITEM_FOTON_VIN, (unsigned char *)vin, &length);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get vin error");
    }

    memcpy(did, (unsigned char *)vin, 17);

    return ret;
}

int ft_uds_did_set_vin(unsigned char *did, unsigned int len)
{
    unsigned int length;
    unsigned char vin[18];

    if (17 > len)
    {
        log_e(LOG_UDS, "set vin len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    memcpy((unsigned char *)vin, did, 17);

    length = 18;
    vin[17] = 0;

    return cfg_set_para(CFG_ITEM_FOTON_VIN, (void *)vin, length);
}


int ft_uds_did_get_program_date(unsigned char *did, unsigned int len)
{
    unsigned int length;
    uint8_t *program_date;
    char buf[8] = {0};
    uint8_t hex2bcd[4] = {0};

    if (4 > len)
    {
        log_e(LOG_UDS, "get program date len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 4;

    memset(did, 0, length);

    program_date = get_date_yymmdd(buf, 1, hex2bcd);

    memcpy(did, (unsigned char *)program_date, length);

    return 0;
}

int ft_uds_did_set_program_date(unsigned char *did, unsigned int len)
{
    log_e(LOG_UDS, "set_program_date is not supported!");

    return 0;
}

int ft_uds_did_get_system_name(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (14 > len)
    {
        log_e(LOG_UDS, "get system name len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 14;
    memset(did, 0, length);
    memcpy(did, SYSTEM_NAME, strlen(SYSTEM_NAME));

    return 0;
}

int ft_uds_did_get_hw_version(unsigned char *did, unsigned int len)
{
    char ft_hw_ver[14];
    unsigned int length;
    int ret = 0;

    if (14 > len)
    {
        log_e(LOG_UDS, "get hw version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 14;

    memset(did, 0, length);
    memset(ft_hw_ver, 0, sizeof(ft_hw_ver));

    ret = cfg_get_para(CFG_ITEM_FT_UDS_HW, ft_hw_ver, &length);

    if (ret != 0)
    {
        log_e(LOG_UDS, "get ft uds hw error");
        return ret;
    }

    memcpy(did, ft_hw_ver, 14);

    return ret;
}


int ft_uds_did_get_sw_version(unsigned char *did, unsigned int len)
{
    unsigned int length;

    if (14 > len)
    {
        log_e(LOG_UDS, "get sw version len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 14;

    memset(did, 0, length);
    get_uds_sw_ver((UDS_DID_VER *) did);

    return 0;
}

int ft_uds_did_get_finger_print_id(unsigned char *did, unsigned int len)
{
    unsigned int length;
    char buf[8] = {0};
    uint8_t hex2bcd[4] = {0};

    if (14 > len)
    {
        log_e(LOG_UDS, "get finger_print_id len error, len:%d", len);
        return UDS_INVALID_PARA;
    }

    length = 14;

    memset(did, 0, length);

    length = 4;
    memcpy(did, get_date_yymmdd(buf, 1, hex2bcd), length);

    length = 10;

    return  cfg_get_para(CFG_ITEM_FT_UDS_REPAIR_SHOP_CODE, did + 4, &length);
}


