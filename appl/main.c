/****************************************************************
file:         main.c
description:  the source file of app loader implemention
date:         2016/12/01
author        liuzhongwen
****************************************************************/
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

#define APP_STARTUP_TIME    60
pid_t app_pid = -1;

static void *appl_startup_status_addr = 0;

/****************************************************************
function:     appl_start_app
description:  start tbox app
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int appl_start_app(void)
{
    while (1)
    {
    	if(!file_exists(COM_APP_CUR_IMAGE_DIR"/"COM_APP_IMAGE ))
    	{
    		log_e(LOG_APPL, "%s is not exist", COM_APP_CUR_IMAGE_DIR"/"COM_APP_IMAGE );
			return -1;
		}
		
        app_pid = fork();

        if (-1 == app_pid)    /**/
        {
            log_e(LOG_APPL, "fork failed, error:%s", strerror(errno));
            return 1;
        }
        else if (0 == app_pid)  // child process
        {
            char *args[] =
            {
                COM_APP_IMAGE,     /* argv[0], programme name. */
                NULL               /* list of argument must finished by NULL.  */
            };

            if (-1 == setsid())
            {
                log_e(LOG_APPL, "setsid error:%s", strerror(errno));
                return 1;
            }

            if (NULL != appl_startup_status_addr)
            {
                shm_write(appl_startup_status_addr, (unsigned char *)"UNKNOWN", strlen("UNKNOWN") + 1);
            }

            if (execvp(COM_APP_CUR_IMAGE_DIR"/"COM_APP_IMAGE, args) < 0)
            {
                log_e(LOG_APPL, "execvp failed,bin:%s,error:%s",
                      COM_APP_CUR_IMAGE_DIR"/"COM_APP_IMAGE,strerror(errno));
                return 1;
            }
            else
            {
                log_e(LOG_APPL, "execvp successful !");
                return 0;
            }
        }
        else
        {
            int  ret, time = 0;
            int  statchild;
            char start_status[16];

            log_o(LOG_APPL, "child process, pid:%u", app_pid);

            /*
            wait for tbox app start up and change the flag;
            all the pthread of app must startup in 5S.
            */
            while(1)
			{
				usleep(500000);

				time += 500;
					
				//wait for child to finished
				ret = waitpid(app_pid, &statchild, WNOHANG);
				/* the child process is running */
				if( 0 == ret )
				{
				    if( NULL != appl_startup_status_addr )
	                {
		                ret = shm_read(appl_startup_status_addr, (unsigned char *)start_status, sizeof(start_status));
		                if (ret != 0)
		                {
		                    log_e(LOG_APPL, "read startup shm failed, ret:%08x", ret);
							break;
		                }
		                else
		                {
		                    if ( 0 != strncmp(start_status, "OK", strlen("OK")) )
		                    {
		                        if( time > APP_STARTUP_TIME * 1000 )
		                        {
		                        	log_e(LOG_APPL, "tbox app status is NOK");
		                        	return 1;
		                        }
								else
								{
									continue;
								}
		                    }
		                    else
		                    {
		                        log_o(LOG_APPL, "tbox app status is OK");
								break;
		                    }
		                }
	                }
				}
				else /* the child process exit */
				{
				    log_e(LOG_APPL, "tbox app exit");
					break;
				}
   			}

            //wait for child to finished
            wait(&statchild);
            log_o(LOG_APPL, "child process exit");

            if (WIFEXITED(statchild))
            {
                if (WEXITSTATUS(statchild) == 0)
                {
                    log_e(LOG_APPL, "child process exit status:%d", WEXITSTATUS(statchild));
                    continue;
                }
                else
                {
                    log_e(LOG_APPL, "child process exit abnormally,WEXITSTATUS:%d",WEXITSTATUS(statchild));
                    continue;
                }
            }
            else
            {
                log_e(LOG_APPL, "child process exit abnormally,WIFEXITED:%d",WIFEXITED(statchild));
                continue;
            }
        }
    }
}

/****************************************************************
function:     appl_get_app_pid
description:  get tbox app pid
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
pid_t appl_get_app_pid(void)
{
    return  app_pid;
}

/****************************************************************
function:     appl_stop_app
description:  stop tbox app
input:        none
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int appl_stop_app(void)
{
    int ret;
    int statchild;

    log_o(LOG_APPL, "start to stop tbox app");

    /* terminate the app process  */
    pid_t pid = appl_get_app_pid();

    if (pid < 0)
    {
        log_o(LOG_APPL, "tbox app is stopped");
        return 0;
    }

    ret = kill(pid, SIGKILL);

    if (ret != 0 && errno != ESRCH)
    {
        log_e(LOG_APPL, "stop tbox app failed, error:%s", strerror(errno));
        return ret;
    }

    wait(&statchild); // wait for child to finished

    log_o(LOG_APPL, " tbox app is stopped");

    return 0;
}

