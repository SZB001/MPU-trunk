#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include <ctype.h>
#include "com_app_def.h"
#include "mid_def.h"
#include "init.h"
#include "log.h"
#include "shell_api.h"
#include "tcom_api.h"
#include "fota.h"
#include "xml.h"

extern int fota_uds_open(int port, int fid, int rid, int pid);
extern int fota_uds_get_version_gw(uint8_t *s_ver,           int *s_siz, 
                                         uint8_t *h_ver,    int *h_siz, 
                                         uint8_t *bl_ver,   int *bl_siz, 
                                         uint8_t *sn,       int *sn_siz,
                                         uint8_t *partnum,  int *partnum_siz,
                                         uint8_t *supplier, int *supplier_siz);
extern int fota_uds_get_version(uint8_t *s_ver,           int *s_siz, 
                                      uint8_t *h_ver,    int *h_siz, 
                                      uint8_t *bl_ver,   int *bl_siz, 
                                      uint8_t *sn,       int *sn_siz,
                                      uint8_t *partnum,  int *partnum_siz,
                                      uint8_t *supplier, int *supplier_siz);
extern void fota_uds_close(void);
extern int fota_uds_req_download(uint32_t addr, int size);
extern int fota_uds_trans_data(uint8_t *data, int size);
extern int fota_uds_trans_exit(void);
extern int fota_uds_prog_prepare(void);
extern int fota_uds_enter_diag(void);
extern int fota_uds_req_seed(int lvl, uint8_t *buf, int size);
extern int fota_uds_send_key(int lvl, uint8_t *key, int len);
extern int fota_uds_prog_post(void);
extern int fota_uds_reset(void);
extern int fota_uds_enter_diag_GW(void);
extern int fota_uds_write_data_by_identifier(uint8_t *identifier, uint8_t *data, int len);
extern int fota_uds_write_data_by_identifier_ex(uint8_t *identifier, uint8_t *data, int len);

#define FOTA_MAX_ECU_NAME_LEN     (16)

typedef struct
{
    unsigned char au8ECUName[FOTA_MAX_ECU_NAME_LEN];
    unsigned int  u32PhyID;
    unsigned int  u32ResID;
    unsigned int  u32FunID;
}ECU_NAME_2_UDSID_t;

static ECU_NAME_2_UDSID_t s_atECUName2UDSID[] = {{"vcu",  0x7E2, 0x7EA, 0x7DF},
                                                 {"bms",  0x706, 0x716, 0x7DF},
                                                 {"mcup", 0x707, 0x717, 0x7DF},
                                                 {"obcp", 0x70A, 0x71A, 0x7DF},
                                                 {"flr",  0x7C1, 0x7D1, 0x7DF},
                                                 {"flc",  0x7C2, 0x7D2, 0x7DF},
                                                 {"apa",  0x7C0, 0x7D0, 0x7DF},
                                                 {"esc",  0x720, 0x730, 0x7DF},
                                                 {"eps",  0x724, 0x734, 0x7DF},
                                                 {"ehb",  0x722, 0x732, 0x7DF},
                                                 {"bdcm", 0x740, 0x750, 0x7DF},
                                                 {"lsa",  0x763, 0x773, 0x7DF},
                                                 {"clm",  0x74B, 0x75B, 0x7DF},
                                                 {"ptc",  0x765, 0x775, 0x7DF},
                                                 {"eacp", 0x74C, 0x75C, 0x7DF},
                                                 {"egsm", 0x70B, 0x71B, 0x7DF},
                                                 {"alm",  0x766, 0x776, 0x7DF},
                                                 {"wpc",  0x786, 0x796, 0x7DF},
                                                 {"ihu",  0x780, 0x790, 0x7DF},
                                                 {"icu",  0x781, 0x791, 0x7DF},
                                                 {"isr",  0x783, 0x793, 0x7DF},
                                                 {"dvr",  0x787, 0x797, 0x7DF},
                                                 {"tap",  0x785, 0x795, 0x7DF},
                                                 {"mfcp", 0x782, 0x792, 0x7DF},
                                                 {"acu",  0x746, 0x756, 0x7DF},
                                                 {"plg",  0x764, 0x774, 0x7DF},};

