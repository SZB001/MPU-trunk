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
#include "minizip.h"
#include "fota_api.h"
#include "pm_api.h"
#include "fota_foton.h"
#include "aes_e.h"

//#include "../autopilot/ap_data.h"

enum
{
    FOTA_MSG_TEST = MPU_MID_FOTA,
    FOTA_MSG_TESTZIP,
};

typedef struct
{
    char fota_root[256];
    int (*fota_cb)(int, int);
} fota_msg_t;

extern unsigned long long tm_get_time(void);
extern unsigned char tbox_selfupgrade_flag;
extern unsigned char ecu_upgrade_finish_report_ecu_info_flag;
static unsigned char aes_key[16]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0f,0x0e,0x0d,0x0c,0x0b,0x0a};


static int fota_do_request(const char *fzip, fota_t *fota, int (*cb)(int, int))
{
    char path[256];
    const char *p = fzip + strlen(fzip) - 4;
    int  res = 0;
    char finv[256 + 4];

    if (strlen(fzip) < 5 || strcmp(p, ".zip") != 0)
    {
        log_e(LOG_FOTA, "fota file must be a .zip file");
        res = -1;
    }
    else if (p - fzip >= sizeof(path))
    {
        log_e(LOG_FOTA, "path name \"%.*s\" is too long", p - fzip, fzip);
        res = -1;
    }
    else
    {

        strcpy(finv, fzip);
        strcat(finv, ".inv"); 
        InvCipherFile((char *)fzip, aes_key, finv);
        
        strncpy(path, fzip, p - fzip);
        path[p - fzip] = 0;
        if (miniunz(finv, path) != 0 || fota_load(path, fota) != 0)
        {
            log_e(LOG_FOTA, "load fota file \"%s\" fail", fzip);
            res =  -1;
        }
        else
        {
            res = fota_excute(fota, cb);
        }
    }

    if (cb && (tbox_selfupgrade_flag == 0))
    {
        cb(res ? FOTA_EVENT_ERROR : FOTA_EVENT_FINISH, 0);
        ecu_upgrade_finish_report_ecu_info_flag = 1;
    }

    return res;
}

static int fota_test_report(int evt, int par)
{
    static unsigned long long time = 0;
    
    if (evt == FOTA_EVENT_PROCESS && tm_get_time() - time >= 2000)
    {
        time = tm_get_time();
        log_o(LOG_FOTA, "FOTA work process: %d%%", par);
    }
    return 0;
}

static int fota_sleep;

static int fota_sleep_handler(PM_EVT_ID id)
{
    return fota_sleep;
}

extern void hu_cmd_fota_selupgrade_finish(void);
unsigned char ecu_selfupgrade_readversion = 0;
void tbox_self_upgrade_report(void)
{
  // AP_SOCK_INFO* info;
  //  fota_msg_t msgdata;
  //  msgdata.fota_cb(FOTA_EVENT_FINISH, 0);
  hu_cmd_fota_selupgrade_finish();
  //info->waitcnt = 0;
  ecu_selfupgrade_readversion = 1;
  
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
            case PM_MSG_SLEEP:
            case PM_MSG_EMERGENCY:
            case PM_MSG_OFF:
                fota_sleep = 1;
                break;

            case PM_MSG_RUNNING:
                fota_sleep = 0;
                break;
            case FOTA_MSG_TESTZIP:
                log_i(LOG_FOTA, "get test message: %s", msgdata.fota_root);
                if (fota_sleep)
                {
                    msgdata.fota_cb(FOTA_EVENT_ERROR, 0);
                }
                else
                {
                    fota_do_request(msgdata.fota_root, &fota, msgdata.fota_cb);
                }
                break;
                
            case FOTA_MSG_TEST:
                log_i(LOG_FOTA, "get test message: %s", msgdata.fota_root);
                if (fota_load(msgdata.fota_root, &fota) == 0)
                {
                    fota_show(&fota);
                    fota_show_bus(&fota);
                    fota_excute(&fota, fota_test_report);
                }
                break;
            default:
                if (!fota_sleep)
                {
                    static uint64_t info_uptime = 0;

                    if (!info_uptime || tm_get_time() - info_uptime > 300*1000)
                    {
                        foton_update_ecu_info();
                        info_uptime = tm_get_time();                        
                    }
                }
        }
    }
    
    return NULL;
}


int fota_request(const char *fzip, int (*cb)(int, int))
{
    TCOM_MSG_HEADER msg;
    fota_msg_t fota_msg;
    
    msg.msgid    = FOTA_MSG_TESTZIP;
    msg.msglen   = sizeof(fota_msg);
    msg.sender   = MPU_MID_FOTA;
    msg.receiver = MPU_MID_FOTA;

    if (strlen(fzip) >= sizeof(fota_msg.fota_root))
    {
        log_e(LOG_FOTA, "file name \"%s\" is too long", fzip);
        return -1;
    }
    
    strcpy(fota_msg.fota_root, fzip);
    fota_msg.fota_cb = cb;
    
    if (tcom_send_msg(&msg, &fota_msg) != 0)
    {
        log_e(LOG_FOTA, "send tcom message fail");
        return -1;
    }

    return 0;    
}

