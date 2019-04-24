#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__
#include "tcom_api.h"

#ifndef	MIN
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#endif
#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define ABS(d)              ((d) < 0 ? -(d) : (d))
#define GET_WORD(data)      GET_HWBE(data)
#define GET_DWORD(data)     GET_WDBE(data)

#define GET_BYTE(data)      (*(data))
#define GET_HWBE(data)      (GET_BYTE(data) * 0x100 + GET_BYTE((data) + 1))
#define GET_HWLE(data)      (GET_BYTE(data) + GET_BYTE((data) + 1) * 0x100)
#define GET_WDBE(data)      (GET_HWBE(data) * 0x10000 + GET_HWBE((data) + 2))
#define GET_WDLE(data)      (GET_HWLE(data) + GET_HWBE((data) + 2) * 0x10000)
#define ARRAY_SZ(array)     (sizeof(array) / sizeof(array[0]))
#define ARRAY_LMT(array)    ((array) + ARRAY_SZ(array))
#define BYTE2BCD(d)         ((d) % 10 + (d) % 100 / 10 * 16)
#define BITMSK(n)           (1 << (n))
#define BYTELSB(d)          ((d) & 0x01 ? 0 : (d) & 0x02 ? 1 :\
                             (d) & 0x04 ? 2 : (d) & 0x08 ? 3 :\
                             (d) & 0x10 ? 4 : (d) & 0x20 ? 5 :\
                             (d) & 0x40 ? 6 : (d) & 0x80 ? 7 : -1)
#define RDUP_DIV(v, d)      (((v) + (d) - 1) / (d))
typedef struct
{
    char    url[256];
    uint16_t port;
} /*__attribute__ ((packed))*/ svr_addr_t;

extern int protocol_wait_msg(uint16_t mid, int pipe, TCOM_MSG_HEADER *msg, void *msgdata,
                             int timeout);
extern void protocol_dump(uint16_t logid, const char *title, const uint8_t *buf, int len, int dir);

#endif
