
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "log.h"
#include "fota.h"
#include "fota_foton.h"
#include "fota_api.h"

#define FOR_EACH_FOTABUS(bus, fota) \
    for (bus = (fota)->bus; bus->port && bus < (fota)->bus + FOTA_MAX_BUS; bus++)
#define FOR_EACH_BUSECU(ecu, bus) \
    for (ecu = (bus)->ecu; ecu->name[0] && ecu < (bus)->ecu + FOTA_MAX_ECU; ecu++)

int foton_erase(uint32_t base, int size)
{
    uint8_t tmp[16];

    tmp[0]  = 0xff;
    tmp[1]  = 0;
    tmp[2]  = 0x44;
    tmp[3]  = base >> 24;
    tmp[4]  = base >> 16;
    tmp[5]  = base >> 8;
    tmp[6]  = base;
    tmp[7]  = size >> 24;
    tmp[8]  = size >> 16;
    tmp[9]  = size >> 8;
    tmp[10] = size;

    if (fota_uds_request(0, 0x31, 0x01, tmp, 11, 60) != 0 ||
        fota_uds_result(tmp, sizeof(tmp)) != 3 || tmp[0] != 0xff || tmp[1] != 0 || tmp[2] != 0)
    {
        log_e(LOG_FOTA, "erase flash(base=%08X, size=%d) fail", base, size);
        return -1;
    }

    return 0;
}

int foton_check0(void)
{
    uint8_t tmp[16];

    return (fota_uds_request(0, 0x31, 0x01, (uint8_t*)"\xf0\x02", 2, 3) != 0 ||
        fota_uds_result(tmp, sizeof(tmp)) != 3 || 
        tmp[0] != 0xf0 || tmp[1] != 0x02 || tmp[2] != 0);
}


int foton_check1(void)
{
    uint8_t tmp[16];

    return (fota_uds_request(0, 0x31, 0x01, (uint8_t*)"\xf0\x01", 2, 3) != 0 ||
        fota_uds_result(tmp, sizeof(tmp)) != 3 || 
        tmp[0] != 0xf0 || tmp[1] != 0x01 || tmp[2] != 0);
}

int foton_check2(void)
{
    uint8_t tmp[16];

    return (fota_uds_request(0, 0x31, 0x01, (uint8_t*)"\xff\x01", 2, 3) != 0 ||
        fota_uds_result(tmp, sizeof(tmp)) != 3 || 
        tmp[0] != 0xff || tmp[1] != 0x01 || tmp[2] != 0);
}


int foton_security(uint8_t *seed, int *par, uint8_t *key, int keysz)
{
    if (keysz < 4)
    {
        return -1;
    }
    
    key[0] = par[0] * seed[0] * seed[0] + 
             par[1] * seed[1] * seed[1] + 
             par[2] * seed[0] * seed[1];
    
    key[1] = par[0] * seed[0] + 
             par[1] * seed[1] + 
             par[3] * seed[0] * seed[1];
    
    key[2] = par[0] * seed[2] * seed[3] +
             par[1] * seed[3] * seed[3] +
             par[2] * seed[2] * seed[3];
    
    key[3] = par[0] * seed[2] * seed[3] + 
             par[1] * seed[3] + 
             par[3] * seed[2] * seed[3];

    log_o(LOG_FOTA, "par  :%02X, %02X, %02X, %02X", par[0], par[1], par[2], par[3]);
    log_o(LOG_FOTA, "seed :%02X, %02X, %02X, %02X", seed[0], seed[1], seed[2], seed[3]);
    log_o(LOG_FOTA, "key  :%02X, %02X, %02X, %02X", key[0], key[1], key[2], key[3]);
    return 4;
}

int foton_swver(uint8_t *buff, int size)
{
    int len;
    
    if (fota_uds_request(0, 0x22, 0, (uint8_t*)"\xf1\x95", 2, 5) != 0 ||
        (len = fota_uds_result(buff, size)) < 2 ||
        buff[0] != 0xf1 || buff[1] != 0x95)
    {
        return -1;
    }

    memcpy(buff, buff + 2, len - 2);
    buff[len - 2] = 0;
    return len - 2;;
}

int foton_hwver(uint8_t *buff, int size)
{
    int len;
    
    if (fota_uds_request(0, 0x22, 0, (uint8_t*)"\xf1\x93", 2, 5) != 0 ||
        (len = fota_uds_result(buff, size)) < 2 ||
        buff[0] != 0xf1 || buff[1] != 0x93)
    {
        return -1;
    }

    memcpy(buff, buff + 2, len - 2);
    buff[len - 2] = 0;
    return len - 2;;
}


