#ifndef __HTTP_H__
#define __HTTP_H__
#include <stdint.h>
enum
{
    HTTP_EVENT_FINISH,
    HTTP_EVENT_PROCESS,
    HTTP_EVENT_ERROR,
};

extern int http_download_request(const char *url, const char *fpath, uint8_t *md5, int (*cb)(int, int));
extern int http_post_msg(const char *url, const char *msg);

#endif