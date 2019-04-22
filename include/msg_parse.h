#ifndef __MSG_PARSE_H__
#define __MSG_PARSE_H__

#include "log.h"

#define DCCP_PKG_MARKER                 (0x7E)
#define DCCP_PKG_ESC                    (0x7D)
#define DCCP_PKG_MARKER_SIZE            (1)
#define DCCP_PKG_CS_SIZE                (1)


enum MSG_SEG
{
    FIRST_SEG = 0,
    MIDDLE_SEG,
    LAST_SEG,
};

typedef struct
{
    int used;
    int size;
    unsigned char *data;
} MSG_RX;

typedef void (*msg_proc)(unsigned char *msg, unsigned int len);
typedef void (*msg_handler)(unsigned char *msg, unsigned int len, void *para);

static inline void msg_init_rx(MSG_RX *rx, unsigned char *buf, int size)
{
    assert(rx || buf);
    memset(buf, 0, size);
    rx->data  = buf;
    rx->size  = size;
    rx->used = 0;
}

static inline unsigned char msg_checksum(const unsigned char *buf, int len, unsigned char cs)
{
    int i;
    volatile unsigned char checksum = cs;

    if (buf == NULL || len <= 0)
    {
        return 0;
    }

    for (i = 0; i < len; ++i)
    {
        checksum = checksum ^ buf[i];
    }

    return checksum;
}

void msg_decodex(MSG_RX *rx, msg_handler msg_proc, void *para);
void msg_decode(int *r_pos, int *w_pos, unsigned char *msg, msg_proc msg_proc);

int msg_encode(unsigned char *imsg, unsigned int ilen, unsigned char *omsg, unsigned int olen,
               unsigned char flag , unsigned char *base_cs);


#endif


