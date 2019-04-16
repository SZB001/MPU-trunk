#include <string.h>
#include "com_app_def.h"
#include "shell_api.h"
#include "uds_proxy.h"
#include "uds_define.h"
#include "log.h"
#include "init.h"
#include "uds_diag.h"
#include "cfg_api.h"
#include "ft_uds_tbox_rule.h"

int uds_shell_set_vehicle_type(int argc, const char **argv)
{
    unsigned int temp = 0;
    char did;
    int ret = 0;

    if (argc != 1)
    {
        shellprintf(" usage: car type <type> \r\n");
        return -1;
    }

    sscanf((char const *) argv[0], "%u", &temp);

    did = (char)temp;

    ret = dbc_set_by_type(did, 0);


    if (ret == 0)
    {
        ret = cfg_set_para(CFG_ITEM_FT_UDS_VEHICLE_TYPE, (void *)&did, 1);
    }

    return ret;
}


int uds_shell_set_ft_hw(int argc, const char **argv)
{
    char ft_hw_ver[14];
    int ret;
    memset(ft_hw_ver, 0, sizeof(ft_hw_ver));

    if (argc != 1)
    {
        shellprintf(" usage: foton uds hw\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 14)
    {
        shellprintf(" error: foton uds hw must be 14 charactores\r\n");
        return -1;
    }

    ret =  cfg_set_para(CFG_ITEM_FT_UDS_HW, (void *)argv[0], 14);

    return ret;
}

int uds_shell_set_ft_pn(int argc, const char **argv)
{
    char ft_part_num[14] = {0};
    int ret;
    
    memset(ft_part_num, 0, sizeof(ft_part_num));

    if (argc != 1)
    {
        shellprintf(" usage: foton uds pn\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 13)
    {
        shellprintf(" error: foton uds pn must be 13 charactores\r\n");
        return -1;
    }

    memcpy(ft_part_num, (void *)argv[0], 13);

    ft_part_num[13] = 0;

    ret =  cfg_set_para(CFG_ITEM_PART_NUM, ft_part_num, 14);

	shellprintf(" save ok\r\n");

    return ret;
}

int uds_shell_vendor_init(INIT_PHASE phase)
{
    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            /* register shell cmd */
            shell_cmd_register("setcartype", uds_shell_set_vehicle_type, "set uds car type");
            shell_cmd_register("setfthw", uds_shell_set_ft_hw, "set uds ft hw");
			shell_cmd_register("ftsetfn", uds_shell_set_ft_pn, "set ft pn");
            break;

        default:
            break;
    }

    return 0;
}

