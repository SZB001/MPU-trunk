#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "dev_api.h"
#include "at.h"
#include "at_api.h"
#include "shell_api.h"
#include "hu.h"
#include "fota_api.h"

enum
{
    HU_FOTA_STAT_IDLE,
    HU_FOTA_STAT_DLD_REQUEST,
    HU_FOTA_STAT_DLD_RESTART,
    HU_FOTA_STAT_DLD_CONFIRM,
    HU_FOTA_STAT_DLD_PROCESS,
    HU_FOTA_STAT_DLD_FINISH,
    HU_FOTA_STAT_UPD_REQUEST,
    HU_FOTA_STAT_UPD_CONFIRM,
    HU_FOTA_STAT_UPD_PROCESS,
    HU_FOTA_STAT_UPD_FINISH,
    HU_FOTA_STAT_UPD_WARN,
    HU_FOTA_STAT_UPD_ROLLUPD,
};

static int hu_fota_state = 0;
static uint64_t hu_fota_time = 0;
static int hu_fota_rollback_state = 0;

void hu_fota_upd_warn_start(void)
{
    hu_fota_state = HU_FOTA_STAT_UPD_WARN;
}

void hu_fota_upd_rollupd_reslut_state(int error)
{
    if (error)
    {
        log_e(LOG_FOTA, "rollback failed for some ECU(%d)!",error);
        hu_fota_rollback_state = 1;
    }
    else
    {
        hu_fota_rollback_state = 0;
    }

    hu_fota_state = HU_FOTA_STAT_UPD_ROLLUPD;
}

static int hu_fota_dld_request_send(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_IDLE && fota_state() == FOTA_STAT_NEWREQ)
    {
        hu_fota_state = HU_FOTA_STAT_DLD_REQUEST;
        pack->len = 1;
        pack->dat[0] = 0x00;

        return 0;
    }

    if (hu_fota_state == HU_FOTA_STAT_DLD_CONFIRM && tm_get_time() - 
    hu_fota_time > 65000)
    {
        hu_fota_state = HU_FOTA_STAT_IDLE;
        fota_dld_cancel();
    }

    return -1;
}


static void hu_fota_dld_request_done(int error)
{
    if (!error)
    {
        hu_fota_state = HU_FOTA_STAT_DLD_CONFIRM;
        hu_fota_time  = tm_get_time();
    }
    else
    {
        hu_fota_state = HU_FOTA_STAT_IDLE;
        fota_dld_cancel();
    }
}

static int hu_fota_dld_confirm_ack(hu_pack_t *pack)
{
    pack->len = 0;
    return 0;
}

static int hu_fota_dld_confirm_recv(hu_pack_t *pack)
{
    if (hu_fota_state == HU_FOTA_STAT_DLD_CONFIRM)
    {
        if (pack->dat[0] == 0)
        {
            hu_fota_state = fota_dld_start() == 0 ?
                HU_FOTA_STAT_DLD_PROCESS : HU_FOTA_STAT_DLD_FINISH;
        }
        else
        {
            hu_fota_state = HU_FOTA_STAT_IDLE;
            fota_dld_cancel();
        }
    }

    return 0;
}

static int hu_fota_dld_process_send(hu_pack_t *pack)
{    
    if (hu_fota_state == HU_FOTA_STAT_DLD_PROCESS)
    {
        int percent = fota_dld_percent();
        
        if((percent > 100) || (percent < 0))
        {
            hu_fota_state = HU_FOTA_STAT_DLD_FINISH;
        }
        else
        {
            pack->len = 1;
            pack->dat[0] = percent;
            return 0;
        }
    }

    return -1;
}

static int hu_fota_dld_finish_send(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_DLD_FINISH)
    {
        pack->len = 1;
        pack->dat[0] = fota_state() == FOTA_STAT_NEWFILE ? 0 : 1;
        return 0;
    }

    return -1;
}


static void hu_fota_dld_finish_done(int error)
{
    hu_fota_state = HU_FOTA_STAT_IDLE;
}

static int hu_fota_dld_restart_ack(hu_pack_t *pack)
{
    pack->len = 0;
    return 0;
}