/****************************************************************
function:     appl_copy
description:  copy images from one directory to other directory
input:        const char *src_dir, source directory;
              const char *dst_dir, destination directory;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int appl_copy(const char *src_dir, const char *dst_dir)
{
    int ret;

    if( !dir_exists(src_dir) )
    {
        return 0;
    }

    if( dir_is_empty(src_dir) )
    {
        return 0;    
    }
    
    /* create dst directory */
    ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        return ret;
    }

    /* remove the files and directory in dst_dir */
    ret = dir_remove_path(dst_dir);

    if (ret != 0)
    {
        log_e(LOG_APPL, "remove data dir failed,path:%s, ret:0x%08x",
              dst_dir, ret);
        return ret;
    }

    /* create dst data directory */
    ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

    if (0 != ret)
    {
        return ret;
    }

    /* copy the file and directory from src_dir to dst_dir */
    ret = dir_copy_path(src_dir, dst_dir);

    if (ret != 0)
    {
        log_e(LOG_APPL, "copy images failed, src path:%s,dst path:%s, ret:0x%08x",
              src_dir, src_dir, ret);
        return ret;
    }

    return 0;
}

/****************************************************************
function:     appl_overlap
description:  overlap images from one directory to other directory
input:        const char *src_dir, source directory;
              const char *dst_dir, destination directory;
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************/
int appl_overlap(const char *src_dir, const char *dst_dir)
{
    int ret;

    if( !dir_exists(src_dir) )
    {
        return 0;
    }

    if( dir_is_empty(src_dir) )
    {
        return 0;    
    }
   
    if( !dir_exists(dst_dir) )
    {
        /* create dst directory */
        ret = dir_make_path(dst_dir, S_IRUSR | S_IWUSR | S_IXUSR, false);

        if (0 != ret)
        {
            return ret;
        }
    }

    /* copy the file and directory from src_dir to dst_dir */
    ret = dir_copy_path(src_dir, dst_dir);

    if (ret != 0)
    {
        log_e(LOG_APPL, "copy images failed, src path:%s,dst path:%s, ret:0x%08x",
              src_dir, dst_dir, ret);
        return ret;
    }

    return 0;
}


int main(int argc, char **argv)
{
    int ret;
    static int startup_cnt   = 0;

    appl_startup_status_addr = shm_create(COM_APP_STARTUP_FILE, O_CREAT | O_TRUNC | O_RDWR, 16);
    if( NULL == appl_startup_status_addr )
    {
        log_e(LOG_APPL, "create shm failed");    
    }

    /* if upgrade directory is good, copy it to current directory */
    if (dir_get_status(COM_APP_UPG_DIR) && (!dir_is_empty(COM_APP_UPG_DIR)))
    {
        if( !dir_exists(COM_APP_PRE_DIR) )
        {
            /* create dst directory */
            ret = dir_make_path(COM_APP_PRE_DIR, S_IRUSR | S_IWUSR | S_IXUSR, false);

            if (0 != ret)
            {
                return ret;
            }
        }
        
        dir_update_start(COM_APP_PRE_DIR);
        
        /*backup the current image file*/
        ret = appl_copy(COM_APP_CUR_IMAGE_DIR, COM_APP_PRE_IMAGE_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "backup image files failed, ret:0x%08x", ret);
            return ret;
        }

        /*backup the current cfg file*/
        ret = appl_copy(COM_APP_CUR_CFG_DIR, COM_APP_PRE_CFG_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "backup cfg files failed, ret:0x%08x", ret);
            return ret;
        }

        /*backup the current dbc file*/
        ret = appl_copy(COM_APP_CUR_DBC_DIR, COM_APP_PRE_DBC_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "backup dbc files failed, ret:0x%08x", ret);
            return ret;
        }

        /*backup the current usrdata file*/
        ret = appl_copy(COM_APP_CUR_DATA_DIR, COM_APP_PRE_DATA_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "backup usrdata files failed, ret:0x%08x", ret);
            return ret;
        }

        dir_update_commit(COM_APP_PRE_DIR);

        dir_update_start(COM_APP_CUR_DIR);
        
        /* update the upgraded image file */
        ret = appl_overlap(COM_APP_UPG_IMAGE_DIR, COM_APP_CUR_IMAGE_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "upgrade failed, ret:0x%08x", ret);
            return ret;
        }

        dir_update_commit(COM_APP_CUR_DIR);

		/*backup the current file to another zone*/
		ret = dir_copy(COM_APP_CUR_DIR, COM_DATA_CUR_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "backup to /usrdata/current failed, ret:0x%08x", ret);
        }
    }

    /* remove upgrade file, if remove directory failed, ignore this error */
    ret = dir_remove_path(COM_APP_UPG_DIR);

    if (ret != 0)
    {
        log_e(LOG_APPL, "remove upgrade dir failed, path:%s, ret:0x%08x",
              COM_APP_UPG_DIR, ret);
    }
	
    if (dir_exists("/usrdata/cache"))
    {
        ret = dir_remove_path("/usrdata/cache");
        if (ret != 0)
        {
            log_e(LOG_APPL, "remove fota cache dir failed, path:%s, ret:0x%08x",
                  "/usrdata/cache", ret);
        }
    }

