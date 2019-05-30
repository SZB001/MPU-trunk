
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include <ctype.h>
#include "init.h"
#include "log.h"
#include "shell_api.h"
#include "fota.h"
#include "xml.h"
#include "fota_foton.h"
#include "fota_api.h"
#include "com_app_def.h"
#include "file.h"

#define FOR_EACH_FOTAECU(ecu, fota) \
    for (ecu = (fota)->ecu; ecu->fota && ecu < (fota)->ecu + FOTA_MAX_ECU; ecu++)
#define FOR_EACH_VEROFLST(ver, lst, max) \
    for (ver = (lst); ver->ecu && ver < (lst) + (max); ver++)
#define FOR_EACH_FOTABUS(bus, fota) \
    for (bus = (fota)->bus; bus->port && bus < (fota)->bus + FOTA_MAX_BUS; bus++)
#define FOR_EACH_BUSECU(ecu, bus) \
    for (ecu = (bus)->ecu; ecu->name[0] && ecu < (bus)->ecu + FOTA_MAX_ECU; ecu++)


    
#define ECU_VALID(ecu)  ((ecu)->fota)
#define VER_VALID(ver)  ((ver)->ecu)

extern void hu_fota_upd_rollupd_reslut_state(int state);

unsigned char tbox_selfupgrade_flag = 0 ;

static int fota_ecu_match(fota_ecu_t *ecu)
{
    char ver[128];
    int  len;
    const char *verstr;
    
    if ((len = ecu->fota->hwver((uint8_t*)ver, sizeof(ver))) <= 0)
    {
        log_e(LOG_FOTA, "get HW version of ECU(%s) fail", ecu->name);
        return -1;
    }

    verstr = ecu->fota->verstr((uint8_t*)ver, len);
    
    if (strcmp(ecu->hw_ver, verstr) != 0)
    {
        log_e(LOG_FOTA, "can't match HW version of ECU(%s), need: %s, read: %s", 
            ecu->name, ecu->hw_ver, verstr);
        return -1;
    }

    if ((len = ecu->fota->swver((uint8_t*)ver, sizeof(ver))) <= 0)
    {
        log_e(LOG_FOTA, "get SW version of ECU(%s) fail", ecu->name);
        return -1;
    }

    verstr = ecu->fota->verstr((uint8_t*)ver, len);
    
    if (strcmp(ecu->src.ver, verstr) != 0)
    {
        log_e(LOG_FOTA, "can't match SW version of ECU(%s), need: %s, read: %s", 
            ecu->name, ecu->src.ver, verstr);
        return -1;
    }
    return 0;
}

static int fota_ecu_program(fota_ecu_t *ecu, fota_ver_t *ver, int erase)
{
    static fota_img_t img;
    static char fpath[256];
    int i;
    img_sect_t *sect;

    if (strlen(ecu->fota->root) + strlen(ver->fpath) >= sizeof(fpath))
    {
        log_e(LOG_FOTA, "file path \"%s\" + \"%s\" is too long(max %d)",
            ecu->fota->root, ver->fpath, sizeof(fpath) - 1);
        return -1;
    }
    
    strcpy(fpath, ecu->fota->root);
    strcat(fpath, ver->fpath);
    
    if (ver->img_load(fpath, &img, ver->img_attr) != 0)
    {
        log_e(LOG_FOTA, "load image file \"%s\" fail", fpath);
        return -1;
    }

    fota_show_img(&img);

    i = 0;
    FOR_EACH_IMGSECT(sect, &img, erase && ecu->fota->erase)
    {
        log_i(LOG_FOTA, "erasing section %d, base=%X, size=%d...", ++i, 
            sect->base, sect->size);
        
        ecu->rback = 1;
        if (ecu->fota->erase(sect->base, sect->size) != 0)
        {
            log_e(LOG_FOTA, "erase fail");
            return -1;
        }
        
    }

    i = 0;
    FOR_EACH_IMGSECT(sect, &img, 1)
    {
        uint8_t *data = sect->data;
        int dlen = sect->size;
        int plen;

        log_i(LOG_FOTA, "programing section %d, base=%X, size=%d...", ++i, 
            sect->base, sect->size);
        
        if ((plen = fota_uds_req_download(sect->base, sect->size)) <= 0)
        {
            log_e(LOG_FOTA, "request download for section %d fail", i);
            return -1;
        }

        while (dlen > 0)
        {
            int tlen = dlen > plen ? plen : dlen, percent;
            log_i(LOG_FOTA, "transfering data, length = %d...", tlen);
            
            if (fota_uds_trans_data(data, tlen) != 0)
            {
                log_e(LOG_FOTA, "transfer data for section %d fail", i);
                return -1;
            }

            dlen -= tlen;
            data += tlen;
            ecu->fota->curr += tlen;
        
            percent = ecu->fota->curr * 100 / ecu->fota->total;
           if(tbox_selfupgrade_flag == 1)
           {
               if(percent >= 90)
               {
                   percent = 90;
               }
           }
 
            if (ecu->fota->callback && ecu->fota->callback(FOTA_EVENT_PROCESS, percent))
            {
                log_e(LOG_FOTA, "transfer is canceled by call back");
                return -1;
            }
        }

        if (fota_uds_trans_exit() != 0)
        {
            log_e(LOG_FOTA, "transfer exit for section %d fail", i);
            return -1;
        }
    }

    if (ecu->fota->check[1] && ecu->fota->check[1]() != 0)
    {
        log_e(LOG_FOTA, "1st check fail");
        return -1;
    }

    if (ecu->fota->check[2] && ecu->fota->check[2]() != 0)
    {
        log_e(LOG_FOTA, "2nd check fail");
        return -1;
    }
        
    return 0;
}