static int hu_fota_dld_restart_recv(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_IDLE && fota_state() == FOTA_STAT_PENDING)
    {
        if (pack->dat[0] == 0)
        {
            hu_fota_state = fota_dld_start() == 0 ?
                HU_FOTA_STAT_DLD_PROCESS : HU_FOTA_STAT_DLD_FINISH;
        }
    }

    return 0;    
}


static hu_cmd_t hu_cmd_fota_dld_request =
{
    .cmd     = HU_CMD_FOTA_DLD_REQUEST,
    .uptype  = HU_CMD_TYPE_NEEDACK,
   // .timeout = 150,
    .timeout = 3000,
    .send_proc = hu_fota_dld_request_send,
    .done_proc = hu_fota_dld_request_done,
};


static hu_cmd_t hu_cmd_fota_dld_confirm =
{
    .cmd     = HU_CMD_FOTA_DLD_CONFIRM,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
  //  .timeout = 500,
   .timeout = 3000,
    .send_proc = hu_fota_dld_confirm_ack,
    .recv_proc = hu_fota_dld_confirm_recv,
};


static hu_cmd_t hu_cmd_fota_dld_process =
{
    .cmd     = HU_CMD_FOTA_DLD_PROCESS,
    .uptype  = HU_CMD_TYPE_PERIOD,
    .period  = 2000,
    .send_proc = hu_fota_dld_process_send,
};


static hu_cmd_t hu_cmd_fota_dld_finish =
{
    .cmd     = HU_CMD_FOTA_DLD_FINISH,
    .uptype  = HU_CMD_TYPE_NEEDACK,
  //  .timeout = 500,
   .timeout = 3000,
    .send_proc = hu_fota_dld_finish_send,
    .done_proc = hu_fota_dld_finish_done,
};


static hu_cmd_t hu_cmd_fota_dld_restart =
{
    .cmd     = HU_CMD_FOTA_DLD_RESTART,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
   // .timeout = 500,
    .timeout = 3000,
    .send_proc = hu_fota_dld_restart_ack,
    .recv_proc = hu_fota_dld_restart_recv,
};


static int hu_cmd_fota_upd_request_ack(hu_pack_t *pack)
{
    pack->len = 1;
    pack->dat[0] = 0;
    return 0;
}

static int hu_cmd_fota_upd_request_recv(hu_pack_t *pack)
{
    if (hu_fota_state == HU_FOTA_STAT_IDLE && (pack->dat[0] == 0 || pack->dat[0] == 1))
    {
        hu_fota_state = fota_upd_start() == 0 ?
            HU_FOTA_STAT_UPD_PROCESS : HU_FOTA_STAT_UPD_FINISH;
    }

    return 0;
}

static int hu_cmd_fota_upd_process_send(hu_pack_t *pack)
{
    if (hu_fota_state == HU_FOTA_STAT_UPD_PROCESS)
    {
        int percent = fota_upd_percent();
        
        if((percent > 100) || (percent < 0))
        {
            hu_fota_state = HU_FOTA_STAT_UPD_FINISH;
        }
        else
        {
            pack->len = 1;
            pack->dat[0] = percent;
            return 0;
        }
    }

    return -1;
}
extern void fota_selfupgrade_finish_status(void);
void hu_cmd_fota_selupgrade_finish(void)
{
  hu_fota_state = HU_FOTA_STAT_UPD_FINISH;
  fota_selfupgrade_finish_status();
}

static int hu_cmd_fota_upd_finish_send(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_UPD_FINISH)
    {
        pack->len = 1;
        pack->dat[0] = fota_state() == FOTA_STAT_FINISH ? 0 : 1;
        return 0;
    }

    return -1;
}

static void hu_cmd_fota_upd_finish_done(int error)
{   
    hu_fota_state = HU_FOTA_STAT_IDLE;
}

static int hu_fota_upd_warn_send(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_UPD_WARN)
    {
        pack->len = 1;
        pack->dat[0] = 0;
        return 0;
    }
    return -1;
}

static void hu_fota_upd_warn_done(int error)
{
    hu_fota_state = HU_FOTA_STAT_IDLE;
    sleep(2);
}

