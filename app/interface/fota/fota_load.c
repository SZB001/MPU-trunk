#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "fota.h"
#include "xml.h"
#include "fota_foton.h"

#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define XML_FOTA_FILE       "/fota_info.xml"
#define XML_BUS_FILE        "/home/root/foton/bus_info.xml"

static int fota_load_ecu_info(xml_node_t *node, ecu_inf_t *ecu)
{
    char *p;
    
    if ((p = xml_get_sub_value(node, "name")) == NULL)
    {
        log_e(LOG_FOTA, "can't find name of ECU");
        return -1;
    }

    if (strlen(p) >= FOTA_MAX_ECUNAME)
    {
        log_e(LOG_FOTA, "ECU name \"%s\"is too long(max %d)", p, FOTA_MAX_ECUNAME);
        return -1;
    }

    strcpy(ecu->name, p);

    if ((p = xml_get_sub_value(node, "physicalid")) == NULL ||
        sscanf(p, "%x", &ecu->pid) != 1)
    {
        log_e(LOG_FOTA, "can't find physical-id of ECU");
        return -1;
    }

    if ((p = xml_get_sub_value(node, "responseid")) == NULL ||
        sscanf(p, "%x", &ecu->rid) != 1)
    {
        log_e(LOG_FOTA, "can't find response-id of ECU");
        return -1;
    }

    return 0;

}

static int fota_load_bus_info(xml_node_t *node, bus_inf_t *bus)
{
    xml_node_t *node_lst[FOTA_MAX_ECU];
    char *p;
    int node_cnt, i;
    
    if ((p = xml_get_sub_value(node, "port")) == NULL ||
        sscanf(p, "%d", &bus->port) != 1)
    {
        log_e(LOG_FOTA, "can't find port of bus");
        return -1;
    }
    
    if ((p = xml_get_sub_value(node, "baudrate")) == NULL ||
        sscanf(p, "%d", &bus->baud) != 1)
    {
        log_e(LOG_FOTA, "can't find baud rate of bus");
        return -1;
    }

    if ((p = xml_get_sub_value(node, "functionid")) == NULL ||
        sscanf(p, "%x", &bus->fid) != 1)
    {
        log_e(LOG_FOTA, "can't find function-id of bus");
        return -1;
    }

    node_cnt = xml_get_sub_node_lst(node, "ecu", node_lst, FOTA_MAX_ECU);

    if (node_cnt > FOTA_MAX_ECU)
    {
        log_e(LOG_FOTA, "too many ECU(max %d) in bus", FOTA_MAX_ECU);
        return -1;
    }

    if (node_cnt <= 0)
    {
        log_e(LOG_FOTA, "no ECU in bus");
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        if (fota_load_ecu_info(node_lst[i], bus->ecu + i) != 0)
        {
            log_e(LOG_FOTA, "load ECU %d information fail", i + 1);
            return -1;
        }
    }
    
    return 0;
}


static int fota_load_bus_file(char *fpath, bus_inf_t *bus)
{
    xml_t *xml;
    xml_node_t *node, *node_lst[FOTA_MAX_BUS];
    int node_cnt, i;
    
    if ((xml = xml_load(fpath)) == NULL)
    {
        log_e(LOG_FOTA, "load bus information file \"%s\" fail", fpath);
        return -1;
    }

    if ((node = xml_get_node(xml, "businfo")) == NULL)
    {
        log_e(LOG_FOTA, "can't find bus information in \"%s\"", fpath);
        xml_free(xml);
        return -1;
    }

    node_cnt = xml_get_sub_node_lst(node, "bus", node_lst, FOTA_MAX_BUS);

    if (node_cnt <= 0)
    {
        log_e(LOG_FOTA, "can't find bus in \"%s\"", fpath);
        xml_free(xml);
        return -1;
    }

    if (node_cnt > FOTA_MAX_BUS)
    {
        log_e(LOG_FOTA, "too many bus(max %d) in \"%s\"", FOTA_MAX_BUS, fpath);
        xml_free(xml);
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        if (fota_load_bus_info(node_lst[i], bus + i) != 0)
        {
            log_e(LOG_FOTA, "load bus %d fail", i + 1);
            xml_free(xml);
            return -1;
        }
        //printf("bus %d: %d, %d, %x\n", i + 1, bus[i].port, bus[i].baud, bus[i].fid);
    }

    xml_free(xml);
    return 0;
}

