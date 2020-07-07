#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "../support/protocol.h"
#include "com_app_def.h"
#include "mid_def.h"
#include "init.h"
#include "log.h"
#include "shell_api.h"
#include "tcom_api.h"
#include "fota.h"
#include "xml.h"


enum
{
    FOTA_MSG_TEST = MPU_MID_FOTA,
};

typedef union
{
    char fota_root[256];
} fota_msg_t;

extern int xml_load_fota(char *root, fota_t *fota);
extern void fota_show(fota_t *fota);
extern int fota_excute(fota_t *fota);
extern int fota_ecu_get_ver(unsigned char *name, char *s_ver, int *s_siz, 
                                                      char *h_ver, int *h_siz, 
                                                      char *bl_ver,   int *bl_siz,
                                                      char *sn, int *sn_siz,
                                                      char *partnum,  int *partnum_siz,
                                                      char *supplier, int *supplier_siz);

//-1:fail 0:success 1:invalid 2:upgrading
static int upgradeStatus = 1;

static unsigned int upgradeErrCode = 0;

unsigned char ecuName[10];

int get_upgrade_result(unsigned char *name, unsigned int * ErrCode)
{
    int temp = upgradeStatus;

    if (NULL != name)
    {
        memcpy(name, ecuName, sizeof(ecuName));
    }

    //When Get Upgrade Result, We Return Result And Reset Result
    if (-1 == temp)
    {
        upgradeStatus = 1;
        memset(ecuName, 0, sizeof(ecuName));

        *ErrCode = upgradeErrCode;
        upgradeErrCode = 0;
    }
    else if(0 == temp)
    {
        upgradeStatus = 1;
        memset(ecuName, 0, sizeof(ecuName));

        upgradeErrCode = 0;
    }
            

    return temp;
}


//Error Code: 0x0000xxyy
//xx: SID
//yy: Negative Response, If yy = FF, Means Wait Server Timeout
void set_upgrade_result_errcode(unsigned int ErrCode)
{
    upgradeErrCode = ErrCode;
}

static void *fota_main(void)
{
    int tcomfd;

    log_o(LOG_FOTA, "FOTA test thread running");
    prctl(PR_SET_NAME, "FOTA");

    if ((tcomfd = tcom_get_read_fd(MPU_MID_FOTA)) < 0)
    {
        log_e(LOG_FOTA, "get tcom pipe fail, thread exit");
        return NULL;
    }


    while (1)
    {
        TCOM_MSG_HEADER msg;
        fota_msg_t msgdata;
        int res;
        static fota_t fota;

        res = protocol_wait_msg(MPU_MID_FOTA, tcomfd, &msg, &msgdata, 200);

        if (res < 0)
        {
            log_e(LOG_FOTA, "unexpectedly error: %s, thread exit", strerror(errno));
            break;
        }

        switch (msg.msgid)
        {
            case FOTA_MSG_TEST:
                if (1 == upgradeStatus)
                {
                    upgradeStatus = 2;
                    log_i(LOG_FOTA, "get test message: %s", msgdata.fota_root);

                    if (xml_load_fota(msgdata.fota_root, &fota) == 0)
                    {
                        fota_show(&fota);
                        upgradeStatus = fota_excute(&fota);
                    }
                    else
                    {
                        upgradeStatus = -1;
                        log_e(LOG_FOTA, "load file fail");
                    }
                }

                break;
        }
    }

    return NULL;
}

int fota_upgrade(unsigned char *file_path)
{
    TCOM_MSG_HEADER msg;

    if (file_path == NULL)
    {
        log_e(LOG_FOTA, " file_path Is NULL ");
        return -1;
    }

    if (strlen((char *)file_path) > 255)
    {
        log_e(LOG_FOTA, " error: path \"%s\" is too long\r\n");
        return -1;
    }

    msg.msgid    = FOTA_MSG_TEST;
    msg.msglen   = strlen((char *)file_path) + 1;
    msg.sender   = MPU_MID_FOTA;
    msg.receiver = MPU_MID_FOTA;

    log_e(LOG_FOTA, " path:\"%s\",len:%d\r\n", file_path, msg.msglen);

    if (tcom_send_msg(&msg, file_path) != 0)
    {
        log_e(LOG_FOTA, " error: send tcom message fail\r\n");
        return -2;
    }

    return 0;
}

