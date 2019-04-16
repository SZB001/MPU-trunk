#ifndef __FTP_API_H__
#define __FTP_API_H__

enum
{
    FTP_FINISH,
    FTP_ERROR,
    FTP_PROCESS,
};

enum
{
    FTP_ERR_SYSTEM = 1,
    FTP_ERR_BUSY,
    FTP_ERR_INVURL,
    FTP_ERR_CONNECT,
    FTP_ERR_LOGIN,
    FTP_ERR_REMFILE,
    FTP_ERR_LOCFILE,
    FTP_ERR_REMIDR,
    FTP_ERR_TRANS,
    FTP_ERR_STORE,
};

enum
{
    FTP_REQ_GET,
    FTP_REQ_PUT,
};

extern int ftp_request(int req, const char *url, const char *fpath, void (*callback)(int, int));
extern int ftp_init(int phase);
extern const char *ftp_errstr(int err);

#endif