static int fota_load_ver(xml_node_t *node, fota_ver_t *ver)
{
    char *p;
    
    if ((p = xml_get_sub_value(node, "version")) == NULL)
    {
        log_e(LOG_FOTA, "can't find version");
        return -1;
    }

    if (strlen(p) >= sizeof(ver->ver))
    {
        log_e(LOG_FOTA, "version is too long, max(%d)", sizeof(ver->ver) - 1);
        return -1;
    }
    
    strcpy(ver->ver, p);

    if ((p = xml_get_sub_value(node, "file/patch")) == NULL)
    {
        log_e(LOG_FOTA, "can't find image file");
        return -1;
    }

    if (p[0] != '/')
    {
        log_e(LOG_FOTA, "file path \"%s\" must start with '/'", p);
        return -1;
    }

    if (strlen(p) >= sizeof(ver->fpath))
    {
        log_e(LOG_FOTA, "image file path \"%s\" is too long, max(%d)", sizeof(ver->fpath) - 1);
        return -1;
    }
    
    strcpy(ver->fpath, p);

    if ((p = xml_get_sub_value(node, "file/format")) == NULL)
    {
        log_e(LOG_FOTA, "can't find image format");
        return -1;
    }
    
    if (strcmp(p, "S19") == 0)
    {
        ver->img_load = (void*)fota_load_s19;
        ver->img_calc = (void*)fota_calc_s19;
    }
    else if (strcmp(p, "S19W2") == 0)
    {
        ver->img_load = (void*)fota_load_s19;
        ver->img_calc = (void*)fota_calc_s19;
        ver->img_attr = 1;
    }
    else if (strcmp(p, "HEX") == 0)
    {        
        ver->img_load = (void*)fota_load_hex;
        ver->img_calc = (void*)fota_calc_hex;
    }
    else if (strcmp(p, "BIN") == 0)
    {
        ver->img_load = (void*)fota_load_bin;
        ver->img_calc = (void*)fota_calc_bin;
        
        if ((p = xml_get_sub_value(node, "file/address")) != NULL)
        {
            sscanf(p, "%X", &ver->img_attr);
        }
    }
    else if (strcmp(p, "PKG") != 0)
    {
        log_e(LOG_FOTA, "unsupported image format: %s", p);
        return -1;
    }

    return 0;
}

