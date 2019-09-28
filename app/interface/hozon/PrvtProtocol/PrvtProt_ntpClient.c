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
#include "init.h"
#include "timer.h"
#include "log.h"
#include "dev_time.h"
#include "gb32960_api.h"

#define NTP_SERVICE_CNT 5
//char ntp_serverURL[NTP_SERVICE_CNT][64]= {"ntp1.aliyun.com","time1.aliyun.com","time.syn029.com", \
                                          "ntp.shu.edu.cn", "s2f.time.edu.cn"};
char ntp_serverURL[NTP_SERVICE_CNT][64]= {"120.25.115.20","203.107.6.88","1.83.125.83", \
                                          "202.120.127.191", "202.112.29.82"};
static unsigned char ntp_test_flag = 0; 
static int PP_ntp_service(char * serviceURL);
/*
*   ntp校准时间
*/
void PP_ntp_calibrationTime(void)
{
    static int i = 0;
    static uint64_t caltm_time = 0;
    
    if((!gb32960_networkSt() 
        || (dev_is_time_syn())) && !ntp_test_flag)
    {
        return;
    }

    if ((0 == caltm_time) || \
            (tm_get_time() - caltm_time > 20000))
    {
        i = (i < NTP_SERVICE_CNT)? i : 0;
        if(PP_ntp_service(ntp_serverURL[i]))
        {
            log_e(LOG_HOZON, "adjust time form %s faile,ntp:%d",ntp_serverURL[i],i);
            i++;
        }
        else
        {
            log_i(LOG_HOZON, "ntp adjust time success!\n");
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
    sprintf(line , "ntp server URL: %s\n", serviceURL);

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