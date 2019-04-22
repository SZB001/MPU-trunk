/****************************************************************
 file:         at_queue.c
 description:  the source file of at_queue implementation
 date:         2016/9/22
 author        yuzhimin
 ****************************************************************/
#include <string.h>
#include <assert.h>
#include "at_queue.h"

AT_QUEUE at_high;
AT_QUEUE at_normal;

/****************************************************************
 function:     _at_init_queue_
 description:  init queue
 input:        P_AT_QUEUE q
 output:       none
 return:       none
 ****************************************************************/
static void _at_init_queue_(P_AT_QUEUE q)
{
    if (NULL == q)
    {
        return;
    }

    q->head = 0;
    q->tail = 0;
    memset(q->at_cmd, 0, sizeof(AT_CMD_T) * AT_CMD_QUEUE_NUM);
    return;
}

/****************************************************************
 function:     at_is_empty
 description:  whether the queue is empty
 input:        P_AT_QUEUE q
 output:       none
 return:       false indicates is not empty;
 true indicates is empty
 ****************************************************************/
bool at_is_empty(P_AT_QUEUE q)
{
    if (NULL == q)
    {
        return false;
    }

    if (q->head == q->tail)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************
 function:     at_is_full
 description:  whether the queue is full
 input:        P_AT_QUEUE q
 output:       none
 return:       false indicates is not full;
 true indicates is full
 ****************************************************************/
bool at_is_full(P_AT_QUEUE q)
{
    if (NULL == q)
    {
        return false;
    }

    if (q->head == (q->tail + 1) % AT_CMD_QUEUE_NUM)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/****************************************************************
 function:     at_enqueue
 description:  push cmd to queue
 input:        P_AT_QUEUE q
 const AT_CMD_T* at
 output:       none
 return:       false indicates failed;
 true indicates successful
 ****************************************************************/
bool at_enqueue(P_AT_QUEUE q, const AT_CMD_T *at)
{
    if (NULL == q || NULL == at)
    {
        return false;
    }

    if (true == at_is_full(q))
    {
        return false;
    }
    else
    {
        q->at_cmd[q->tail].id = at->id;
        q->at_cmd[q->tail].len = at->len;
        memcpy(q->at_cmd[q->tail].str, at->str, at->len);
        q->tail = (q->tail + 1) % AT_CMD_QUEUE_NUM;
        return true;
    }
}

/****************************************************************
 function:     at_dequeue
 description:  pull cmd from queue
 input:        P_AT_QUEUE q
 const AT_CMD_T* at
 output:       none
 return:       false indicates failed;
 true indicates successful
 ****************************************************************/
bool at_dequeue(P_AT_QUEUE q, AT_CMD_T *at)
{
    if (NULL == q || NULL == at)
    {
        return false;
    }

    if (true == at_is_empty(q))
    {
        return false;
    }
    else
    {
        at->id = q->at_cmd[q->head].id;
        at->len = q->at_cmd[q->head].len;
        memcpy(at->str, q->at_cmd[q->head].str, at->len);
        q->head = (q->head + 1) % AT_CMD_QUEUE_NUM;
        return true;
    }
}

/****************************************************************
 function:     at_clear_queue
 description:  clean queue by q
 input:        P_AT_QUEUE q
 output:       none
 return:       none
 ****************************************************************/
void at_clear_queue(P_AT_QUEUE q)
{
    _at_init_queue_(q);
}

/****************************************************************
 function:     at_queue_get_len
 description:  get the length of queue
 input:        P_AT_QUEUE q
 output:       none
 return:       int
 ****************************************************************/
int at_queue_get_len(P_AT_QUEUE q)
{
    if (NULL == q)
    {
        return false;
    }

    if (q->tail >= q->head)
    {
        return (q->tail - q->head);
    }
    else
    {
        return (q->tail - q->head + AT_CMD_QUEUE_NUM);
    }
}

/****************************************************************
 function:     at_init_queue
 description:  init the two queue
 input:        none
 output:       none
 return:       none
 ****************************************************************/
void at_init_queue(void)
{
    _at_init_queue_(&at_high);
    _at_init_queue_(&at_normal);
}