static int fota_load_ecu_file(const char *fpath, fota_ecu_t *ecu)
{
    xml_t *xml;
    xml_node_t *ecu_node, *node, *node_lst[MAX(FOTA_MAX_REL_VER, FOTA_MAX_KEY_PAR)];
    char *p;
    int node_cnt, i;

    if ((xml = xml_load(fpath)) == NULL)
    {
        log_e(LOG_FOTA, "load XML file \"%s\" fail", fpath);
        return -1;
    }
    
    if ((ecu_node = xml_get_node(xml, ecu->name)) == NULL)
    {
        log_e(LOG_FOTA, "can't find information of ECU(%s) in \"%s\"", ecu->name, fpath);
        xml_free(xml);
        return -1;
    }

    if ((p = xml_get_sub_value(ecu_node, "hwversion")) == NULL)
    {
        log_e(LOG_FOTA, "can't find hardware version of ECU(%s) in \"%s\"", ecu->name, fpath);
        xml_free(xml);
        return -1;
    }

    if (strlen(p) >= sizeof(ecu->hw_ver))
    {
        log_e(LOG_FOTA, "version is too long, max(%d)", sizeof(ecu->hw_ver) - 1);
        return -1;
    }
    
    strcpy(ecu->hw_ver, p);
    
    if ((node = xml_get_sub_node(ecu_node, "flashver")) == NULL)
    {
        log_e(LOG_FOTA, "can't find target version of ECU(%s) in \"%s\"", ecu->name, fpath);
        xml_free(xml);
        return -1;
    }
    
    if (fota_load_ver(node, &ecu->tar) != 0)
    {
        log_e(LOG_FOTA, "load target version of ECU(%s) fail", ecu->name);
        xml_free(xml);
        return -1;
    }

    ecu->tar.ecu = ecu;

    if ((node = xml_get_sub_node(ecu_node, "flashdrv")) != NULL)
    {
        if (fota_load_ver(node, &ecu->drv) != 0)
        {
            log_e(LOG_FOTA, "load flash driver of ECU(%s) fail", ecu->name);
            xml_free(xml);
            return -1;
        }
        
        ecu->drv.ecu = ecu;
    }
    
    node_cnt = xml_get_sub_node_lst(ecu_node, "relver", node_lst, FOTA_MAX_REL_VER);

    if (node_cnt > FOTA_MAX_REL_VER)
    {
        log_e(LOG_FOTA, "too many related version(max %d) of ECU(%s) in \"%s\"", 
            FOTA_MAX_REL_VER, ecu->name, fpath);
        xml_free(xml);
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        if (fota_load_ver(node_lst[i], ecu->rel + i) != 0)
        {
            log_e(LOG_FOTA, "load related version %d of ECU(%s) fail", i + 1, ecu->name);
            xml_free(xml);
            return -1;
        }
        
        ecu->rel[i].ecu = ecu;
    }

    node_cnt = xml_get_sub_node_lst(ecu_node, "keypar", node_lst, FOTA_MAX_KEY_PAR);

    if (node_cnt > FOTA_MAX_KEY_PAR)
    {
        log_e(LOG_FOTA, "too many security parameter(max %d) of ECU(%s) in \"%s\"", 
            FOTA_MAX_KEY_PAR, ecu->name, fpath);
        xml_free(xml);
        return -1;
    }

    for (i = 0; i < node_cnt; i++)
    {
        sscanf(node_lst[i]->value, "%X", ecu->key_par + i);
    }

    ecu->key_lvl = 1;
    
    if ((p = xml_get_sub_value(ecu_node, "keylvl")) != NULL)
    {
        sscanf(p, "%d", &ecu->key_lvl);
    }

    xml_free(xml);
    return 0;
}

int fota_load_bus(bus_inf_t *bus)
{
    return fota_load_bus_file(XML_BUS_FILE, bus);
}

