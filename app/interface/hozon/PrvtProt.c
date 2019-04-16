/******************************************************
�ļ�����	PrvtProt.c

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

#include <sys/prctl.h>
//#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "PrvtProt.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/


/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static void *PrvtProt_main(void);

/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PrvtProt_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
int PrvtProt_init(INIT_PHASE phase)
{
    int ret = 0;
    uint32_t reginf = 0, cfglen;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:

            break;
    }

    return ret;
}


/******************************************************
*��������PrvtProt_run

*��  �Σ�void

*����ֵ��void

*��  �������������߳�

*��  ע��
******************************************************/
int PrvtProt_run(void)
{
    int ret = 0;
    pthread_t tid;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    ret = pthread_create(&tid, &ta, (void *)PrvtProt_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_HOZON, "pthread_create failed, error: %s", strerror(errno));
        return ret;
    }

    return 0;
}


/******************************************************
*��������PrvtProt_main

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
static void *PrvtProt_main(void)
{
	log_o(LOG_HOZON, "proprietary protocol  of hozon thread running");

    prctl(PR_SET_NAME, "HZpp");
    while (1)
    {
        /*res = gb_do_checksock(&state) ||	//���socket����
              gb_do_receive(&state) ||		//socket���ݽ���
              gb_do_wait(&state) ||			//��ʱ�ȴ��������ǳ���
              gb_do_login(&state) ||		//����
              gb_do_suspend(&state) ||		//ͨ����ͣ
              gb_do_report(&state) ||		//����ʵʱ��Ϣ
              gb_do_logout(&state);			//�ǳ�
*/
    }

    return NULL;
}
