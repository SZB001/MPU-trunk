#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "com_app_def.h"
#include "log.h"
#include "fota.h"
#include "xml.h"


#define XML_FOTA_FILE       "/fota_info.xml"

void xml_show(xml_t *xml);

static int xml_load_ver(xml_node_t *ver_node, fota_ver_t *ver)
{
    xml_node_t *node;
    char *p;

    if ((p = xml_get_sub_value(ver_node, "version")) == NULL)
    {
        log_e(LOG_FOTA, "can't find version number");
        return -1;
    }

    strncpy(ver->ver, p, sizeof(ver->ver));

    if ((p = xml_get_sub_value(ver_node, "file/patch")) == NULL)
    {
        log_e(LOG_FOTA, "can't find version file");
        return -1;
    }

    strncpy(ver->fpath, p, sizeof(ver->fpath));

    if ((p = xml_get_sub_value(ver_node, "file/format")) == NULL)
    {
        log_e(LOG_FOTA, "can't find version format");
        return -1;
    }

    if (strcmp(p, "S19") == 0)
    {
        ver->load = s19_load;
    }
    else
    {
        log_e(LOG_FOTA, "unknown version format: %s", p);
        return -1;
    }

    ver->valid = 1;
    return 0;
}

extern int foton_security(uint8_t *seed, int *par, uint8_t *key, int keysz);
extern int foton_erase(uint32_t addr, int size);
extern int foton_check1(unsigned int crc);
extern int foton_check2(void);

int xml_load_ecu(xml_node_t *ecu_node, fota_ecu_t *ecu)
{
    static xml_t xml;
    xml_node_t *node, *relv_lst[FOTA_MAX_REL_VER], *kpar_lst[FOTA_MAX_KEY_PAR];
    char fpath[256], *p;
    int node_cnt, i;

    ecu->tar.ecu = ecu;
    ecu->drv.ecu = ecu;
    ecu->src.ecu = ecu;

    if ((p = xml_get_sub_value(ecu_node, "name")) == NULL)
    {
        log_e(LOG_FOTA, "can't find name of ECU");
        return -1;
    }

    strncpy(ecu->name, p, sizeof(ecu->name) - 1);

    if ((p = xml_get_sub_value(ecu_node, "physicalid")) == NULL ||
        sscanf(p, "%X", &ecu->pid) != 1)
    {
        log_e(LOG_FOTA, "can't find physical ID of ECU");
        return -1;
    }

    if ((p = xml_get_sub_value(ecu_node, "responseid")) == NULL ||
        sscanf(p, "%X", &ecu->rid) != 1)
    {
        log_e(LOG_FOTA, "can't find response ID");
        return -1;
    }

    if ((p = xml_get_sub_value(ecu_node, "systemkey")) != NULL)
    {
        strncpy(ecu->skey, p, sizeof(ecu->skey) - 1);
    }

    if ((node = xml_get_sub_node(ecu_node, "sourcever")) != NULL &&
        xml_load_ver(node, &ecu->src) != 0)
    {
        log_e(LOG_FOTA, "load source version of ECU(%s) fail", ecu->name);
        return -1;
    }

    if ((p = xml_get_sub_value(ecu_node, "infoname")) == NULL)
    {
        log_e(LOG_FOTA, "can't find information file of ECU(%s)", ecu->name);
        return -1;
    }

    strcpy(fpath, ecu->fota->root);
    strcat(fpath, p);

    if (xml_load_file(fpath, &xml) != 0)
    {
        log_e(LOG_FOTA, "load information file \"%s\" fail", ecu->name, fpath);
        return -1;
    }

    if ((ecu_node = xml_get_node(&xml, ecu->name)) == NULL)
    {
        log_e(LOG_FOTA, "can't find information of ECU(%s) in \"%s\"", ecu->name, fpath);
        xml_destroy(&xml);
        return -1;
    }

    if ((node = xml_get_sub_node(ecu_node, "flashver")) == NULL)
    {
        log_e(LOG_FOTA, "can't find flash version of ECU(%s) in \"%s\"", ecu->name, fpath);
        xml_destroy(&xml);
        return -1;
    }


    if (xml_load_ver(node, &ecu->tar) != 0)
    {
        log_e(LOG_FOTA, "load flash version of ECU(%s) fail", ecu->name);
        xml_destroy(&xml);
        return -1;
    }

    if ((node = xml_get_sub_node(ecu_node, "flashdrv")) != NULL &&
        xml_load_ver(node, &ecu->drv) != 0)
    {
        log_e(LOG_FOTA, "load flash driver of ECU(%s) fail", ecu->name);
        xml_destroy(&xml);
        return -1;
    }

    node_cnt = xml_get_sub_node_lst(ecu_node, "relver", relv_lst, FOTA_MAX_REL_VER);

    if (node_cnt > FOTA_MAX_ECU)
    {
        log_e(LOG_FOTA, "too many related version(max %d) of ECU(%s) in \"%s\"",
              FOTA_MAX_REL_VER, ecu->name, fpath);
        xml_destroy(&xml);
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        ecu->rel[i].ecu = ecu;

        if (xml_load_ver(relv_lst[i], ecu->rel + i) != 0)
        {
            log_e(LOG_FOTA, "load related version %d of ECU(%s) fail", i + 1, ecu->name);
            xml_destroy(&xml);
            return -1;
        }
    }

    node_cnt = xml_get_sub_node_lst(ecu_node, "keypar", kpar_lst, FOTA_MAX_KEY_PAR);

    if (node_cnt > FOTA_MAX_ECU)
    {
        log_e(LOG_FOTA, "too many key parameter(max %d) of ECU(%s) in \"%s\"",
              FOTA_MAX_KEY_PAR, ecu->name, fpath);
        xml_destroy(&xml);
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        sscanf(kpar_lst[i]->value, "%X", ecu->key_par + i);
    }

    ecu->gw_sa = 0xDE;

    if ((p = xml_get_sub_value(ecu_node, "gwsa")) != NULL)
    {
        sscanf(p, "%d", &ecu->gw_sa);
    }

    ecu->can_port = 0;

    if ((p = xml_get_sub_value(ecu_node, "canport")) != NULL)
    {
        sscanf(p, "%d", &ecu->can_port);

        if (ecu->can_port > 3)
        {
            ecu->can_port = 0;
            log_e(LOG_FOTA, "CAN port can not set(%d)", ecu->can_port);
        }
    }

    ecu->key_lvl = 1;

    if ((p = xml_get_sub_value(ecu_node, "keylvl")) != NULL)
    {
        sscanf(p, "%d", &ecu->key_lvl);
    }

    ecu->security = foton_security;
    ecu->erase = foton_erase;
    ecu->check1 = foton_check1;
    ecu->check2 = foton_check2;
    ecu->valid = 1;
    xml_destroy(&xml);
    return 0;
}


