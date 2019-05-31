/******************************************************
文件名：	PrvtProt_rmtCtrl_data.c

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
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"

#include "../sockproxy/sockproxy_data.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../../support/protocol.h"
#include "hozon_SP_api.h"
#include "shell_api.h"
//#include "../PrvtProt_shell.h"
//#include "../PrvtProt_queue.h"
//#include "../PrvtProt_EcDc.h"
//#include "../PrvtProt_cfg.h"
//#include "../PrvtProt.h"
#include "PP_doorLockCtrl.h"
#include "PP_ACCtrl.h"
#include "PP_ChargeCtrl.h"
#include "PP_rmtCtrl_data.h"

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/

static PrvtProt_RmtCtrlSend_t  rmtCtrl_datamem[PP_RMTCTRL_MAX_SENDQUEUE];
static list_t     rmtCtrl_free_lst;
static list_t     rmtCtrl_realtm_lst;

static pthread_mutex_t PP_txdatamtx = PTHREAD_MUTEX_INITIALIZER;//初始化静态锁
/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void PP_data_rmtCtrl_clearqueue(void);
/******************************************************
description： function code
******************************************************/
/******************************************************
*函数名：PP_rmtCtrl_init

*形  参：void

*返回值：void

*描  述：初始化

*备  注：
******************************************************/
void PP_rmtCtrl_data_init(void)
{
	PP_data_rmtCtrl_clearqueue();
}

/******************************************************
*函数名：PP_data_rmtCtrl_clearqueue

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
static void PP_data_rmtCtrl_clearqueue(void)
{
    int i;

    list_init(&rmtCtrl_realtm_lst);
    list_init(&rmtCtrl_free_lst);

    for (i = 0; i < PP_RMTCTRL_MAX_SENDQUEUE; i++)
    {
        list_insert_before(&rmtCtrl_free_lst, &rmtCtrl_datamem[i].link);
    }

}

/******************************************************
*函数名：PP_rmtCtrl_data_write

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
void PP_rmtCtrl_data_write(uint8_t *data,int len,PP_rmtCtrlsendInform_cb sendInform_cb,void *cb_para)
{

	pthread_mutex_lock(&PP_txdatamtx);

	int i;
	PrvtProt_RmtCtrlSend_t *rpt;
	list_t *node;

	if(len <= PP_RMTCTRL_SENDBUFLNG)
	{
		if ((node = list_get_first(&rmtCtrl_free_lst)) != NULL)
		{
			rpt = list_entry(node, PrvtProt_RmtCtrlSend_t, link);
			rpt->msglen  = len;
			for(i = 0;i < len;i++)
			{
				rpt->msgdata[i] = data[i];
			}
			rpt->list = &rmtCtrl_realtm_lst;
			rpt->SendInform_cb = sendInform_cb;
			rpt->Inform_cb_para = cb_para;
			list_insert_before(&rmtCtrl_realtm_lst, node);
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

	pthread_mutex_unlock(&PP_txdatamtx);
}

/******************************************************
*函数名：PP_rmtCtrl_data_get_pack

*形  参：void

*返回值：void

*描  述：

*备  注：
******************************************************/
PrvtProt_RmtCtrlSend_t *PP_rmtCtrl_data_get_pack(void)
{
    list_t *node = NULL;

    pthread_mutex_lock(&PP_txdatamtx);

    node = list_get_first(&rmtCtrl_realtm_lst);

    pthread_mutex_unlock(&PP_txdatamtx);

    return node == NULL ? NULL : list_entry(node, PrvtProt_RmtCtrlSend_t, link);;
}


void PP_rmtCtrl_data_put_back(PrvtProt_RmtCtrlSend_t *pack)
{
	pthread_mutex_lock(&PP_txdatamtx);
    list_insert_after(pack->list, &pack->link);
    pthread_mutex_unlock(&PP_txdatamtx);
}
