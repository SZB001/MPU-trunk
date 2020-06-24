/*********************************************************************
file:         main.c
description:  the source file of gmobi otamaster loader implemention
date:         2018/09/29
author        chenyin
*********************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include "com_app_def.h"
#include "dir.h"
#include "file.h"
#include "shm.h"
#include "wsrv_def.h"

#define GOML_POLLING_INTERVAL 1   /*Unit: s*/
#define GOML_DA_STARTUP_TIME    3   /*Unit: s*/
#define GOML_DA_NAME_MAX        32

#define GOML_SUB_DA_DIR     "/sbin"
#define GOML_SUB_DA         "otamaster"     /*长度 < GOML_DA_NAME_MAX*/
#define GOML_MASTER_DA_DIR  "/usrapp/current/data/image"
#define GOML_MASTER_DA      "otamasterv4"     /*长度 < GOML_DA_NAME_MAX*/

static pid_t gom_pid = -1;

static void *pGomlShmAddr = NULL;
static char GomlDaName[GOML_DA_NAME_MAX];
static const char *pGomlDaFilePath = NULL;

/****************************************************************
function:     appl_start_gom
description:  start gmobi otamaster
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int appl_start_gom(void)
{
    while (1)
    {
        gom_pid = fork();

        if (-1 == gom_pid)    /**/
        {
            log_e(LOG_GOML, "fork failed, error:%s", strerror(errno));
            return 1;
        }
        else if (0 == gom_pid)  // child process
        {
            char *args[] =
            {
                GomlDaName,       /* argv[0], program name. */
                NULL               /* list of argument must finished by NULL.  */
            };

            if (-1 == setsid())
            {
                log_e(LOG_GOML, "setsid error:%s", strerror(errno));
                return 1;
            }

            if (execvp(pGomlDaFilePath, args) < 0)
            {
                log_e(LOG_GOML, "execvp failed, bin:%s, error:%s",
                      pGomlDaFilePath, strerror(errno));
                return 1;
            }
        }
        else
        {
            int ret;
            int statchild;
            FILE *ptream;
            char pid_buf[16];

            log_o(LOG_GOML, "child process, pid:%u", gom_pid);
            
            /*
            wait for gmobi otamaster start up.
            */
            sleep(GOML_DA_STARTUP_TIME);

            /*
            check gmobi otamaster start up state.
            */
            ptream = popen("ps | grep otamaster | grep -v grep | awk '{print $1}'", "r");
            
            if (!ptream)
            {
                log_e(LOG_GOML, "open dev failed , error:%s", strerror(errno));
                return 1;
            }

            ret = fread(pid_buf, sizeof(char), sizeof(pid_buf), ptream);
            pclose(ptream);

            if (ret <= 0)
            {
                log_e(LOG_GOML, "%s not start up\n", pGomlDaFilePath);
                return 1;
            }

            log_o(LOG_GOML, "%s is running\n", pGomlDaFilePath);
            
            //wait for child to finished
            wait(&statchild);
            log_o(LOG_GOML, "child process exit");

            if (WIFEXITED(statchild))
            {
                if (WEXITSTATUS(statchild) == 0)
                {
                    log_e(LOG_GOML, "child process exit status:%d", WEXITSTATUS(statchild));
                    continue;
                }
                else
                {
                    log_e(LOG_GOML, "child process exit abnormally,WEXITSTATUS:%d",WEXITSTATUS(statchild));
                    continue;
                }
            }
            else
            {
                log_e(LOG_GOML, "child process exit abnormally,WIFEXITED:%d",WIFEXITED(statchild));
                continue;
            }
        }
    }
}

int main(int argc, char **argv)
{
    int ret;
    static int startup_cnt   = 0;
    uint8_t shm_rdbuf[WSRV_GOML_SHM_SIZE];

    pGomlShmAddr = shm_create(WSRV_GOML_SHM_NAME, O_CREAT | O_TRUNC | O_RDWR,
        WSRV_GOML_SHM_SIZE);
    if( NULL == pGomlShmAddr )
    {
        log_e(LOG_GOML, "create shm failed\r\n");
    }

    memset(GomlDaName, '\0', sizeof(GomlDaName));

    while(1)
    {
        ret = shm_read(pGomlShmAddr, shm_rdbuf, WSRV_GOML_SHM_SIZE);
        if(ret == 0)
        {
            if(strncmp((const char *)shm_rdbuf, WSRV_GOML_OTACTRL_IHU,
                strlen(WSRV_GOML_OTACTRL_IHU)) == 0)
            {
                strcpy(GomlDaName, GOML_SUB_DA);
                pGomlDaFilePath = GOML_SUB_DA_DIR"/"GOML_SUB_DA;
                break;
            }

            if(strncmp((const char *)shm_rdbuf, WSRV_GOML_OTACTRL_TBOX,
                strlen(WSRV_GOML_OTACTRL_TBOX)) == 0)
            {
                strcpy(GomlDaName, GOML_MASTER_DA);
                pGomlDaFilePath = GOML_MASTER_DA_DIR"/"GOML_MASTER_DA;
                break;
            }
        }
        else
        {
            log_e(LOG_GOML, "read shm failed\r\n");
        }
        sleep(GOML_POLLING_INTERVAL);
    }

start_gom:
    log_o(LOG_GOML, "OTACtrl=%s", shm_rdbuf);
    log_o(LOG_GOML, "startup %s", pGomlDaFilePath);
    if (file_exists(pGomlDaFilePath))
    {
        startup_cnt++;
        ret = appl_start_gom();
        if (0 != ret)
        {
             log_e(LOG_GOML, "start %s failed, ret:0x%08x", pGomlDaFilePath, ret);

            if(startup_cnt <= 2)
            {
                goto start_gom;
            }
            else
            {
                startup_cnt = 0;
                goto exit;
            }
        }
    }
    else
    {
        goto exit;    
    }
    
exit:
    log_e(LOG_GOML, "goml exit");
    return 0;
}