int xml_load_fota(char *root, fota_t *fota)
{
    static xml_t xml;
    xml_node_t *node, *ecu_lst[FOTA_MAX_ECU];
    char fpath[256], *p = root + strlen(root);
    int ecu_cnt, i;

    while (*(p - 1) == '/')
    {
        *(--p) = 0;
    }

    memset(fota, 0, sizeof(fota_t));
    strcpy(fota->root, root);
    strcpy(fpath, root);
    strcat(fpath, XML_FOTA_FILE);

    if (xml_load_file(fpath, &xml) != 0)
    {
        log_e(LOG_FOTA, "load FOTA information file \"%s\" fail", fpath);
        return -1;
    }

    //xml_show(&xml);

    if ((node = xml_get_node(&xml, "fota")) == NULL)
    {
        log_e(LOG_FOTA, "can't find FOTA information in \"%s\"", fpath);
        xml_destroy(&xml);
        return -1;
    }



    if ((p = xml_get_sub_value(node, "name")) != NULL)
    {
        strncpy(fota->name, p, sizeof(fota->name) - 1);
    }

    if ((p = xml_get_sub_value(node, "vehicle")) != NULL)
    {
        strncpy(fota->vehi, p, sizeof(fota->vehi) - 1);
    }

    if ((p = xml_get_sub_value(node, "description")) != NULL)
    {
        strncpy(fota->desp, p, sizeof(fota->desp) - 1);
    }

    if ((p = xml_get_sub_value(node, "version")) != NULL)
    {
        strncpy(fota->ver, p, sizeof(fota->ver) - 1);
    }

    if ((p = xml_get_sub_value(node, "functionid")) == NULL ||
        sscanf(p, "%X", &fota->fid) != 1)
    {
        log_e(LOG_FOTA, "can't find function ID");
        xml_destroy(&xml);
        return -1;
    }

    if ((node = xml_get_sub_node(node, "strategy")) == NULL)
    {
        log_e(LOG_FOTA, "can't find FOTA strategy in \"%s\"", fpath);
        xml_destroy(&xml);
        return -1;
    }

    ecu_cnt = xml_get_sub_node_lst(node, "ecu", ecu_lst, FOTA_MAX_ECU);

    if (ecu_cnt == 0)
    {
        log_e(LOG_FOTA, "can't find ECU description in \"%s\"", fpath);
        xml_destroy(&xml);
        return -1;
    }

    if (ecu_cnt > FOTA_MAX_ECU)
    {
        log_e(LOG_FOTA, "too many ECU descriptions(max %d) in \"%s\"", FOTA_MAX_ECU, fpath);
        xml_destroy(&xml);
        return -1;
    }

    for (i = 0; i < ecu_cnt; i++)
    {
        fota->ecu[i].fota = fota;

        if (xml_load_ecu(ecu_lst[i], fota->ecu + i) != 0)
        {
            log_e(LOG_FOTA, "load ECU %d information fail", i + 1);
            xml_destroy(&xml);
            return -1;
        }
    }

    xml_destroy(&xml);
    return 0;
}
