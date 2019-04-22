#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "log.h"
#include "timer.h"
#include "fota_api.h"
#include "http.h"
#include "cfg_api.h"

extern unsigned int read_report_url_file(char *url);

#define FOTA_MAX_URL_LEN    256
#define FOTA_REQ_ROOT       "/home/root/foton/fotareq"
#define FOTA_REQ_FILE       FOTA_REQ_ROOT ".zip"
#define FOTA_ACK_URL        "http://otauat.foton.com.cn:8080/RemoteFlash/api/upgraderecord/feedback"
typedef struct
{
    char url[2][FOTA_MAX_URL_LEN];
    int  state;
    int  percent;
    int  cancel;
    int  test;
    int  silent;
    unsigned long long time;
} fota_req_t;

static pthread_mutex_t fota_mtx = PTHREAD_MUTEX_INITIALIZER;
static fota_req_t fota_req = {.state = FOTA_STAT_NOREQ, .silent = 0, .test = 0};

#define FOTA_LOCK()     pthread_mutex_lock(&fota_mtx)
#define FOTA_UNLOCK()   pthread_mutex_unlock(&fota_mtx)

extern void hu_fota_upd_warn_start(void);


static int fota_upd_cb(int evt, int par)
{
    FOTA_LOCK();
    if (fota_req.cancel || fota_req.state != FOTA_STAT_UPGRADE)
    {
        fota_req.cancel = 0;
        FOTA_UNLOCK();
        return -1;
    }

    switch (evt)
    {
        case FOTA_EVENT_PROCESS:
            fota_req.percent = par;
            break;
        case FOTA_EVENT_ERROR:
            fota_req.state = FOTA_STAT_OLDFILE;
            break;
        case FOTA_EVENT_FINISH:
            fota_req.state = FOTA_STAT_FINISH;
        default:
            break;
    }
    FOTA_UNLOCK();
    
    if (evt == FOTA_EVENT_ERROR || evt == FOTA_EVENT_FINISH)
    {
        static char post_form[512], vin[18],report_url[256];
        unsigned int sn, len;
        
        len = sizeof(sn);
        cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char*)&sn, &len);
        len = sizeof(vin);
        cfg_get_para(CFG_ITEM_FTTSP_VIN, vin, &len);
        
		//printf("vin  %x %x %x %x\r\n",vin[0],vin[1],vin[2],vin[3]);
	   // printf(" 	 %x %x %x %x\r\n",vin[4],vin[5],vin[6],vin[7]);
	   // printf(" 	 %x %x %x %x\r\n",vin[8],vin[9],vin[10],vin[11]);
	   // printf(" 	 %x %x %x %x\r\n",vin[12],vin[13],vin[14],vin[15]);
        
        len = sizeof(report_url);

       if (read_report_url_file(report_url)!= 0)
       {
           log_e(LOG_FOTA, "read report url file error!");
       }

        sprintf(post_form, "vinCode=%s&sn=%u&reqPath=%s&status=", vin, sn, report_url);
        strcat(post_form, evt == FOTA_EVENT_ERROR ? "1" : "0");
        http_post_msg(FOTA_ACK_URL, post_form);
    }
    
    return 0;
}
void fota_selfupgrade_finish_status(void)
{
    fota_req.state = FOTA_STAT_UPGRADE;
	fota_upd_cb(FOTA_EVENT_FINISH,0);
}

static int fota_dld_cb(int evt, int par)
{
    FOTA_LOCK();
    if (fota_req.state != FOTA_STAT_DOWNLOAD)
    {
        FOTA_UNLOCK();
        return -1;
    }
    
    if (fota_req.cancel)
    {
        fota_req.cancel = 0;
        fota_req.state = FOTA_STAT_NOREQ;
        FOTA_UNLOCK();
        return -1;
    }
    
    switch (evt)
    {
        case HTTP_EVENT_PROCESS:
            fota_req.percent = par;
          //  printf("wangxinwang fota_req.percent = %d\r\n",fota_req.percent);
            break;
        case HTTP_EVENT_ERROR:
            fota_req.state = FOTA_STAT_PENDING;            
            break;
        case HTTP_EVENT_FINISH:
       // printf("wangxinwang fota_req.silent = %d  fota_req.test =%d\r\n",fota_req.silent,fota_req.test);
            if (fota_req.silent && !fota_req.test)
            {
                fota_req.percent = 0;
                fota_req.state = fota_request(FOTA_REQ_FILE, fota_upd_cb) ? 
                                 FOTA_STAT_OLDFILE : FOTA_STAT_UPGRADE;
            }
            else
            {
                fota_req.state = FOTA_STAT_NEWFILE;
            }
        default:
            break;
    }
    FOTA_UNLOCK();
    return 0;
}

void fota_silent(int en)
{
    FOTA_LOCK();
    fota_req.silent = en;
    FOTA_UNLOCK();
}

int fota_state(void)
{
    int ret;

    FOTA_LOCK();
    ret = fota_req.state;
    FOTA_UNLOCK();
    return ret;
}