int fota_load(char *root, fota_t *fota)
{
    xml_t *xml;
    xml_node_t *node, *ecu_lst[FOTA_MAX_ECU];
    char fpath[256], *p = root + strlen(root);
    int ecu_cnt, i;

    while (*(p - 1) == '/')
    {
        *(--p) = 0;
    }

    memset(fota, 0, sizeof(fota_t));

    if (fota_load_bus_file(XML_BUS_FILE, fota->bus) != 0)
    {
        log_e(LOG_FOTA, "load bus information fail");
        return -1;
    }

    if (strlen(root) >= sizeof(fota->root))
    {
        log_e(LOG_FOTA, "root path \"%s\" is too long", root);
        return -1;
    }
    strcpy(fota->root, root);
    
    if (strlen(root) + strlen(XML_FOTA_FILE) >= sizeof(fpath))
    {
        log_e(LOG_FOTA, "file path \"%s\" + \"%s\" is too long(max %d)",
            root, XML_FOTA_FILE, sizeof(fpath) - 1);
        return -1;
    }
    strcpy(fpath, root);
    strcat(fpath, XML_FOTA_FILE);
    
    if ((xml = xml_load(fpath)) == NULL)
    {
        log_e(LOG_FOTA, "load FOTA information file \"%s\" fail", fpath);
        return -1;
    }

    if ((node = xml_get_node(xml, "fota")) == NULL)
    {
        log_e(LOG_FOTA, "can't find FOTA information in \"%s\"", fpath);
        xml_free(xml);
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
        strncpy(fota->desc, p, sizeof(fota->desc) - 1);
    }

    if ((p = xml_get_sub_value(node, "version")) != NULL)
    {
        strncpy(fota->ver, p, sizeof(fota->ver) - 1);
    }
#if 0
    if ((p = xml_get_sub_value(node, "functionid")) == NULL ||
        sscanf(p, "%X", &fota->fid) != 1)
    {
        log_e(LOG_FOTA, "can't find function ID");
        xml_free(xml);
        return -1;
    }
#endif
    if ((node = xml_get_sub_node(node, "strategy")) == NULL)
    {
        log_e(LOG_FOTA, "can't find FOTA strategy in \"%s\"", fpath);
        xml_free(xml);
        return -1;
    }

    ecu_cnt = xml_get_sub_node_lst(node, "ecu", ecu_lst, FOTA_MAX_ECU);

    if (ecu_cnt == 0)
    {
        log_e(LOG_FOTA, "can't find ECU description in \"%s\"", fpath);
        xml_free(xml);
        return -1;
    }

    if (ecu_cnt > FOTA_MAX_ECU)
    {
        log_e(LOG_FOTA, "too many ECU descriptions(max %d) in \"%s\"", FOTA_MAX_ECU, fpath);
        xml_free(xml);
        return -1;
    }

    for (i = 0; i < ecu_cnt; i++)
    {
        xml_node_t *node = ecu_lst[i];
        fota_ecu_t *ecu = fota->ecu + i;
        
        if ((p = xml_get_sub_value(node, "name")) == NULL)
        {
            log_e(LOG_FOTA, "can't find name of ECU %d", i + 1);
            xml_free(xml);
            return -1;
        }

        if (strlen(p) >= sizeof(ecu->name))
        {
            log_e(LOG_FOTA, "name \"%s\" is too long for ECU %d", p, i + 1);
            xml_free(xml);
            return -1;
        }

        strcpy(ecu->name, p);

        if ((p = xml_get_sub_value(node, "infoname")) == NULL)
        {
            log_e(LOG_FOTA, "can't find information file of ECU(%s)", ecu->name);
            xml_free(xml);
            return -1;
        }

        if (p[0] != '/')
        {
            log_e(LOG_FOTA, "file path \"%s\" must start with '/'", p);
            xml_free(xml);
            return -1;
        }

        if (strlen(root) + strlen(p) >= sizeof(fpath))
        {
            log_e(LOG_FOTA, "file path \"%s\" + \"%s\" is too long(max %d)",
                root, p, sizeof(fpath) - 1);
            xml_free(xml);
            return -1;
        }
        strcpy(fpath, root);
        strcat(fpath, p);

        if ((node = xml_get_sub_node(node, "sourcever")) == NULL)
        {
            log_e(LOG_FOTA, "can't find source version of ECU(%s)", ecu->name);            
            xml_free(xml);
            return -1;
        }

        if (fota_load_ver(node, &ecu->src) != 0)
        {
            log_e(LOG_FOTA, "load source version of ECU(%s) fail", ecu->name);
            xml_free(xml);
            return -1;
        }

        ecu->src.ecu = ecu;
        
        if (fota_load_ecu_file(fpath, ecu) != 0)
        {
            log_e(LOG_FOTA, "load information file for ECU(%s) fail", ecu->name);
            xml_free(xml);
            return -1;
        }
        
        ecu->fota = fota;
    }
    
    fota->security = foton_security;
    fota->check[0] = foton_check0;
    fota->check[1] = foton_check1;
    fota->check[2] = foton_check2;
    fota->erase    = foton_erase;
    fota->swver    = foton_swver;
    fota->hwver    = foton_hwver;
    fota->verstr   = foton_verstr;
    xml_free(xml);
    return 0;
}


