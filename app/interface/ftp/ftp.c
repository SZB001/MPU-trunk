#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>

#include "ftplib.h"

#include "com_app_def.h"
#include "dir.h"
#include "file.h"
#include "ftp_api.h"
#include "init.h"
#include "timer.h"
#include "shell_api.h"

#define FTP_MAX_REQ     4
#define STR_OK_FAIL(a)  ((a) ? "success" : "fail")

typedef struct
{
    char user[32];
    char pswd[32];
    char host[64];
    char path[128];
    char buff[1024];
    uint16_t  port;
    void (*callback)(int evt, int par);
    char fpath[256];
    int  used;
} ftpreq_t;

static ftpreq_t        ftp_reqs[FTP_MAX_REQ];
static pthread_mutex_t ftp_mutex;

#define ftp_lock()      pthread_mutex_lock(&ftp_mutex)
#define ftp_unlock()    pthread_mutex_unlock(&ftp_mutex)

static int ftp_paseurl(const char *url, char *usr, char *psw, char *host, uint16_t *port,
                       char *path)
{
    strcpy(usr, "anonymous");
    strcpy(psw, "");
    *path = '/';
    *port = 21;

    if (sscanf(url, "ftp://%63[^:@/]/%127s", host, path + 1) == 2 ||
        sscanf(url, "ftp://%63[^:@/]:%hu/%127s", host, port, path + 1) == 3 ||
        sscanf(url, "ftp://%31[^:@/]@%63[^:@/]/%127s", usr, host, path + 1) == 3 ||
        sscanf(url, "ftp://%31[^:@/]@%63[^:@/]:%hu/%127s", usr, host, port, path + 1) == 4 ||
        sscanf(url, "ftp://%31[^:@/]:%31[^:@/]@%63[^:@/]/%127s", usr, psw, host, path + 1) == 4 ||
        sscanf(url, "ftp://%31[^:@/]:%31[^:@/]@%63[^:@/]:%hu/%127s", usr, psw, host, port, path + 1) == 5)
    {
        log_i(LOG_FTP, "host=%s, port=%hu, user=%s, pswd=%s, path=%s",
              host, *port, usr, psw, path);
        return 0;
    }

    return -1;
}

static void *ftp_get(ftpreq_t *req)
{
    char tmpbuf[32];
    netbuf *ftp_conn, *ftp_file;
    unsigned int f_size, r_size, s_size, time;
    FILE *f_save = NULL;
    int res, err, id = req - ftp_reqs;

    res = FtpConnect(req->host, &ftp_conn);
    log_i(LOG_FTP, "<%d>:connecting server(%s:%hu)......%s",
          id, req->host, req->port, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_CONNECT;
        goto get_exit0;
    }

    res = FtpLogin(req->user, req->pswd, ftp_conn);
    log_i(LOG_FTP, "<%d>:logining to server(user:%s%s%s)......%s",
          id, req->user, req->pswd[0] ? ", pswd:" : "", req->pswd, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_LOGIN;
        goto get_exit1;
    }

    res = FtpSize(req->path, &f_size, FTPLIB_IMAGE, ftp_conn);
    sprintf(tmpbuf, "%u", f_size);
    log_i(LOG_FTP, "<%d>:getting remote file size......%s",
          id, res ? tmpbuf : "fail");

    if (!res)
    {
        err = FTP_ERR_REMFILE;
        goto get_exit1;
    }

    res = FtpAccess(req->path, FTPLIB_FILE_READ, FTPLIB_IMAGE, ftp_conn, &ftp_file);
    log_i(LOG_FTP, "<%d>:openning remote file(%s)......%s",
          id, req->path, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_REMFILE;
        goto get_exit1;
    }

    if (file_exists(req->fpath))
    {
        file_delete(req->fpath);
    }

    if (dir_make_path(req->fpath, S_IRUSR | S_IWUSR, true) == 0)
    {
        f_save = fopen(req->fpath, "wb");
    }

    log_i(LOG_FTP, "<%d>:creating local file(%s)......%s",
          id, req->fpath, STR_OK_FAIL(f_save));

    if (!f_save)
    {
        err = FTP_ERR_LOCFILE;
        goto get_exit2;
    }

    r_size = s_size = 0;
    time = tm_get_systick();

    while (r_size < f_size)
    {
        int g_size = FtpRead(req->buff, sizeof(req->buff), ftp_file);

        if (g_size < 0)
        {
            log_i(LOG_FTP, "<%d>:getting file......fail", id);
            err = FTP_ERR_TRANS;
            goto get_exit3;
        }

        if (fwrite(req->buff, 1, g_size, f_save) < g_size)
        {
            log_i(LOG_FTP, "<%d>:getting file......fail", id);
            err = FTP_ERR_LOCFILE;
            goto get_exit3;
        }

        r_size += g_size;

        if (tm_get_systick() - time >=  1000)
        {
            double spd = (double)(r_size - s_size) / 1024;
            double pct = (double)r_size / (double)f_size * 100;

            s_size = r_size;
            time = tm_get_systick();
            log_i(LOG_FTP, "<%d>:getting file......%.1lfkB/s, %.1lf%%", id, spd, pct);

            if (req->callback)
            {
                req->callback(FTP_PROCESS, (int)pct);
            }
        }

    }

    log_i(LOG_FTP, "<%d>:getting file......finished", id);
    err = 0;

get_exit3:
    fclose(f_save);
get_exit2:
    FtpClose(ftp_file);
get_exit1:
    FtpQuit(ftp_conn);
get_exit0:

    if (req->callback)
    {
        err ? req->callback(FTP_ERROR, err) : req->callback(FTP_FINISH, 0);
    }

    ftp_lock();
    req->used = 0;
    ftp_unlock();
    return NULL;
}

