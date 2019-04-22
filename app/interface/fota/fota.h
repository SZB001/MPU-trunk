#ifndef __FOTA_H__
#define __FOTA_H__
#include "stdint.h"
#include "can_api.h"

#define FOTA_MAX_BUS        5
#define FOTA_MAX_ECU        16
#define FOTA_MAX_REL_VER    4
#define FOTA_MAX_KEY_PAR    4
#define FOTA_MAX_ECUNAME    16


/* Max. sections in image */
#define IMAGE_MAX_SECT  128
/* Max. image size, in KB */
#define IMAGE_MAX_SIZE  1024

#define FOR_EACH_IMGSECT(sect, img, cond) \
    for (sect = (img)->sect; (cond) && sect->size && sect < (img)->sect + IMAGE_MAX_SECT; sect++)

typedef struct
{
    uint32_t base;
    uint32_t size;
    uint8_t *data;
} img_sect_t;

typedef struct
{
    img_sect_t sect[IMAGE_MAX_SECT];    
    uint8_t buff[IMAGE_MAX_SIZE * 1024];
} fota_img_t;

typedef struct
{
    char name[FOTA_MAX_ECUNAME];
    int  pid;
    int  rid;
} ecu_inf_t;

typedef struct
{
    int port;
    int baud;
    int fid;    
    ecu_inf_t ecu[FOTA_MAX_ECU];    
} bus_inf_t;

typedef struct _fota_ver fota_ver_t;
typedef struct _fota_ecu fota_ecu_t;
typedef struct _fota     fota_t;

struct _fota_ver
{
    char ver[32];
    char fpath[128];
    int  (*img_load)(const char *fpath, fota_img_t *img, ...);
    int  (*img_calc)(const char *fpath);
    uint32_t img_attr;
    fota_ecu_t *ecu;
};

struct _fota_ecu
{
    char name[FOTA_MAX_ECUNAME];
    char oem_name[32];
    char hw_ver[32];
    fota_ver_t src;
    fota_ver_t tar;
    fota_ver_t rel[FOTA_MAX_REL_VER];
    fota_ver_t drv;
    int  key_par[FOTA_MAX_KEY_PAR];
    int  key_lvl;    
    fota_t *fota;
    int  rback;
};

struct _fota
{
    char name[32];
    char vehi[32];
    char desc[32];
    char ver[32];
    char root[256];
    int  (*callback)(int evt, int par);
    int  (*security)(uint8_t *seed, int *par, uint8_t *key, int ksz);
    int  (*erase)(uint32_t base, int size);
    int  (*check[3])(void);    
    int  (*hwver)(uint8_t *buff, int size);
    int  (*swver)(uint8_t *buff, int size);
    const char* (*verstr)(uint8_t *buff, int size);
    int  curr, total;
    bus_inf_t  bus[FOTA_MAX_BUS];
    fota_ecu_t ecu[FOTA_MAX_ECU];
};



extern int fota_load(char *root, fota_t *fota);
extern void fota_show(fota_t *fota);
extern void fota_show_bus(fota_t *fota);
extern int fota_excute(fota_t *fota, int (*cb)(int, int));

extern int fota_uds_open(int port, int fid, int rid, int pid);
extern void fota_uds_close(void);
extern int fota_uds_request(int bc, int sid, int sub, uint8_t *data, int len, int timeout);
extern int fota_uds_result(uint8_t *buf, int siz);
extern int fota_uds_req_download(uint32_t base, int size);
extern int fota_uds_trans_data(uint8_t *data, int size);
extern int fota_uds_trans_exit(void);

extern int fota_calc_bin(const char *fpath);
extern int fota_calc_s19(const char *fpath);
extern int fota_calc_hex(const char *fpath);
extern int fota_load_bin(const char *fpath, fota_img_t *img, uint32_t base);
extern int fota_load_s19(const char *fpath, fota_img_t *img, int wsize);
extern int fota_load_hex(const char *fpath, fota_img_t *img);
extern void fota_show_img(fota_img_t *img);
extern int fota_load_bus(bus_inf_t *bus);
#endif