static int fota_test_hex(int argc, const char **argv)
{
    static fota_img_t img;
    
    if (argc != 1)
    {
        shellprintf(" usage: hextst <path>\r\n");
        return -1;
    }

    fota_load_hex(argv[0], &img);
    fota_show_img(&img);
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

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

__attribute__((visibility("default"))) int C_print(const char *str)
{
    shellprintf(" %s\n", str);
    return strlen(str);
}

int L_print(lua_State *L, const char *str)
{
    int ret;
    
    lua_getglobal(L, "L_print");
    lua_pushstring(L, str);
    if (lua_pcall(L, 1, 1, 0))
    {
        shellprintf(" error: %s\r\n", lua_tostring(L, -1));
        return -1;
    }

    ret = lua_tonumber(L, -1);
    lua_pop(L, 1);
    return ret;
}

static int fota_lua(int argc, const char **argv)
{
    lua_State *L;
    
    if (argc < 1)
    {
        shellprintf(" usage: lua <lua file>\r\n");
        return -1;
    }
    
    if ((L = luaL_newstate()) == NULL)
    {
        shellprintf(" error: create LUA state fail\r\n");
        return -1;
    }

    luaL_openlibs(L);
    if (luaL_loadfile(L, argv[0]) || lua_pcall(L, 0, 0, 0))
    {
        shellprintf(" error: %s\r\n", lua_tostring(L, -1));
    }

    L_print(L, " print from C: Hello My LUA");
    
    lua_close(L);
    return 0;
}

static int fota_unzip(int argc, const char **argv)
{   
    if (argc < 1)
    {
        shellprintf(" usage: unzip <zip file> [extractdir]\r\n");
        return -1;
    }

    return miniunz(argv[0], argc > 1 ? argv[1] : NULL);
}
/*
extern int fota_upd_cb(int evt, int par);

static int fota_foton_unzip(int argc, const char **argv)
{   
    const char test_path[]="/home/root/foton/fotareq.zip";
    fota_request(test_path, fota_upd_cb);
    return 0;
}*/


static int fota_set_silent(int argc, const char **argv)
{   
    if (argc < 1)
    {
        shellprintf(" usage: fotasilent <0/1>\r\n");
        return -1;
    }

    fota_silent(strcmp(argv[0], "0") == 0 ? 0 : 1);
    return 0;
}
/*
static int file_test_option(int argc, const char **argv)
{
    FILE *aa,*bb;
  const char *a ="/home/root/foton/123.txt";
    long fileLen;
	char *a_buff= "wangxinwang";*/
/*    if((plain = fopen(plainFile,"wb")) == NULL)
    {
        return PLAIN_FILE_OPEN_ERROR;
    }

    //取文件长度
    fseek(cipher,0,SEEK_END);   //将文件指针置尾
    fileLen = ftell(cipher);    //取文件指针当前位置
    rewind(cipher);             //将文件指针重指向文件头
    //读取文件
   
      fread(dataBlock,sizeof(unsigned char),16,cipher);
      fwrite(outdata,sizeof(unsigned char),16 - outdata[15],plain);
      fwrite(outdata,sizeof(unsigned char),16,plain);
      
    fclose(plain);
    fclose(cipher);
 */
 /*   if((aa = fopen(a,"ab+")) == NULL)
    {
        return -1;
    }
    fwrite(a_buff,strlen(a_buff),1,aa);
    fwrite(a_buff,strlen(a_buff),1,aa);
    fclose(aa);

 
}*/
/*
 typedef struct
 {
    unsigned char *name;
    unsigned int port;
    unsigned char number;  
 }ecu_info_test;
 
 ecu_info_test foton_foton_test;

static int file_struct_study(int argc, const char **argv)
{
   char a[] = "wangxwang0927";
   ecu_info_test.name = a;
   ecu_info_test.port = 1;
   ecu_info_test.number = 0;
   
   
   printf("%s",a);



}
*/

static int fota_foton_info(int argc, const char **argv)
{
    foton_ecu_info_t ecu_i[FOTA_MAX_ECU];
    int i, cnt = foton_get_ecu_info(ecu_i, FOTA_MAX_ECU);

    if (!cnt)
    {
        shellprintf(" can't get ECU information\r\n");
    }

    for (i = 0; i < cnt; i++)
    {
        shellprintf(" ECU %d\r\n", i+1);
        shellprintf("  name       : %s\r\n", ecu_i[i].name);
        shellprintf("  HW version : %s\r\n", ecu_i[i].hwver);
        shellprintf("  SW version : %s\r\n", ecu_i[i].swver);
        shellprintf("  supplier   : %s\r\n", ecu_i[i].supplier);
        shellprintf("  part No.   : %s\r\n", ecu_i[i].partno);
    }
    
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
            ret |= pm_reg_handler(MPU_MID_FOTA, fota_sleep_handler);
            ret |= shell_cmd_register("hextst", fota_test_hex, "test Hex");
            ret |= shell_cmd_register("fotatst", fota_test, "test FOTA");
            ret |= shell_cmd_register("unzip", fota_unzip,  "unzip .zip file");
            ret |= shell_cmd_register("lua", fota_lua,  "run lua script");
            ret |= shell_cmd_register("fotasilent", fota_set_silent,  "set fota silent mode");
            ret |= shell_cmd_register("fotonecui", fota_foton_info,  "get foton ECU information");
            //ret |= shell_cmd_register("fotonunzip", fota_foton_unzip,  "test unzip");
            //ret |= shell_cmd_register("file_test", file_test_option,  "file option test");
            //ret |= shell_cmd_register("file_struct", file_struct_study,  "file option test");
           
            break;
    }

    return ret;
}

#define INIT_FUNC(f) int (* const _initfn_##f)(void) __attribute__((unused, section(".appinit"))) = f

int test(void)
{
    printf("\n----------initialize function call----------\n");
    return 0;
}
INIT_FUNC(test);

extern int (*appinit_s)(void);
extern int (*appinit_e)(void);

int fota_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    int (*testfun)(void) = appinit_s;
    
    testfun();
    appinit_s();

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