static int hu_cmd_fota_rollupd_finish_send(hu_pack_t *pack)
{
    if(hu_fota_state == HU_FOTA_STAT_UPD_ROLLUPD)
    {
        pack->len = 1;
        pack->dat[0] = hu_fota_rollback_state;
        return 0;
    }
    return -1;
}

static void hu_cmd_fota_rollupd_finish_done(int error)
{
    hu_fota_state = HU_FOTA_STAT_IDLE;
    hu_fota_rollback_state = 0;
}

static hu_cmd_t hu_cmd_fota_upd_request =
{
    .cmd     = HU_CMD_FOTA_UPD_REQUEST,
    .dwtype  = HU_CMD_TYPE_NEEDACK,
    .send_proc = hu_cmd_fota_upd_request_ack,
    .recv_proc = hu_cmd_fota_upd_request_recv,
};


static hu_cmd_t hu_cmd_fota_upd_process =
{
    .cmd     = HU_CMD_FOTA_UPD_PROCESS,
    .uptype  = HU_CMD_TYPE_PERIOD,
    .period    = 2000,
    .send_proc = hu_cmd_fota_upd_process_send,
};


static hu_cmd_t hu_cmd_fota_upd_finish =
{
    .cmd     = HU_CMD_FOTA_UPD_FINISH,
    .uptype  = HU_CMD_TYPE_NEEDACK,
  //  .timeout = 150,
   .timeout = 3000,
    .send_proc = hu_cmd_fota_upd_finish_send,
    .done_proc = hu_cmd_fota_upd_finish_done,
};

static hu_cmd_t hu_cmd_fota_upd_warn =
{
    .cmd     = HU_CMD_FOTA_UPD_WARN,
    .uptype  = HU_CMD_TYPE_NEEDACK,
   // .timeout = 150,
    .timeout = 3000,
    .send_proc = hu_fota_upd_warn_send,
    .done_proc = hu_fota_upd_warn_done,
};

static hu_cmd_t hu_cmd_fota_rollupd_finish =
{
    .cmd     = HU_CMD_FOTA_ROLLUPD_FINISH,
    .uptype  = HU_CMD_TYPE_NEEDACK,
   // .timeout = 150,
    .timeout = 3000,
    .send_proc = hu_cmd_fota_rollupd_finish_send,
    .done_proc = hu_cmd_fota_rollupd_finish_done,
};

static int hu_fota_test(int argc, const char **argv)
{
    int mode = 0;
    int rollback_state = 0;

    if (argc > 0)
    {
        sscanf(argv[0], "%d", &mode);
    }
    printf("argc = %d, %s, %d\r\n", argc, argv[0], mode);
    if(mode == 2) {
        hu_fota_upd_warn_start();
    } else if(mode == 3 || mode == 4) {
        rollback_state = mode % 2;
        hu_fota_upd_rollupd_reslut_state(rollback_state);
    } else {
        fota_new_request(argv[1], NULL, mode);
    }

    return 0;
}

int hu_fota_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= hu_register_cmd(&hu_cmd_fota_dld_request);
            ret |= hu_register_cmd(&hu_cmd_fota_dld_confirm);
            ret |= hu_register_cmd(&hu_cmd_fota_dld_process);
            ret |= hu_register_cmd(&hu_cmd_fota_dld_finish);
            ret |= hu_register_cmd(&hu_cmd_fota_dld_restart);
            ret |= hu_register_cmd(&hu_cmd_fota_upd_request);
            ret |= hu_register_cmd(&hu_cmd_fota_upd_process);
            ret |= hu_register_cmd(&hu_cmd_fota_upd_finish);
            ret |= hu_register_cmd(&hu_cmd_fota_upd_warn);
            ret |= hu_register_cmd(&hu_cmd_fota_rollupd_finish);
            
            ret |= shell_cmd_register("hufotatst", hu_fota_test, "fota test for HU");
            #if 0
            ret |= shell_cmd_register("hucallnum", hu_xcall_show_callnum, "show Geely HU X-call number");
            ret |= shell_cmd_register("hustartcall", hu_xcall_start_call, "Geely HU X-call start");
            ret |= shell_cmd_register("huhangupcall", hu_xcall_hangup_call, "Geely HU X-call hangup");
            #endif
            break;
    }

    return ret;
}