int foton_partno(uint8_t *buff, int size)
{
    int len;
    
    if (fota_uds_request(0, 0x22, 0, (uint8_t*)"\xf1\x87", 2, 5) != 0 ||
        (len = fota_uds_result(buff, size)) < 2 ||
        buff[0] != 0xf1 || buff[1] != 0x87)
    {
        return -1;
    }

    memcpy(buff, buff + 2, len - 2);
    buff[len - 2] = 0;
    return len - 2;;
}

int foton_supplier(uint8_t *buff, int size)
{
    int len;
    
    if (fota_uds_request(0, 0x22, 0, (uint8_t*)"\xf1\x8a", 2, 5) != 0 ||
        (len = fota_uds_result(buff, size)) < 2 ||
        buff[0] != 0xf1 || buff[1] != 0x8a)
    {
        return -1;
    }

    memcpy(buff, buff + 2, len - 2);
    buff[len - 2] = 0;
    return len - 2;;
}

const char* foton_verstr(uint8_t *ver, int size)
{
    return (char*)ver;
}

#if 1


static pthread_mutex_t foton_ecu_m = PTHREAD_MUTEX_INITIALIZER;
static foton_ecu_info_t foton_ecu_i[FOTA_MAX_ECU];
static int foton_ecu_c;


int foton_get_ecu_info(foton_ecu_info_t *inf, int max)
{
    int cnt = 0;
    
    pthread_mutex_lock(&foton_ecu_m);
    cnt = max > foton_ecu_c ? foton_ecu_c : max;
    memcpy(inf, foton_ecu_i, cnt * sizeof(foton_ecu_info_t));
    pthread_mutex_unlock(&foton_ecu_m);
    return cnt;
}

void foton_update_ecu_info(void)
{
    bus_inf_t bus_inf[FOTA_MAX_BUS], *bus;
    foton_ecu_info_t ecu_inf[FOTA_MAX_ECU];
    int ecu_cnt;
    
    if (fota_load_bus(bus_inf) != 0)
    {
        log_e(LOG_FOTA, "load BUS information fail");
        return;
    }
    // printf("wangwangwang\r\n");
    for (bus = bus_inf, ecu_cnt = 0;
         ecu_cnt < FOTA_MAX_ECU && bus->port && bus < bus_inf + FOTA_MAX_BUS; 
         bus++)
    {
        ecu_inf_t *ecu;
        int baud = can_get_baud(bus->port - 1);

        if (baud != bus->baud && can_set_baud(bus->port - 1, bus->baud) != 0)
        {
            log_e(LOG_FOTA, "can't set baud rate of bus %d", bus->port);
            continue;
        }

        sleep(2);
        
        FOR_EACH_BUSECU(ecu, bus)
        {
            foton_ecu_info_t *ecu_i = ecu_inf + ecu_cnt;
                
            if (ecu_cnt >= FOTA_MAX_ECU)
            {
                break;
            }
            
            if (fota_uds_open(bus->port - 1, bus->fid, ecu->rid, ecu->pid) != 0)
            {
                log_e(LOG_FOTA, "open UDS for ECU(%s) fail", ecu->name);
                fota_uds_close();
                continue;
            }

            if (foton_swver((uint8_t*)ecu_i->swver, sizeof(ecu_i->swver)) <= 0 ||
                foton_hwver((uint8_t*)ecu_i->hwver, sizeof(ecu_i->hwver)) <= 0 ||
                foton_partno((uint8_t*)ecu_i->partno, sizeof(ecu_i->partno)) <= 0 ||
                foton_supplier((uint8_t*)ecu_i->supplier, sizeof(ecu_i->supplier)) <= 0)
            {
                fota_uds_close();
                continue;
            }

            fota_uds_close();
            strncpy(ecu_i->name, ecu->name, sizeof(ecu_i->name));
            ecu_i->name[sizeof(ecu_i->name) - 1] = 0;
            ecu_cnt++;            
        }

        can_set_baud(bus->port - 1, baud);
        sleep(2);
    }

    if (ecu_cnt > 0)
    {
        pthread_mutex_lock(&foton_ecu_m);
        foton_ecu_c = ecu_cnt;
        memcpy(foton_ecu_i, ecu_inf, ecu_cnt * sizeof(foton_ecu_info_t));
        pthread_mutex_unlock(&foton_ecu_m);
    }
}

#endif