static int fota_test(int argc, const char **argv)
{
    TCOM_MSG_HEADER msg;

    if (argc != 1)
    {
        shellprintf(" usage: fotatst <fota root path>\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 255)
    {
        shellprintf(" error: path \"%s\" is too long\r\n");
        return -1;
    }

    upgradeStatus = 1;
    msg.msgid    = FOTA_MSG_TEST;
    msg.msglen   = strlen(argv[0]) + 1;
    msg.sender   = MPU_MID_FOTA;
    msg.receiver = MPU_MID_FOTA;

    if (tcom_send_msg(&msg, argv[0]) != 0)
    {
        shellprintf(" error: send tcom message fail\r\n");
        return -2;
    }

    return 0;
}

static int fota_test_ver(int argc, const char **argv)
{
    char s_ver[255] = {0};
    char h_ver[255] = {0};
    char bl_ver[255] = {0};
    char sn[255] = {0};
    char partnum[255] = {0};
    char supplier[255] = {0};
    int s_len;
    int h_len;
    int bl_len;
    int sn_len;
    int partnum_len;
    int supplier_len;

    if (argc != 1)
    {
        shellprintf(" usage: fotatstver ECUName\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 255)
    {
        shellprintf(" error: path \"%s\" is too long\r\n");
        return -1;
    }

    fota_ecu_get_ver((unsigned char *)argv[0], s_ver,    &s_len, 
                                               h_ver,    &h_len, 
                                               bl_ver,   &bl_len,
                                               sn,       &sn_len,
                                               partnum,  &partnum_len,
                                               supplier, &supplier_len);

    shellprintf("Read version:");
    shellprintf("s_len        = %d, %s_sv       : %s", s_len, argv[0], s_ver);
    shellprintf("h_len        = %d, %s_hv       : %s", h_len, argv[0], h_ver);
    shellprintf("bl_len       = %d, %s_blv      : %s", bl_len, argv[0], bl_ver);
    shellprintf("sn_len       = %d, %s_sn       : %s", sn_len, argv[0], sn);
    shellprintf("partnum_len  = %d, %s_partnum  : %s", partnum_len, argv[0], partnum);
    shellprintf("supplier_len = %d, %s_supplier : %s", supplier_len, argv[0], supplier);

    return 0;
}

static int fota_udstest(int argc, const char **argv)
{
    uint32_t i, port, txlen;
    uint8_t txbuf[3072] = {0};

    for(i = 0; i < argc; i ++)
    {
        shellprintf("argv[%d]=%s\r\n", i, argv[i]);
    }

    if(argc != 2)
    {
        shellprintf(" usage: fotaudstest <port> <len>\r\n");
        shellprintf("     example: udstest 1 100\r\n");
        shellprintf("     \"Open(port 1) -> Diag (tx 100B) -> Close\".\r\n");
        return -1;
    }

    if(sscanf(argv[0], "%d", &port) < 1)
    {
        shellprintf("arg: port error.\r\n");
        return -1;
    }

    if(sscanf(argv[1], "%d", &txlen) < 1)
    {
        shellprintf("arg: len error.\r\n");
        return -1;
    }

    if(fota_uds_open(port, 0x7DF, 0x7CA, 0x7C2) != 0)
    {
        shellprintf("fota_uds_open() error.\r\n");
        return -1;
    }

    fota_uds_request(0, 0x36, 0x0, txbuf, txlen + 1, 5);

    fota_uds_close();

    return 0;
}


int fota_init(int phase)
{
    int ret = 0;

    ret |= xml_init(phase);

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= shell_cmd_register("fotatst", fota_test, "test FOTA");
            ret |= shell_cmd_register("fotatstver", fota_test_ver, "test FOTA read version");

            ret |= shell_cmd_register("fotaudstest", fota_udstest, "test FOTA uds tx length");
            break;
    }

    return ret;
}

int fota_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)fota_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_FOTA, "create thread fail: %s", strerror(errno));
        return ret;
    }

    return 0;
}

