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

#define GOM_STARTUP_TIME    3
#define GMOBI_OM            "otamaster"
#define GMOBI_OM_DIR        "/sbin"

pid_t gom_pid = -1;

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
                GMOBI_OM,       /* argv[0], programme name. */
                NULL               /* list of argument must finished by NULL.  */
            };

            if (-1 == setsid())
            {
                log_e(LOG_GOML, "setsid error:%s", strerror(errno));
                return 1;
            }

            if (execvp(GMOBI_OM_DIR"/"GMOBI_OM, args) < 0)
            {
                log_e(LOG_GOML, "execvp failed,bin:%s,error:%s",
                      GMOBI_OM_DIR"/"GMOBI_OM, strerror(errno));
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
            sleep(GOM_STARTUP_TIME);

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
                log_e(LOG_GOML, "%s not start up\n", GMOBI_OM);
                return 1;
            }

            log_o(LOG_GOML, "%s is running\n", GMOBI_OM);
            
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

start_gom:
    if (file_exists(GMOBI_OM_DIR"/"GMOBI_OM))
    {
        startup_cnt++;
        ret = appl_start_gom();
        if (0 != ret)
        {
            log_e(LOG_GOML, "start %s failed, ret:0x%08x", GMOBI_OM, ret);            

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