static int fota_upgrade_prepare(fota_ecu_t *ecu)
{
    return (fota_uds_request(1, 0x10, 0x83, NULL, 0, 2) != 0 ||
        (ecu->fota->check[0] && ecu->fota->check[0]() != 0) ||
        fota_uds_request(1, 0x85, 0x82, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x28, 0x83, (uint8_t*)"\x01", 1, 2) != 0);
}

static int fota_upgrade_excute(fota_ecu_t *ecu, fota_ver_t *ver)
{
    uint8_t seed[128], key[128];
    int key_size = 0, seed_size;
    
    if (fota_uds_request(0, 0x10, 0x02, NULL, 0, 3) != 0)
    {
        log_e(LOG_FOTA, "enter program session for ECU(%s) fail", ecu->name);
        return -1;
    }
        
    if (fota_uds_request(0, 0x27, ecu->key_lvl, NULL, 0, 3) != 0 ||
        (seed_size = fota_uds_result(seed, sizeof(seed))) <= 0)
    {
        log_e(LOG_FOTA, "request seed in level %d failed", ecu->key_lvl);
        return -1;
    }
    
    if (ecu->fota->security && 
        (key_size = ecu->fota->security(seed, ecu->key_par, key, sizeof(key))) <= 0)
    {
        log_e(LOG_FOTA, "calculate key for ECU(%s) fail", ecu->name);
        return -1;
    }
    
    if (fota_uds_request(0, 0x27, ecu->key_lvl + 1, key, key_size, 3) != 0)
    {
        log_e(LOG_FOTA, "send key to ECU(%s) fail", ecu->name);
        return -1;
    }
    
    if (VER_VALID(&ecu->drv) && fota_ecu_program(ecu, &ecu->drv, 0) != 0)
    {
        log_e(LOG_FOTA, "download flash driver to ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_ecu_program(ecu, ver, 1) != 0)
    {
        log_e(LOG_FOTA, "download file \"%s\" to ECU(%s) fail", ver->fpath, ecu->name);
        return -1;
    }

    return 0;
}

static int fota_upgrade_finish(fota_ecu_t *ecu)
{
    return (fota_uds_request(0, 0x11, 0x01, NULL, 0, 10) != 0 ||
        fota_uds_request(1, 0x10, 0x83, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x28, 0x80, (uint8_t*)"\x03", 1, 2) != 0 ||
        fota_uds_request(1, 0x85, 0x81, NULL, 0, 2) != 0 ||
        fota_uds_request(1, 0x10, 0x81, NULL, 0, 2) != 0 ||
        fota_uds_request(0, 0x14, 0, (uint8_t *)"\xff\xff\xff", 3, 3) != 0);
}

static int fota_ecu_upgrade(fota_ecu_t *ecu, fota_ver_t *ver)
{
    if (fota_upgrade_prepare(ecu) != 0)
    {
        log_e(LOG_FOTA, "upgrade step 1 for ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_upgrade_excute(ecu, ver) != 0)
    {
        log_e(LOG_FOTA, "upgrade step 2 for ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_upgrade_finish(ecu) != 0)
    {
        log_e(LOG_FOTA, "upgrade step 3  for ECU(%s) fail", ecu->name);
        return -1;
    }
    
    return 0;
    
}

static int fota_ecu_resolve(fota_ecu_t *ecu)
{
    char ver[128];
    int  len;
    fota_ver_t *relv;
    const char *verstr;

    if ((len = ecu->fota->swver((uint8_t*)ver, sizeof(ver))) <= 0)
    {
        log_e(LOG_FOTA, "get SW version of ECU(%s) fail", ecu->name);
        return -1;
    }

    verstr = ecu->fota->verstr((uint8_t*)ver, len);

    FOR_EACH_VEROFLST(relv, ecu->rel, FOTA_MAX_REL_VER)
    {
        if (strcmp(verstr, relv->ver) >= 0)
        {
            continue;
        }
        
        log_o(LOG_FOTA, "upgrade ECU(%s) from \"%s\" to \"\%s\"", verstr, relv->ver);

        if (fota_ecu_upgrade(ecu, relv) != 0)
        {
            log_e(LOG_FOTA, "upgrade ECU(%s) to \"%s\" fail", relv->ver, ecu->name);
            return -1;
        }
    }
    
    return 0;
}


void fota_show_bus(fota_t *fota)
{
    bus_inf_t *bus;
    int i = 0;

    FOR_EACH_FOTABUS(bus, fota)
    {
        ecu_inf_t *ecu;
        int j = 0;
        
        shellprintf(" BUS %d\r\n", ++i);
        shellprintf("   port        : %d\r\n", bus->port);
        shellprintf("   baud rate   : %d\r\n", bus->baud);
        shellprintf("   function id : %X\r\n", bus->fid);
        
        FOR_EACH_BUSECU(ecu, bus)
        {
            shellprintf("   ECU %d\r\n", ++j);
            shellprintf("     name        : %s\r\n", ecu->name);
            shellprintf("     physical id : %X\r\n", ecu->pid);
            shellprintf("     response id : %X\r\n", ecu->rid);
        }
    }
}

void fota_show(fota_t *fota)
{
    int i = 0;
    fota_ecu_t *ecu;
    
    shellprintf(" FOTA\r\n");
    shellprintf("   name        : %s\r\n", fota->name);
    shellprintf("   vehicle     : %s\r\n", fota->vehi);
    shellprintf("   descrip     : %s\r\n", fota->desc);
    shellprintf("   version     : %s\r\n", fota->ver);
    
    FOR_EACH_FOTAECU(ecu, fota)
    {
        fota_ver_t *ver;
        
        shellprintf("   ECU %d\r\n", ++i);
        shellprintf("     name            : %s\r\n", ecu->name);
        shellprintf("     source version  : %s\r\n", ecu->src.ver);
        shellprintf("     flash version   : %s\r\n", ecu->tar.ver);
        shellprintf("     flash driver    : %s\r\n", ecu->drv.ver);

        FOR_EACH_VEROFLST(ver, ecu->rel, FOTA_MAX_REL_VER)
        {
            shellprintf("     related version : %s\r\n", ver->ver);
        }
        shellprintf("     security level  : %d\r\n", ecu->key_lvl);
    }
        
}

static ecu_inf_t* fota_find_ecu_info(fota_t *fota, const char *name)
{
    bus_inf_t *bus;
    
    FOR_EACH_FOTABUS(bus, fota)
    {
        ecu_inf_t *ecu;
        
        FOR_EACH_BUSECU(ecu, bus)
        {
            if (strcmp(ecu->name, name) == 0)
            {
                return ecu;
            }
        }
    }

    return NULL;
}

static bus_inf_t* fota_find_ecu_bus(fota_t *fota, const char *name)
{
    bus_inf_t *bus;
    
    FOR_EACH_FOTABUS(bus, fota)
    {
        ecu_inf_t *ecu;
        
        FOR_EACH_BUSECU(ecu, bus)
        {
            if (strcmp(ecu->name, name) == 0)
            {
                return bus;
            }
        }
    }

    return NULL;
}

static int fota_ver_size(fota_ver_t *ver)
{
    static char fpath[256];

    if (strlen(ver->ecu->fota->root) + strlen(ver->fpath) >= sizeof(fpath))
    {
        log_e(LOG_FOTA, "file path \"%s\" + \"%s\" is too long(max %d)",
            ver->ecu->fota->root, ver->fpath, sizeof(fpath) - 1);
        return -1;
    }
    
    strcpy(fpath, ver->ecu->fota->root);
    strcat(fpath, ver->fpath);

    return ver->img_calc(fpath);
}


static int fota_rel_size(fota_ecu_t *ecu)
{
    int size, total = 0;
    fota_ver_t *relv;

    FOR_EACH_VEROFLST(relv, ecu->rel, FOTA_MAX_REL_VER)
    {
        if (strcmp(ecu->src.ver, relv->ver) >= 0)
        {
            continue;
        }

        if ((size = fota_ver_size(relv)) <= 0)
        {
            log_e(LOG_FOTA, "can't calculate size of \"%s\"", relv->fpath);
            return -1;
        }

        total += size;
    }

    return total;
}

static int fota_calc_workload(fota_t *fota)
{
    int size;
    int tbox_selfupgrade = 0;
    fota_ecu_t *ecu;

    fota->total = 0;
    FOR_EACH_FOTAECU(ecu, fota)
    {
        if (strcmp(ecu->name, "TBOX") == 0)
        {
            tbox_selfupgrade = 1;
            tbox_selfupgrade_flag = 1;
            continue;
        }
        
        size = 0;
        if (VER_VALID(&ecu->drv) && (size = fota_ver_size(&ecu->drv)) <= 0)
        {
            log_e(LOG_FOTA, "can't calculate size of \"%s\"", ecu->drv.fpath);
            return -1;
        }
        fota->total += size;
        
        size = 0;
        if (VER_VALID(&ecu->tar) && (size = fota_ver_size(&ecu->tar)) <= 0)
        {
            log_e(LOG_FOTA, "can't calculate size of \"%s\"", ecu->tar.fpath);
            return -1;
        }
        fota->total += size;

        if ((size = fota_rel_size(ecu)) < 0)
        {
            return -1;
        }
        fota->total += size;
    }

    if (fota->total == 0)
    {
        log_e(LOG_FOTA, "work load is zero");
    }

    return fota->total+tbox_selfupgrade;
}

void fota_rollback(fota_t *fota)
{
    int error = 0;
    fota_ecu_t *ecu;

    log_o(LOG_FOTA, "starting FOTA rollback");
    fota->callback = NULL;
    
    FOR_EACH_FOTAECU(ecu, fota)
    {
        if (ecu->rback)
        {
            bus_inf_t *bus  = fota_find_ecu_bus(fota, ecu->name);
            ecu_inf_t *info = fota_find_ecu_info(fota, ecu->name);
            int baud;

            if (strcmp(ecu->name, "TBOX") == 0)
            {
                continue;
            }
            
            log_o(LOG_FOTA, "starting to rollback ECU(%s) to \"%s\"", ecu->name, ecu->src.ver);        
            sleep(2);
            baud = can_get_baud(bus->port - 1);
            
            if (baud != bus->baud && can_set_baud(bus->port - 1, bus->baud) != 0)
            {
                log_e(LOG_FOTA, "can't set baud rate of bus %d for ECU(%s)", bus->port, ecu->name);
                error++;
                continue;
            }
        
            if (fota_uds_open(bus->port - 1, bus->fid, info->rid, info->pid) != 0)
            {
                log_e(LOG_FOTA, "open UDS for ECU(%s) fail", ecu->name);
                can_set_baud(bus->port - 1, baud);
                error++;
                continue;;
            }
            
            if (fota_ecu_upgrade(ecu, &ecu->src) != 0)
            {
                log_e(LOG_FOTA, "program ECU(%s) fail", ecu->name);
                fota_uds_close();
                can_set_baud(bus->port - 1, baud);
                error++;
                continue;
            }

            fota_uds_close();
            can_set_baud(bus->port - 1, baud);
            log_o(LOG_FOTA, "ECU(%s) rollback done!", ecu->name);
        }
    }

    if (error)
    {
        log_e(LOG_FOTA, "warnning: rollback failed for some ECU!");
    }

    log_o(LOG_FOTA, "FOTA rollback done!");
    //hu_fota_upd_rollupd_reslut_state(error);
}

static int fota_tbox_upgrade(fota_ver_t *ver)
{
    static char fpath[256];

    if (strlen(ver->ecu->fota->root) + strlen(ver->fpath) >= sizeof(fpath))
    {
        log_e(LOG_FOTA, "file path \"%s\" + \"%s\" is too long(max %d)",
            ver->ecu->fota->root, ver->fpath, sizeof(fpath) - 1);
        return -1;
    }
    
    strcpy(fpath, ver->ecu->fota->root);
    strcat(fpath, ver->fpath);

    if (file_copy(fpath, COM_APP_PKG_DIR"/"COM_PKG_FILE) != 0)
    {
        log_e(LOG_FOTA, "copy \"%s\" to \"%s\" fail", fpath, COM_APP_PKG_DIR);
        return -1;
    }

    return shell_cmd_exec("pkgupgrade", NULL, 0);
}

int fota_excute(fota_t *fota, int (*cb)(int , int))
{
    int error = 0;
    fota_ecu_t *ecu;
    fota_ecu_t *tbox = NULL;
    
    log_o(LOG_FOTA, "starting FOTA work");
    
    if (fota_calc_workload(fota) <= 0)
	 //if (fota_calc_workload(fota) < 0)  
    {
        log_e(LOG_FOTA, "calculate workload fail");
        return -1;
    }
    
    log_o(LOG_FOTA, "workload: %d Bytes", fota->total);

    fota->callback = cb;
    FOR_EACH_FOTAECU(ecu, fota)
    {
        bus_inf_t *bus  = fota_find_ecu_bus(fota, ecu->name);
        ecu_inf_t *info = fota_find_ecu_info(fota, ecu->name);
        int baud;

        log_o(LOG_FOTA, "starting upgrade ECU(%s) to \"%s\"", ecu->name, ecu->tar.ver);

        if (strcmp(ecu->name, "TBOX") == 0)
        {
            tbox = ecu;
            continue;
        }

        if (!bus || !info)
        {
            log_e(LOG_FOTA, "can't find ECU(%s) in bus", ecu->name);
            error++;
            break;
        }
        
        if (!VER_VALID(&ecu->tar))
        {
            log_e(LOG_FOTA, "target version of ECU(%s) is not found", ecu->name);
            error++;
            break;
        }

        baud = can_get_baud(bus->port - 1);
        if (baud != bus->baud && can_set_baud(bus->port - 1, bus->baud) != 0)
        {
            log_e(LOG_FOTA, "can't set baud rate of bus %d for ECU(%s)", bus->port, ecu->name);
            error++;
            break;
        }
        
        if (fota_uds_open(bus->port - 1, bus->fid, info->rid, info->pid) != 0)
        {
            log_e(LOG_FOTA, "open UDS for ECU(%s) fail", ecu->name);
            can_set_baud(bus->port - 1, baud);
            error++;
            break;
        }

        if (fota_ecu_match(ecu) != 0)
        {
            fota_uds_close();
            error++;
            break;
        }

        if (fota_ecu_resolve(ecu) != 0)
        {
            log_e(LOG_FOTA, "resolve version-relative of ECU(%s) fail", ecu->name);
            fota_uds_close();
            error++;
            break;
        }
        
        if (fota_ecu_upgrade(ecu, &ecu->tar) != 0)
        {
            log_e(LOG_FOTA, "program ECU(%s) fail", ecu->name);            
            fota_uds_close();
            can_set_baud(bus->port - 1, baud);
            error++;
            break;
        }

        fota_uds_close();
        can_set_baud(bus->port - 1, baud);
        sleep(2);
        log_o(LOG_FOTA, "ECU(%s) upgrading done!", ecu->name);
    }

    if (!error && tbox && (error = fota_tbox_upgrade(&tbox->tar)))
    {
        log_e(LOG_FOTA, "upgrade ECU(tbox) fail");
    }
    
    if (error)
    {
        fota_rollback(fota);
    }
    else    
    {
        log_o(LOG_FOTA, "FOTA work done!");
    }
    
    return error;
}

