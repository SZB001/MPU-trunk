
#include <stdio.h>
#include "com_app_def.h"
#include "log.h"
#include "tcom_api.h"
#include "shell_api.h"
#include "cfg_api.h"
#include "gps_api.h"
#include "can_api.h"
#include "pm_api.h"
#include "nm_api.h"
#include "at.h"
#include "scom.h"
#include "dev.h"
#include "assist.h"
#include "sock_api.h"
#include "gb32960_api.h"
//#include "foton_api.h"
#include "dsu_api.h"
#include "uds.h"
#include "fct.h"
#include "timer.h"
#include "file.h"
#include "ftp_api.h"
#include "fota_api.h"
//#include "geelyhu_api.h"
//#include "ap_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "tbox_ivi_api.h"
#include "uds_node_miss.h"
#include "remote_diag_api.h"
#include "ble.h"
#include "wsrv_api.h"
#include "udef_cfg_api.h"

int main(int argc , char **argv)
{
    int ret;
    int i, j;
    typedef int (*module_init_fn)(INIT_PHASE phase);
    typedef int (*module_run_fn)(void);

    prctl(PR_SET_NAME, "MAIN");
    signal(SIGPIPE, SIG_IGN);
    static module_init_fn init_tbl[] =
    {
        tcom_init,
        scom_init,
        udef_cfg_init,
        cfg_init,
        gps_init,
        can_init,
        shell_init,
        nm_init,
        at_init,
        dev_init,
        pm_init,
        sock_init,
        ftp_init,
        assist_init,
        gb_init,
        //ft_init,
        dsu_init,
        uds_init,
        fct_init,
        fota_init,
        //hu_init,
        //ap_init,
		PrvtProt_init,
		//sockproxy_init,
		//ivi_init,
		uds_node_miss_init,/* add by caoml*/
		remote_diag_init,/* add by caoml*/
		ble_init,
		wsrv_init,
    };

    static module_run_fn run_tbl[] =
    {
        tcom_run,
        scom_run,
        cfg_run,
        gps_run,
        can_run,
        shell_run,
        nm_run,
        at_run,
        dev_run,
        pm_run,
        assist_run,
        gb_run,
        //ft_run,
        dsu_run,
        uds_run,
        fct_run,
        fota_run,
        //hu_run,
        //ap_run,
		//PrvtProt_run,
		//sockproxy_run,
		//ivi_run,
		uds_node_miss_run,/* add by caoml*/
		remote_diag_run,/* add by caoml*/
		ble_run,
		wsrv_run,
    };

    log_init();
    tm_set_base(tm_get_systick());

    for (j = 0; j < INIT_PHASE_COUNT; j++)
    {
        for (i = 0; i < sizeof(init_tbl) / sizeof(module_init_fn); i++)
        {
            ret = (init_tbl[i])(j);

            if (ret != 0)
            {
                log_e(LOG_MAIN, "module(%u) init in phase(%u) failed,ret:%u",  i, j, ret);
                return -1;
            }

            log_o(LOG_MAIN, "module(%u) init in phase(%u) successfully", i, j);
        }
    }

    for (i = 0; i < sizeof(run_tbl) / sizeof(module_run_fn); i++)
    {
        ret = (run_tbl[i])();

        if (ret != 0)
        {
            log_e(LOG_MAIN, "module(%u) startup failed,ret:%u",  i, ret);
            return -1;
        }

        log_o(LOG_MAIN, "module(%u) startup successfully", i);
    }

    while (1)
    {
        sleep(0xffffffff);
    }

    return 0;
}
