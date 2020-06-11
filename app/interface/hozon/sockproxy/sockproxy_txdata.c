/******************************************************
�ļ�����	sockproxy_txdata.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>

#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */

#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"

#include "sockproxy_txdata.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/

static SP_Send_t  SP_datamem[SP_MAX_SENDQUEUE];
static list_t     SP_free_lst;
static list_t     SP_realtm_lst;
static list_t     SP_trans_lst;

static pthread_mutex_t SP_txmtx = PTHREAD_MUTEX_INITIALIZER;//��ʼ����̬��
/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void SP_data_clearqueue(void);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������SP_data_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void SP_data_init(void)
{
	SP_data_clearqueue();
}

/******************************************************
*��������SP_data_clearqueue

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
static void SP_data_clearqueue(void)
{
    int i;

    list_init(&SP_trans_lst);
    list_init(&SP_realtm_lst);
    list_init(&SP_free_lst);

    for (i = 0; i < SP_MAX_SENDQUEUE; i++)
    {
        list_insert_before(&SP_free_lst, &SP_datamem[i].link);
    }

}

/******************************************************
*��������SP_data_write

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
void SP_data_write(uint8_t *data,int len,SP_sendInform_cb sendInform_cb,void *cb_para)
{

	pthread_mutex_lock(&SP_txmtx);

	int i;
	SP_Send_t *rpt;
	list_t *node;

	if(len <= SP_SENDBUFLNG)
	{
		if (((node = list_get_first(&SP_free_lst)) != NULL) || \
				((node = list_get_first(&SP_trans_lst)) != NULL))
		{
			rpt = list_entry(node, SP_Send_t, link);
			rpt->msglen  = len;
			for(i = 0;i < len;i++)
			{
				rpt->msgdata[i] = data[i];
			}
			rpt->list = &SP_realtm_lst;
			rpt->SendInform_cb = sendInform_cb;
			rpt->Inform_cb_para = cb_para;
			list_insert_before(&SP_realtm_lst, node);
		}
		else
		{
			 log_e(LOG_HOZON, "BIG ERROR: no buffer to use.");
		}
	}
	else
	{
		log_e(LOG_HOZON, "data is too long.");
	}

	pthread_mutex_unlock(&SP_txmtx);
}

/******************************************************
*��������SP_data_get_pack

*��  �Σ�void

*����ֵ��void

*��  ����

*��  ע��
******************************************************/
SP_Send_t *SP_data_get_pack(void)
{
    list_t *node = NULL;

    pthread_mutex_lock(&SP_txmtx);

    node = list_get_first(&SP_realtm_lst);

    pthread_mutex_unlock(&SP_txmtx);

    return node == NULL ? NULL : list_entry(node, SP_Send_t, link);;
}


void SP_data_put_back(SP_Send_t *pack)
{
	pthread_mutex_lock(&SP_txmtx);
    list_insert_after(pack->list, &pack->link);
    pthread_mutex_unlock(&SP_txmtx);
}

void SP_data_put_send(SP_Send_t *pack)
{
    pthread_mutex_lock(&SP_txmtx);
    list_insert_before(&SP_trans_lst, &pack->link);
    pthread_mutex_unlock(&SP_txmtx);
}

void SP_data_ack_pack(void)
{
    list_t *node;

    pthread_mutex_lock(&SP_txmtx);

    if ((node = list_get_first(&SP_trans_lst)) != NULL)
    {
        list_insert_before(&SP_free_lst, node);
    }

    pthread_mutex_unlock(&SP_txmtx);
}

void SP_data_showNodeCount(void)
{
	log_o(LOG_HOZON, "free node num:%d\n",list_count(&SP_free_lst));
	log_o(LOG_HOZON, "realtm node num:%d\n",list_count(&SP_realtm_lst));
	log_o(LOG_HOZON, "trans node num:%d\n",list_count(&SP_trans_lst));
}