void *ftp_put(ftpreq_t *req)
{
    int f_size = 0, w_size, s_size, res, err, id = req - ftp_reqs;
    FILE *f_read;
    char tmpbuf[32], *f_name;
    netbuf *ftp_conn, *ftp_file;
    unsigned int time;

    f_read = fopen(req->fpath, "rb");
    log_i(LOG_FTP, "<%d>:openning local file(%s)......%s",
          id, req->fpath, STR_OK_FAIL(f_read));

    if (!f_read)
    {
        err = FTP_ERR_LOCFILE;
        goto put_exit0;
    }

    res = fseek(f_read, 0, SEEK_END) || (f_size = ftell(f_read)) < 0 || fseek(f_read, 0, SEEK_SET);
    sprintf(tmpbuf, "%d", f_size);
    log_i(LOG_FTP, "<%d>:getting local file size......%s",
          id, res ? "fail" : tmpbuf);

    if (res)
    {
        err = FTP_ERR_LOCFILE;
        goto put_exit1;
    }

    res = FtpConnect(req->host, &ftp_conn);
    log_i(LOG_FTP, "<%d>:connecting server(%s:%hu)......%s",
          id, req->host, req->port, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_CONNECT;
        goto put_exit1;
    }

    res = FtpLogin(req->user, req->pswd, ftp_conn);
    log_i(LOG_FTP, "<%d>:logining to server(user:%s%s%s)......%s",
          id, req->user, req->pswd[0] ? ", pswd:" : "", req->pswd, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_LOGIN;
        goto put_exit2;
    }

    FtpMkdir(req->path, ftp_conn);
    res = FtpChdir(req->path, ftp_conn);
    log_i(LOG_FTP, "<%d>:entering remote directory(%s)......%s",
          id, req->path, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_REMFILE;
        goto put_exit2;
    }

    f_name = strrchr(req->fpath, '/');
    f_name = f_name ? f_name + 1 : req->fpath;

    res = FtpAccess(f_name, FTPLIB_FILE_WRITE, FTPLIB_IMAGE, ftp_conn, &ftp_file);
    log_i(LOG_FTP, "<%d>:creating remote file(%s)......%s",
          id, f_name, STR_OK_FAIL(res));

    if (!res)
    {
        err = FTP_ERR_REMFILE;
        goto put_exit2;
    }

    w_size = s_size = 0;
    time = tm_get_systick();

    while (w_size < f_size)
    {
        int g_size = fread(req->buff, 1, sizeof(req->buff), f_read);

        if (g_size < sizeof(req->buff) && ferror(f_read))
        {
            log_i(LOG_FTP, "<%d>:putting file......fail", id);
            err = FTP_ERR_LOCFILE;
            goto put_exit3;
        }

        if (g_size > 0 && FtpWrite(req->buff, g_size, ftp_file) < g_size)
        {
            log_i(LOG_FTP, "<%d>:putting file......fail", id);
            err = FTP_ERR_TRANS;
            goto put_exit3;
        }

        w_size += g_size;

        if (tm_get_systick() - time >=  1000)
        {
            double spd = (double)(w_size - s_size) / 1024;
            double pct = (double)w_size / (double)f_size * 100;

            s_size = w_size;
            time = tm_get_systick();
            log_i(LOG_FTP, "<%d>:putting file......%.1lfkB/s, %.1lf%%", id, spd, pct);

            if (req->callback)
            {
                req->callback(FTP_PROCESS, (int)pct);
            }
        }
    }

    log_i(LOG_FTP, "<%d>:putting file......finished", id);
    err = 0;

put_exit3:
    FtpClose(ftp_file);
put_exit2:
    FtpQuit(ftp_conn);
put_exit1:
    fclose(f_read);
put_exit0:

    if (req->callback)
    {
        err ? req->callback(FTP_ERROR, err) : req->callback(FTP_FINISH, 0);
    }

    ftp_lock();
    req->used = 0;
    ftp_unlock();
    return NULL;
}


