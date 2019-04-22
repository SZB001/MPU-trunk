/****************************************************************
 file:         at_queue.h
 description:  the header file of at_queue definition
 date:         2016/9/22
 author        yuzhimin
 ****************************************************************/
#ifndef __AT_QUEUE_H__
#define __AT_QUEUE_H__
#include <stdbool.h>
#include "at_cmd.h"

typedef struct
{
    int head;
    int tail;
    AT_CMD_T at_cmd[AT_CMD_QUEUE_NUM];
} AT_QUEUE, *P_AT_QUEUE;

void at_init_queue(void);

bool at_is_full(P_AT_QUEUE q);
bool at_is_empty(P_AT_QUEUE q);

void at_clear_queue(P_AT_QUEUE q);
int at_queue_get_len(P_AT_QUEUE q);

bool at_dequeue(P_AT_QUEUE q, AT_CMD_T *at);
bool at_enqueue(P_AT_QUEUE q, const AT_CMD_T *at);

extern AT_QUEUE at_high;
extern AT_QUEUE at_normal;

#endif  /*!__AT_QUEUE_H__*/

