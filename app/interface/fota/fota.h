#ifndef __FOTA_H__
#define __FOTA_H__

#define FOTA_MAX_ECU        4
#define FOTA_MAX_REL_VER    4
#define FOTA_MAX_KEY_PAR    4
#define IMAGE_MAX_SECT  32
#define IMAGE_MAX_SIZE  1500

typedef struct
{
    uint32_t addr;
    uint32_t size;
    uint8_t *data;
} section_t;

typedef struct
{
    uint8_t buff[IMAGE_MAX_SIZE * 1024];
    section_t sect[IMAGE_MAX_SECT];
    unsigned int scnt;
} image_t;

typedef struct fota_ver fota_ver_t;
typedef struct fota_ecu fota_ecu_t;
typedef struct fota     fota_t;

struct fota_ver
{
    char ver[16];
    char fpath[128];
    int (*load)(const char *fpath, image_t *img);
    fota_ecu_t *ecu;
    int  valid;
};

struct fota_ecu
{
    char name[32];
    char skey[32];
    char oem[32];
    char hwv[16];
    int  pid, rid;
    fota_ver_t src, tar, rel[FOTA_MAX_REL_VER];
    fota_ver_t drv;
    fota_ver_t cal;
    //proc_t proc;
    int  key_par[FOTA_MAX_KEY_PAR];
    int  key_lvl;
    int  gw_sa;
    int  can_port;
    int (*security)(uint8_t *seed, int *par, uint8_t *key, int ksz);
    int (*erase)(uint32_t addr, int size);
    int (*check1)(unsigned int);
    int (*check2)(void);
    fota_t *fota;
    int valid;
};

struct fota
{
    char name[32];
    char vehi[32];
    char desp[32];
    char ver[16];
    char root[256];
    int  fid;
    int  llv;
    fota_ecu_t ecu[FOTA_MAX_ECU];
};

extern int s19_load(const char *fpath, image_t *img);

#endif
