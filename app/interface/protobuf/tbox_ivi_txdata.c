/******************************************************
文件名：	tbox_ivi_txdata.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
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
#include "../../support/protocol.h"

#include "tbox_ivi_txdata.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/

static HU_Send_t  HU_datamem[HU_MAX_SENDQUEUE];
static list_t     HU_free_lst;
static list_t     HU_realtm_lst;
static list_t     HU_trans_lst;

static pthread_mutex_t HU_txmtx = PTHREAD_MUTEX_INITIALIZER;//初始化静态锁
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void HU_data_clearqueue(void);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：SP_data_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void HU_data_init(void)
{
	HU_data_clearqueue();
}

/******************************************************
*函数名：SP_data_clearqueue

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
static void HU_data_clearqueue(void)
{
    int i;

    list_init(&HU_trans_lst);
    list_init(&HU_realtm_lst);
    list_init(&HU_free_lst);

    for (i = 0; i < HU_MAX_SENDQUEUE; i++)
    {
        list_insert_before(&HU_free_lst, &HU_datamem[i].link);
    }

}

/******************************************************
*函数名：SP_data_write

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
void HU_data_write(uint8_t *data,int len,SP_sendInform_cb sendInform_cb,void *cb_para)
{

	pthread_mutex_lock(&HU_txmtx);

	int i;
	HU_Send_t *rpt;
	list_t *node;

	if(len <= HU_SENDBUFLNG)
	{
		if (((node = list_get_first(&HU_free_lst)) != NULL) || \
				((node = list_get_first(&HU_trans_lst)) != NULL))
		{
			rpt = list_entry(node, HU_Send_t, link);
			rpt->msglen  = len;
			for(i = 0;i < len;i++)
			{
				rpt->msgdata[i] = data[i];
			}
			rpt->list = &HU_realtm_lst;
			rpt->SendInform_cb = sendInform_cb;
			rpt->Inform_cb_para = cb_para;
			list_insert_before(&HU_realtm_lst, node);
		}
		else
		{
			 log_e(LOG_IVI, "BIG ERROR: no buffer to use.");
			 HU_data_init();
		}
	}
	else
	{
		log_e(LOG_IVI, "data is too long.");
	}

	pthread_mutex_unlock(&HU_txmtx);
}

/******************************************************
*函数名：SP_data_get_pack

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
HU_Send_t *HU_data_get_pack(void)
{
    list_t *node = NULL;

    pthread_mutex_lock(&HU_txmtx);

    node = list_get_first(&HU_realtm_lst);

    pthread_mutex_unlock(&HU_txmtx);

    return node == NULL ? NULL : list_entry(node, HU_Send_t, link);;
}


void HU_data_put_back(HU_Send_t *pack)
{
	pthread_mutex_lock(&HU_txmtx);
    list_insert_after(pack->list, &pack->link);
    pthread_mutex_unlock(&HU_txmtx);
}

void HU_data_put_send(HU_Send_t *pack)
{
    pthread_mutex_lock(&HU_txmtx);
    list_insert_before(&HU_trans_lst, &pack->link);
    pthread_mutex_unlock(&HU_txmtx);
}

void HU_data_ack_pack(void)
{
    list_t *node;

    pthread_mutex_lock(&HU_txmtx);

    if ((node = list_get_first(&HU_trans_lst)) != NULL)
    {
        list_insert_before(&HU_free_lst, node);
    }

    pthread_mutex_unlock(&HU_txmtx);
}