start_app:
	if(!dir_get_status(COM_DATA_CUR_DIR) || !dir_get_status(COM_DATA_CUR_CFG_DIR))
	{
		ret = dir_copy(COM_APP_CUR_DIR, COM_DATA_CUR_DIR);
		
		if (ret != 0)
		{
			log_e(LOG_APPL, "backup to /usrdata/current failed, ret:0x%08x", ret);
		}
	}
	if(!dir_get_status(COM_APP_CUR_DIR) && !dir_get_status(COM_APP_PRE_DIR))
	{
		ret = dir_copy(COM_DATA_CUR_DIR, COM_APP_CUR_DIR);
	
		if (ret != 0)
		{
			log_e(LOG_APPL, "usrdata rollback to usrapp failed, ret:0x%08x", ret);
		}
	}

    /* if current directory is good, start app manager */
    if (dir_get_status(COM_APP_CUR_DIR) && (!dir_is_empty(COM_APP_CUR_DIR)) )
    {
        startup_cnt++;
        log_o(LOG_APPL, "current image is good");
        ret = appl_start_app();
        if (0 == ret)
        {
            log_o(LOG_APPL, "start app successfully");
            goto start_app;
        }
        else
        {
            log_e(LOG_APPL, "start app failed, ret:0x%08x", ret);            
            appl_stop_app();

            if(startup_cnt <= 2)
            {
                goto start_app;
            }
            else
            {
                startup_cnt = 0;
                goto roll_back;
            }
        }
    }
    else
    {
        log_e(LOG_APPL, "the current image is not good, go to rollback" );
        goto roll_back;    
    }

roll_back:
    /* if current directory is bad, rollback */
    if (dir_get_status(COM_APP_PRE_DIR) && (!dir_is_empty(COM_APP_PRE_DIR)) )
    {
        log_o(LOG_APPL, "previous image is good, start to rollback");

        dir_update_start(COM_APP_CUR_DIR);
        
        /*rollback the current image file*/
        ret = appl_copy(COM_APP_PRE_IMAGE_DIR, COM_APP_CUR_IMAGE_DIR );

        if (ret != 0)
        {
            log_e(LOG_APPL, "rollack image file failed, ret:0x%08x", ret);
            goto exit;
        }

        /*rollback the current cfg file*/
        ret = appl_copy(COM_APP_PRE_CFG_DIR, COM_APP_CUR_CFG_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "rollack cfg file failed, ret:0x%08x", ret);
            goto exit;
        }

         /*rollback the current dbc file*/
        ret = appl_copy(COM_APP_PRE_DBC_DIR, COM_APP_CUR_DBC_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "rollback dbc file failed, ret:0x%08x", ret);
            goto exit;
        }

        /*rollback the current usrdata file*/
        ret = appl_copy(COM_APP_PRE_DATA_DIR, COM_APP_CUR_DATA_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "rollack usrdata file failed, ret:0x%08x", ret);
            goto exit;
        }

        dir_update_commit(COM_APP_CUR_DIR);

		/*rollback the current file to another zone*/
		ret = dir_copy(COM_APP_CUR_DIR, COM_DATA_CUR_DIR);

        if (ret != 0)
        {
            log_e(LOG_APPL, "rollack usrdata file failed, ret:0x%08x", ret);
            goto exit;
        }
		
        goto start_app;
    }
    else
    {
        log_e(LOG_APPL, "the backup image is not good, go to exit" ); 
        goto exit;
    }
    
exit:
    log_e(LOG_APPL, "appl exit");
    return 0;
}
