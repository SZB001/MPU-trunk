#include <stdio.h>
#include "http_client.h"
#include "http.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "dev_api.h"
#include "at.h"
#include "at_api.h"
#include "shell_api.h"
#include "fota_api.h"
#include "file.h"

typedef struct
{
    char url[256];
    char fpath[256];
    int (*callback)(int, int);
    uint8_t md5[16];
    int  md5_ok;
    int  busy;
} http_req_t;

static int recv_cb(int status, int received, int total, void *usr)
{
    http_req_t *req = usr;
    static unsigned long long time = 0;

    if (tm_get_time() - time > 2000 || received >= total)
    {
        time = tm_get_time();
        log_o(LOG_FOTA, "downloading %%%d(%d/%d), response code %d", 
            received * 100 / total, received, total, status);
    }    

    if((received * 100 / total) >= 100)
    {
      return req->callback(HTTP_EVENT_FINISH , 100);
    }
    if (req && req->callback)
    {
        return req->callback(HTTP_EVENT_PROCESS , received * 100 / total);
    }
    return 0;
}


static void* http_download(http_req_t *req)
{
    ft_http_client_t *http;
    int res = HTTP_EVENT_ERROR;
    
    if ((http = ft_http_new()) == NULL)
    {
        log_e(LOG_FOTA, "no memory for http");
        res = HTTP_EVENT_FINISH;
    }
    else
    {
        ft_http_set_data_recv_cb(http, recv_cb, req);
        if (ft_http_sync_download_file(http, req->url, req->fpath) != 0)
        {        
            log_e(LOG_FOTA, "download file fail");
            res = HTTP_EVENT_ERROR;
        }
        else if (req->md5_ok)
        {
            uint8_t md5[16];
            
            if (file_md5(req->fpath, md5) != 0)
            {
                log_e(LOG_FOTA, "calculate MD5 of file fail");
                res = HTTP_EVENT_ERROR;
            }
            else if (memcmp(md5, req->md5, 16) != 0)
            {
                log_e(LOG_FOTA, "MD5 of file is not matched");
                res = HTTP_EVENT_ERROR;
            }
        }

        ft_http_destroy(http);
        ft_http_deinit();
    }

    if (req->callback)
    {
        req->callback(res, 0);
    }
    req->busy = 0;
    return NULL;
}

int http_post_msg(const char *url, const char *msg)
{
    ft_http_client_t *http;
    int res = 0;
    const char *p = NULL;
    log_i(LOG_FOTA, "try post \"%s\" to \"%s\"", msg, url);
    
    if ((http = ft_http_new()) == NULL)
    {
        log_e(LOG_FOTA, "no memory for http");
        res = -1;
    }
    else
    {   
        p = ft_http_sync_post(http, url, msg, strlen(msg));
        if ((res = ft_http_get_error_code(http)) != 0)
        {
            log_e(LOG_FOTA, "post fail(%d)", res);
        }
        else
        {
            log_i(LOG_FOTA, "response code: %d", ft_http_get_status_code(http));
            log_i(LOG_FOTA, "response data: \"%s\"", p);
        }
        ft_http_destroy(http);
    }
    
    return res;
}


int http_download_request(const char *url, const char *fpath, uint8_t *md5, int (*cb)(int, int))
{
    pthread_t tid;
    pthread_attr_t ta;
    static http_req_t req = {.busy = 0};
    static char ota_url_report_file[]="/home/root/foton/url_report.txt";
    int ret = 0;	
    
	ret = file_write_atomic(ota_url_report_file,(unsigned char *)url,
				strlen(url), S_IRUSR | S_IWUSR | S_IXUSR);
		
	if (ret != 0)
	{
		log_e(LOG_FOTA, "write tsp url report failed, file:%s, ret:0x%08x",
	    ota_url_report_file, ret);
	}
    if (req.busy)
    {
        log_e(LOG_FOTA, "HTTP service is busy");
        return -1;
    }
    
    log_o(LOG_FOTA, "HTTP download, url=%s, local file=%s", url, fpath);

    if (strlen(url) >= sizeof(req.url) || strlen(fpath) >= sizeof(req.fpath))
    {
        log_e(LOG_FOTA, "URL or file path is too long");
        return -1;
    }

    memcpy(req.md5, md5 ? md5 : req.md5, md5 ? 16 : 0);
    req.md5_ok = md5 != NULL;
    
    strcpy(req.url, url);
    strcpy(req.fpath, fpath);
    req.callback = cb;
    
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&tid, &ta,(void *)http_download, &req) < 0)
    {
        log_e(LOG_FTP, "creating thread fail");
        return -1;
    }

    req.busy = 1;
    return 0;
}