int fota_ecu_get_ver(unsigned char *name, char *s_ver,    int *s_siz, 
                                               char *h_ver,    int *h_siz,
                                               char *bl_ver,   int *bl_siz,
                                               char *sn,       int *sn_siz,
                                               char *partnum,  int *partnum_siz,
                                               char *supplier, int *supplier_siz)
{
    if (memcmp(name, "gw", 2) == 0)
    {
        if (fota_uds_open(0, 0x7DF, 0x772, 0x762) != 0)
        {
            log_e(LOG_FOTA, "open UDS for ECU(%s) fail", name);
            return -1;
        }

        fota_uds_get_version_gw((uint8_t *)s_ver,    s_siz, 
                                (uint8_t *)h_ver,    h_siz, 
                                (uint8_t *)bl_ver,   bl_siz, 
                                (uint8_t *)sn,       sn_siz,
                                (uint8_t *)partnum,  partnum_siz,
                                (uint8_t *)supplier, supplier_siz);
    }
    else
    {
        unsigned char u8MaxECUCount = sizeof(s_atECUName2UDSID) / sizeof(ECU_NAME_2_UDSID_t);
        unsigned char u8Loop = 0;

        for(u8Loop = 0; u8Loop < u8MaxECUCount; u8Loop++)
        {
            if(memcmp(name, s_atECUName2UDSID[u8Loop].au8ECUName, strlen((char *)s_atECUName2UDSID[u8Loop].au8ECUName)) == 0)
            {
                log_o(LOG_FOTA, "Get ECU Information: name %s, PID = 0x%x, RID = 0x%x,", s_atECUName2UDSID[u8Loop].au8ECUName, 
                                                                                         s_atECUName2UDSID[u8Loop].u32PhyID,
                                                                                         s_atECUName2UDSID[u8Loop].u32ResID);
                break;
            }
        }

        if(u8Loop < u8MaxECUCount)
        {
            if (fota_uds_open(1, s_atECUName2UDSID[u8Loop].u32FunID, 
                                 s_atECUName2UDSID[u8Loop].u32ResID, 
                                 s_atECUName2UDSID[u8Loop].u32PhyID) != 0)
            {
                log_e(LOG_FOTA, "open UDS for ECU(%s) fail", name);
                return -1;
            }
            
            fota_uds_get_version((uint8_t *)s_ver,    s_siz, 
                                 (uint8_t *)h_ver,    h_siz, 
                                 (uint8_t *)bl_ver,   bl_siz, 
                                 (uint8_t *)sn,       sn_siz,
                                 (uint8_t *)partnum,  partnum_siz,
                                 (uint8_t *)supplier, supplier_siz);
        }
        else
        {
            log_e(LOG_FOTA, "Can Not Find ECU(%s) Information UDS.", name);
            return -1;
        }
    }

    fota_uds_close();

    return 0;
}

static int fota_ecu_check(fota_ecu_t *ecu)
{
    //char ecu_v[16];

    //if (fota_ecu_get_ver(ecu, ecu_v, sizeof(ecu_v)) != 0)
    //{
    //    log_e(LOG_FOTA, "get ECU(%s) version fail", ecu->name);
    //    return -1;
    //}

    //if (fota_ecu_cmp_ver(ecu, ecu_v) != 0)
    //{
    //    log_e(LOG_FOTA, "ECU(%s) version is not matched", ecu->name);
    //    return -1;
    //}

    return 0;
}