int fota_new_request(const char *url, uint8_t *md5, int test)
{   
    FOTA_LOCK();

    if (fota_req.state == FOTA_STAT_NEWREQ || fota_req.state == FOTA_STAT_NEWFILE)
    {
        fota_req.state = tm_get_time() - fota_req.time > 10000 ? FOTA_STAT_NOREQ : fota_req.state;
    }        

    if (fota_req.state != FOTA_STAT_NOREQ &&
        fota_req.state != FOTA_STAT_PENDING &&
        fota_req.state != FOTA_STAT_OLDFILE &&         
        fota_req.state != FOTA_STAT_FINISH)
    {
        log_e(LOG_FOTA, "fota is busy in another request");
        FOTA_UNLOCK();
        return -1;
    }

    if ((fota_req.test = test) == 0)
    {
        if (strlen(url) >= sizeof(fota_req.url[0]))
        {
            log_e(LOG_FOTA, "URL(\"%s\") if too long for request");
            FOTA_UNLOCK();
            return -1;
        }

        strcpy(fota_req.url[0], url);

        if (fota_req.silent)
        {
            if (http_download_request(fota_req.url[0], FOTA_REQ_FILE, md5, fota_dld_cb) != 0)
            {
                log_e(LOG_FOTA, "request download from \"%s\" fail", fota_req.url[0]);
                fota_req.state = FOTA_STAT_PENDING;
                FOTA_UNLOCK();
                return -1;
            }

            fota_req.state = FOTA_STAT_DOWNLOAD;
            fota_req.percent = 0;
            FOTA_UNLOCK();
            return 0;
        }
    }

    fota_req.state = FOTA_STAT_NEWREQ;
    fota_req.time  = tm_get_time();    
    FOTA_UNLOCK();

    //hu_fota_upd_warn_start();
    return 0;
}

int fota_dld_start(void)
{    
    FOTA_LOCK();
    if (fota_req.state != FOTA_STAT_NEWREQ && fota_req.state != FOTA_STAT_PENDING)
    {
        FOTA_UNLOCK();
        return -1;
    }
    
    if (!fota_req.test && http_download_request(fota_req.url[0], FOTA_REQ_FILE, NULL, fota_dld_cb) != 0)
    {
        log_e(LOG_FOTA, "request download from \"%s\" fail", fota_req.url[0]);
        fota_req.state = FOTA_STAT_PENDING;
        FOTA_UNLOCK();
        return -1;
    }

    fota_req.state = FOTA_STAT_DOWNLOAD;
    fota_req.percent = 0;
    FOTA_UNLOCK();
    return 0;
}

void fota_dld_cancel(void)
{
    FOTA_LOCK();
    if (fota_req.state == FOTA_STAT_NEWREQ)
    {
        fota_req.state = FOTA_STAT_PENDING;
    }
    else if (fota_req.state == FOTA_STAT_DOWNLOAD)
    {
        fota_req.cancel = !fota_req.test;
    }
    FOTA_UNLOCK();
}

int fota_dld_percent(void)
{
    int percent;

    FOTA_LOCK();
    switch (fota_req.state)
    {
        case FOTA_STAT_DOWNLOAD:
            switch (fota_req.test)
            {
                case 1: /* download & upgrade success */
                case 2: /* download success, upgrade failed */
                    if ((fota_req.percent += 10) >= 100)
                    {
                        fota_req.state = FOTA_STAT_NEWFILE;
                        fota_req.time  = tm_get_time();
                    }
                    break;
                case 3: /* download failed */
                    if ((fota_req.percent += 10) >= 50)
                    {
                        fota_req.state = FOTA_STAT_PENDING;
                    }
                default:
                    break;
            }                
            percent = fota_req.percent;
            break;
        case FOTA_STAT_NEWFILE:
        case FOTA_STAT_UPGRADE:
        case FOTA_STAT_FINISH:
            percent = 101;
            break;
        default:
            percent = -1;
            break;
    }
    FOTA_UNLOCK();
    return percent;
}


int fota_upd_start(void)
{
    FOTA_LOCK();
    if (fota_req.state != FOTA_STAT_NEWFILE && fota_req.state != FOTA_STAT_OLDFILE)
    {
        FOTA_UNLOCK();
        return -1;
    }

    if (!fota_req.test && fota_request(FOTA_REQ_FILE, fota_upd_cb) != 0)
    {
        fota_req.state = FOTA_STAT_OLDFILE;
        FOTA_UNLOCK();
        return -1;
    }

    fota_req.state = FOTA_STAT_UPGRADE;
    fota_req.percent = 0;
    FOTA_UNLOCK();
    return 0;
}

void fota_upd_cancel(void)
{
    FOTA_LOCK();
    if (fota_req.state == FOTA_STAT_NEWFILE)
    {
        fota_req.state = FOTA_STAT_OLDFILE;
    }
    else if (fota_req.state == FOTA_STAT_UPGRADE)
    {
        fota_req.cancel = !fota_req.test;
    }
    FOTA_UNLOCK();
}

int fota_upd_percent(void)
{
    int percent;

    FOTA_LOCK();
    switch (fota_req.state)
    {
        case FOTA_STAT_UPGRADE:
            switch (fota_req.test)
            {
                case 1: /* download & upgrade success */
                    if ((fota_req.percent += 10) >= 100)
                    {
                        fota_req.state = FOTA_STAT_FINISH;
                    }
                    break;                
                case 2: /* download success, upgrade failed */
                    if ((fota_req.percent += 10) >= 50)
                    {
                        fota_req.state = FOTA_STAT_OLDFILE;
                    }
                default:
                    break;
            }                
            percent = fota_req.percent;
            break;
        case FOTA_STAT_FINISH:
            percent = 101;
            break;
        default:
            percent = -1;
            break;
    }
    FOTA_UNLOCK();
    return percent;
}

void fota_testmode(int mode)
{
    FOTA_LOCK();
    fota_req.test = mode;
    FOTA_UNLOCK();
}
