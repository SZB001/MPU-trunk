/****************************************************************
file:         ring_buffer.c
description:  the source file of ring_buffer implementation
date:         2016/9/22
author        yuzhimin
****************************************************************/

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "log.h"
#include "ring_buffer.h"

/*************************************************************
function:     rb_init
description:  ring_buffer initialization
input:        char *buffer,pointer to a buffer space
              unsigned int size,buffer space size
output:       none
return:       ring_buffer  pointer of ring_buffer type
**************************************************************/
int rb_init(struct ring_buffer *ring_buf, unsigned char *buffer, unsigned int size)
{
    if (!is_power_of_2(size))
    {
        log_e(LOG_MID, "size must be power of 2.");
        return -1;
    }

    memset(ring_buf, 0, sizeof(struct ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in  = 0;
    ring_buf->out = 0;

    return 0;
}

/**************************************************************
function:     rb_used_len
description:  Idle buffer size
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
output:       none
return:       the size of used buffer
***************************************************************/
unsigned int rb_used_len(const struct ring_buffer *ring_buf)
{
    return (ring_buf->in - ring_buf->out);
}

/**************************************************************
function:     rb_used_len
description:  Idle buffer size
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
output:       none
return:       the size of idle buffer
***************************************************************/
unsigned int rb_unused_len(const struct ring_buffer *ring_buf)
{
    return (ring_buf->size - ring_buf->in + ring_buf->out);
}

/**************************************************************
function:     rb_empty
description:  chech whether the ring buffer is empty
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
output:       none
return:       1 indicates the ring buffer is empty;
              0 indicates the ring buffer is not empty;
***************************************************************/
int rb_empty(const struct ring_buffer *ring_buf)
{
    return (ring_buf->in == ring_buf->out);
}

/**************************************************************
function:     rb_out
description:  get data from ring buffer
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
              unsigned int size, data length
output:       char *buffer, save the data to data buffer
return:       the actual size of get out data
***************************************************************/
int rb_out(struct ring_buffer *ring_buf, unsigned char *buffer, unsigned int size)
{
    unsigned int len = 0;


    if (NULL == ring_buf ||  NULL == buffer)
    {
        return -1;
    }

    size  = min(size, ring_buf->in - ring_buf->out);
    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest(if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;

    if (ring_buf->in == ring_buf->out)
    {
        ring_buf->in = ring_buf->out = 0;
    }

    return size;
}

/**************************************************************
function:     rb_get
description:  get data from ring buffer
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
              unsigned int size, data length
output:       char *buffer, save the data to data buffer
return:       the actual size of get out data
***************************************************************/
int rb_get(struct ring_buffer *ring_buf, unsigned char *buffer,
           unsigned int offset, unsigned int size)
{
    unsigned int len = 0;
    unsigned int out;

    if (NULL == ring_buf ||  NULL == buffer)
    {
        return -1;
    }

    out = (ring_buf->out + offset) & (ring_buf->size - 1);

    size  = min(size, ring_buf->in - out);
    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (out & (ring_buf->size - 1)));
    memcpy(buffer, ring_buf->buffer + (out & (ring_buf->size - 1)), len);
    /* then get the rest(if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);

    return size;
}

/********************************************************************
function:     rb_in
description:  put data to ring_buffer
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
              const char *buffer, the put in ring_buffer data
              unsigned int size, data length
output:       none
return:       the actual size of put in data
********************************************************************/
int rb_in(struct ring_buffer *ring_buf, const unsigned char *buffer, unsigned int size)
{
    unsigned int len = 0;

    if (NULL == ring_buf ||  NULL == buffer)
    {
        return -1;
    }

    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
    /* first put the data starting from fifo->in to buffer end */
    len  = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
    memcpy(ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
    /* then put the rest(if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, buffer + len, size - len);
    ring_buf->in += size;

    return size;
}

/*******************************************************************
function:     rb_clean
description:  clear ring_buffer data
input:        ring_buffer *ring_buf, the pointer of ring_buffer type
output:       none
return:       none
********************************************************************/
void rb_clean(struct ring_buffer *ring_buf)
{
    assert(ring_buf);
    memset(ring_buf->buffer, 0, ring_buf->size);
    ring_buf->in = ring_buf->out = 0;
    return;
}