static int fota_ecu_download(fota_ver_t *ver, int erase)
{
    static image_t img;
    static char fpath[512];
    int i;

    strcpy(fpath, ver->ecu->fota->root);
    strcat(fpath, ver->fpath);

    if (ver->load(fpath, &img) != 0)
    {
        log_e(LOG_FOTA, "load image \"%s\" fail", fpath);
        return -1;
    }

    #if 1

    for (i = 0; erase && ver->ecu->erase && i < IMAGE_MAX_SECT && img.sect[i].size; i++)
    {
        log_o(LOG_FOTA, "erase section %d, addr=%X, size=%d", i + 1, img.sect[i].addr, img.sect[i].size);

        if (ver->ecu->erase(img.sect[i].addr, img.sect[i].size) != 0)
        {
            log_e(LOG_FOTA, "erase fail");
            return -1;
        }
    }

    #else

    if (fota_uds_request(0, 0x31, 0x01, "\xff\x0", 2, 10) != 0)
    {
        log_e(LOG_FOTA, "erase fail");
        return -1;
    }

    #endif

    for (i = 0; i < IMAGE_MAX_SECT && img.sect[i].size; i++)
    {
        uint8_t *data = img.sect[i].data;
        int dlen = img.sect[i].size, plen;

        log_o(LOG_FOTA, "program section %d, addr=%X, size=%d", i + 1, img.sect[i].addr, img.sect[i].size);

        if ((plen = fota_uds_req_download(img.sect[i].addr, dlen)) <= 0)
        {
            log_e(LOG_FOTA, "request download for section %d fail", i + 1);
            return -1;
        }

        while (dlen > 0)
        {
            int tlen = dlen > plen ? plen : dlen;
            log_o(LOG_FOTA, "trans data, tlen = %d", tlen);

            if (fota_uds_trans_data(data, tlen) != 0)
            {
                log_e(LOG_FOTA, "transfer data for section %d fail", i + 1);
                return -1;
            }

            dlen -= tlen;
            data += tlen;
        }

        if (fota_uds_trans_exit() != 0)
        {
            log_e(LOG_FOTA, "transfer exit for section %d fail", i + 1);
            return -1;
        }
    }

    if (ver->ecu->check2 && ver->ecu->check2() != 0)
    {
        log_e(LOG_FOTA, "check download fail");
        return -1;
    }

    return 0;
}

