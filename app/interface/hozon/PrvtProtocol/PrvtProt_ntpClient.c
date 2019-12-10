/******************************************************
�ļ�����	PrvtProt_ntpClient.c

������	ntp校时	
Data			Vasion			author
2019/9/27		V1.0			liujian
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <pthread.h>
#include <sys/prctl.h>
#include "init.h"
#include "timer.h"
#include "log.h"
#include "dev_time.h"
#include "gb32960_api.h"
#include "shell_api.h"

#define NTP_SERVICE_CNT 6
char ntp_serverURL[NTP_SERVICE_CNT][64]= {"cn.ntp.org.cn","ntp1.aliyun.com","time1.aliyun.com","time.syn029.com", \
                                          "ntp.shu.edu.cn", "s2f.time.edu.cn"};
static unsigned char ntp_test_flag; 
static int ntp_srv_cnt;
static int PP_ntp_service(char * serviceURL);
static int PP_ntp_shell_setntpaddr(int argc , const const char **argv);
static void PP_ntp_calibrationTime(void);
static void *PP_ntp_main(void);
static uint64_t caltm_time;
/*
* ntp校时初始化
*/
void PP_ntp_Init(void)
{
    ntp_test_flag = 0;
    ntp_srv_cnt = 0;
    shell_cmd_register("hozon_setntpaddr", PP_ntp_shell_setntpaddr, "set ntp addr");
    caltm_time = tm_get_time();
}

void PP_ntp_run(void)
{
    int ret;
    pthread_t ntptid;
    pthread_attr_t ntpta;

    pthread_attr_init(&ntpta);
    pthread_attr_setdetachstate(&ntpta, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&ntptid, &ntpta, (void *)PP_ntp_main, NULL);
    if (ret != 0)
    {
        log_e(LOG_HOZON, "ntp calibrationTime pthread create failed, error: %s", strerror(errno));
    }
}

static void *PP_ntp_main(void)
{
    log_o(LOG_HOZON, "ntp calibrationTime thread running");
    prctl(PR_SET_NAME, "NTP_CALI");
    while(1)
    {
        PP_ntp_calibrationTime();
    }
    return NULL;
}

/*
*   ntp校准时间
*/
static void PP_ntp_calibrationTime(void)
{
    if((!gb32960_networkSt() 
        || (dev_is_time_syn())) && (!ntp_test_flag))
    {
        return;
    }

    if(tm_get_time() - caltm_time > 20000)
    {
        ntp_srv_cnt = (ntp_srv_cnt < NTP_SERVICE_CNT)? ntp_srv_cnt : 0;
        if(PP_ntp_service(ntp_serverURL[ntp_srv_cnt]))
        {
            log_e(LOG_HOZON, "adjust time form %s faile,ntp:%d",ntp_serverURL[ntp_srv_cnt],ntp_srv_cnt);
            ntp_srv_cnt++;
        }
        else
        {
            log_i(LOG_HOZON, "ntp adjust time success!\n");
            ntp_test_flag = 0;
        }
        
        caltm_time = tm_get_time();
    }
}

static int PP_ntp_service(char * serviceURL)
{
    FILE *ptream;
    char line[256];
    int ret = 1;
    
    memset(line , 0 ,sizeof(line));
    sprintf(line , "ntpdate %s", serviceURL);

    /*start ntp calibration*/
    ptream = popen(line,"r");
    if (NULL != ptream)
    {   
        RTCTIME time;
        char *string;
        memset(line , 0 , sizeof(line) );
        if(fgets(line,256 , ptream) != NULL)
        {
            log_i(LOG_HOZON, "ntp:%s",line);
            if( NULL != (string = strstr(line,"step time"))
                || NULL != (string = strstr(line,"adjust time")))
            {
                if (NULL != strstr(string,"offset"))
                {
                    log_i(LOG_HOZON,"adjust time form %s",serviceURL); 
                    tm_get_abstime(&time);
                    dev_syn_time(&time , GNSS_TIME_SOURCE);
                    ret = 0;
                }    
            }
        }

        pclose(ptream);
    }

    return ret;
}

/*
*   ntp校时请求
*/
void PP_SetNTPTime(unsigned char ntpreq)
{
    ntp_test_flag = ntpreq;
    log_o(LOG_HOZON, "ntp calibration time request\n");
}

/*
*  设置ntp addr
*/
static int PP_ntp_shell_setntpaddr(int argc , const const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: ntpaddr <addr>\r\n");
        return -1;
    }

    ntp_srv_cnt = 0; 
    log_o(LOG_HOZON,"ntp addr %s",argv[0]);
    strcpy(ntp_serverURL[0] ,argv[0]);
    sleep(1);

    return 0;
}