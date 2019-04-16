#ifndef __FOTA_API_H
#define __FOTA_API_H
#include <stdint.h>
enum
{
    FOTA_EVENT_FINISH,
    FOTA_EVENT_PROCESS,
    FOTA_EVENT_ERROR,
};

enum
{
    FOTA_STAT_NOREQ,
    FOTA_STAT_NEWREQ,
    FOTA_STAT_PENDING,
    FOTA_STAT_DOWNLOAD,
    FOTA_STAT_NEWFILE,
    FOTA_STAT_OLDFILE,
    FOTA_STAT_UPGRADE,
    FOTA_STAT_FINISH,
};


typedef struct
{
    char name[16];
    char supplier[16];
    char partno[16];
    char swver[16];
    char hwver[16];
} foton_ecu_info_t;


/* from fota_ctrl.c */
extern void fota_silent(int en);
extern int fota_state(void);
extern int fota_new_request(const char *url, uint8_t *md5, int test);
extern int fota_dld_start(void);
extern void fota_dld_cancel(void);
extern int fota_dld_percent(void);
extern int fota_upd_start(void);
extern void fota_upd_cancel(void);
extern int fota_upd_percent(void);
extern void fota_testmode(int mode);

extern int foton_get_ecu_info(foton_ecu_info_t *inf, int max);

/* from fota_main.c */
extern int fota_init(int phase);
extern int fota_run(void);
extern int fota_request(const char *fzip, int (*cb)(int, int));

#endif