static int fota_ecu_upgrade_GW(fota_ecu_t *ecu, fota_ver_t *ver)
{
    uint8_t seed[128], key[128];
    uint8_t buf[2];
    int keysz = 4;

    if (fota_uds_enter_diag_GW() != 0)
    {
        log_e(LOG_FOTA, "enter program session for ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_uds_req_seed(ecu->key_lvl, seed, sizeof(seed)) != 0)
    {
        log_e(LOG_FOTA, "can't get seed from ECU(%s)", ecu->name);
        return -1;
    }

    if (ecu->security && (keysz = ecu->security(seed, ecu->key_par, key, sizeof(key))) < 0)
    {
        log_e(LOG_FOTA, "calculate key for ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_uds_send_key(ecu->key_lvl, key, keysz) != 0)
    {
        log_e(LOG_FOTA, "send key to ECU(%s) fail", ecu->name);
        return -1;
    }

    buf[0] = ecu->gw_sa;
    
	//If Have Flash Diver To Be Send, We Start Progress
	if(ecu->drv.valid)
	{
		buf[1] = '\x00';
	    if (fota_uds_write_data_by_identifier((uint8_t *)"\x74\x00", buf, 2) != 0)
	    {
	        log_e(LOG_FOTA, "write data by identifier 74 00 27 00 to ECU(%s) fail", ecu->name);
	        return -1;
	    }

	    if (fota_ecu_download(&ecu->drv, 0) != 0)
	    {
	        log_e(LOG_FOTA, "download flash driver for ECU(%s) fail", ecu->name);
	        return -1;
	    }
	}

    buf[1] = '\x01';

    if (fota_uds_write_data_by_identifier((uint8_t *)"\x74\x00", buf, 2) != 0)
    {
        log_e(LOG_FOTA, "write data by identifier 74 00 27 01 to ECU(%s) fail", ecu->name);
        return -1;
    }

    if (fota_ecu_download(ver, 0) != 0)
    {
        log_e(LOG_FOTA, "download image(%s) to ECU(%s) fail", ver->fpath, ecu->name);
        return -1;
    }

	//If Have Calc To Be Send, We Start Progress
	if(ecu->cal.valid)
	{
		buf[1] = '\x02';
	    if (fota_uds_write_data_by_identifier((uint8_t *)"\x74\x00", buf, 2) != 0)
	    {
	        log_e(LOG_FOTA, "write data by identifier 74 00 27 00 to ECU(%s) fail", ecu->name);
	        return -1;
	    }

	    if (fota_ecu_download(&ecu->cal, 0) != 0)
	    {
	        log_e(LOG_FOTA, "download flash driver for ECU(%s) fail", ecu->name);
	        return -1;
	    }
	}

    if (fota_uds_write_data_by_identifier_ex((uint8_t *)"\x74\x01", (uint8_t *)&ecu->gw_sa, 1) != 0)
    {
        log_e(LOG_FOTA, "write data by identifier 74 01 to ECU(%s) fail", ecu->name);
        return -1;
    }

    if (0x62 == ecu->gw_sa)
    {
        fota_uds_write_data_by_identifier((uint8_t *)"\x74\x02", (uint8_t *)&ecu->gw_sa, 1);

        fota_uds_reset();
    }

    //if (fota_uds_write_data_by_identifier((uint8_t *)"\x74\x03", (uint8_t *)&ecu->gw_sa, 1) != 0)
    //{
    //    log_e(LOG_FOTA, "write data by identifier 74 03 to ECU(%s) fail", ecu->name);
    //    return -1;
    //}

    return 0;

}


static int fota_ecu_resolve(fota_ecu_t *ecu)
{
    //int i;

    //char ecu_v[16];

    //if (fota_ecu_get_ver(ecu, ecu_v, sizeof(ecu_v)) != 0)
    //{
    //    log_e(LOG_FOTA, "get ECU(%s) version fail", ecu->name);
    //    return -1;
    //}

    //for (i = 0; i < FOTA_MAX_REL_VER && ecu->rel[4].valid; i++)
    //{
    //    if (fota_ecu_cmp_ver(ecu, ecu_v) < 0)
    //    {
    //        if (fota_ecu_upgrade(ecu, ecu->rel + i) != 0)
    //        {
    //            log_e(LOG_FOTA, "update related version %d for ECU(%s) fail", i + 1, ecu->name);
    //            return -1;
    //        }
    //    }
    //}

    return 0;
}

void fota_show(fota_t *fota)
{
    int i;

    shellprintf(" FOTA\r\n");
    shellprintf("   name        : %s\r\n", fota->name);
    shellprintf("   vehicle     : %s\r\n", fota->vehi);
    shellprintf("   descrip     : %s\r\n", fota->desp);
    shellprintf("   version     : %s\r\n", fota->ver);
    shellprintf("   function ID : 0x%X\r\n", fota->fid);

    for (i = 0; i < FOTA_MAX_ECU && fota->ecu[i].valid; i++)
    {
        int j;
        fota_ecu_t *ecu = fota->ecu + i;

        shellprintf("   ECU %d\r\n", i + 1);
        shellprintf("     name            : %s\r\n", ecu->name);

        if (memcmp(ecu->name, "gw", 2) == 0)
        {
            shellprintf("     sa              : %d\r\n", ecu->gw_sa);
        }

        shellprintf("     physical ID     : 0x%X\r\n", ecu->pid);
        shellprintf("     response ID     : 0x%X\r\n", ecu->rid);
        shellprintf("     source version  : %s\r\n", ecu->src.ver);
        shellprintf("     flash version   : %s\r\n", ecu->tar.ver);
        shellprintf("     flash driver    : %s\r\n", ecu->drv.ver);

        for (j = 0; j < FOTA_MAX_REL_VER && ecu->rel[j].valid; j++)
        {
            shellprintf("     related version : %s\r\n", ecu->rel[j].ver);
        }

        shellprintf("     security level  : %d\r\n", ecu->key_lvl);
    }

}

extern unsigned char ecuName[10];

int fota_excute(fota_t *fota)
{
    int i;

    for (i = 0; i < FOTA_MAX_ECU && fota->ecu[i].valid; i++)
    {
        fota_ecu_t *ecu = fota->ecu + i;

        if (!ecu->tar.valid)
        {
            log_e(LOG_FOTA, "target version of ECU(%s) is not found", ecu->name);
            return -1;
        }

        if (fota_uds_open(ecu->can_port, fota->fid, ecu->rid, ecu->pid) != 0)
        {
            log_e(LOG_FOTA, "open UDS for ECU(%s) fail", ecu->name);
            return -1;
        }

        if (fota_ecu_check(ecu) != 0)
        {
            log_e(LOG_FOTA, "check ECU(%s) fail", ecu->name);
            fota_uds_close();
            return -1;
        }

        if (fota_ecu_resolve(ecu) != 0)
        {
            log_e(LOG_FOTA, "resolve related version of ECU(%s) fail", ecu->name);
            fota_uds_close();
            return -1;
        }

        if (memcmp(ecu->name, "gw", 2) == 0)
        {
            memcpy(ecuName, "gw", 2);

            if (fota_ecu_upgrade_GW(ecu, &ecu->tar) != 0)
            {
                log_e(LOG_FOTA, "upgrade ECU(%s) fail", ecu->name);
                fota_uds_close();
                return -1;
            }
        }

        fota_uds_close();
    }

    return 0;
}