int ftp_request(int req, const char *url, const char *fpath, void (*callback)(int, int))
{
    pthread_t tid;
    pthread_attr_t ta;
    ftpreq_t *ftpreq;
    int i, ret = 0;

    log_i(LOG_FTP, "req= %s, url=%s, local file=%s", req ? "SEND" : "GET", url, fpath);

    ftp_lock();

    for (i = 0, ftpreq = NULL; i < FTP_MAX_REQ; i++)
    {
        if (!ftp_reqs[i].used)
        {
            ftpreq = ftp_reqs + i;
            ftpreq->used = 1;
            break;
        }
    }

    ftp_unlock();

    if (!ftpreq)
    {
        log_e(LOG_FTP, "FTP service is busy");
        ret = FTP_ERR_BUSY;
    }
    else if (ftp_paseurl(url, ftpreq->user, ftpreq->pswd, ftpreq->host, &ftpreq->port, ftpreq->path))
    {
        log_e(LOG_FTP, "invalid url: %s", url);
        ret = FTP_ERR_INVURL;
    }
    else
    {
        ftpreq->callback = callback;
        strncpy(ftpreq->fpath, fpath, 255);
        ftpreq->fpath[255] = 0;
        pthread_attr_init(&ta);
        pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&tid, &ta, req ? (void *)ftp_put : (void *)ftp_get, ftpreq) < 0)
        {
            log_e(LOG_FTP, "creating thread fail");
            ret = FTP_ERR_SYSTEM;
        }
    }

    if (ret)
    {
        if (ftpreq)
        {
            ftp_lock();
            ftpreq->used = 0;
            ftp_unlock();
        }

        if (callback)
        {
            callback(FTP_ERROR, ret);
        }
    }

    return ret;
}

const char *ftp_errstr(int err)
{
    static const char *errstr[] =
    {
        "no error",
        "failed in system calling",
        "FTP service is busy",
        "the URL is invalid",
        "failed in connecting",
        "failed in login",
        "failed in remote file accessing",
        "failed in local file accessing",
        "failed in remote directory accessing",
        "failed in transmission",
    };

    if (err < 0 || err > FTP_ERR_TRANS)
    {
        return "";
    }

    return errstr[err];
}

static void ftp_result(int evt, int par)
{
    switch (evt)
    {
        case FTP_ERROR:
            shellprintf(" FTP: error code %d, %s\r\n", par, ftp_errstr(par));
            break;

        case FTP_FINISH:
            shellprintf(" FTP: finished\r\n");
            break;

        case FTP_PROCESS:
            shellprintf(" FTP: process %d%%\r\n", par);
            break;

        default:
            shellprintf(" FTP: unknown event\r\n");
            break;
    }
}

static int ftp_test(int argc, const char **argv)
{
    int req;

    if (argc != 3)
    {
        shellprintf(" para error, usage: ftptst <u/d> <url> <local file>\r\n");
        return -1;
    }

    if (strcmp(argv[0], "d") == 0)
    {
        req = FTP_REQ_GET;
    }
    else if (strcmp(argv[0], "u") == 0)
    {
        req = FTP_REQ_PUT;
    }
    else
    {
        shellprintf(" usage: ftptst <u/d> <url> <local file>\r\n");
        return -1;
    }

    shellprintf(" start to test FTP, req=%s, url=%s, local file=%s\r\n",
                req ? "SEND" : "GET", argv[1], argv[2]);
    return ftp_request(req, argv[1], argv[2], ftp_result);
}

int ftp_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            pthread_mutex_init(&ftp_mutex, NULL);
            memset(ftp_reqs, 0, sizeof(ftp_reqs));

            break;

        case INIT_PHASE_OUTSIDE:
            ret |= shell_cmd_register("ftptst", ftp_test, "test FTP functions");
            FtpInit();

        default:
            break;
    }

    return ret;
}
