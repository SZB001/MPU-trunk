#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "com_app_def.h"
#include "tcom_api.h"
#include "init.h"
#include "pm_api.h"
#include "dev_api.h"
#include "ap_api.h"
#include "cfg_api.h"
#include "sock_api.h"
#include "nm_api.h"
#include "ap_data.h"
#include "timer.h"
#include "../support/protocol.h"
#include "../fota/fota_foton.h"


static pthread_t ap_tid; /* thread id */
static uint8_t   ap_msgbuf[TCOM_MAX_MSG_LEN];
extern unsigned char ecu_selfupgrade_readversion;
void selfupgrade_mpu_only_report(void);
void mpu_self_file_exists(void);

timer_t ap_check_timer;
AP_SOCK_INFO apinfo;
static int ap_allow_sleep = 0;
unsigned char ecu_upgrade_finish_report_ecu_info_flag = 0;

static int ap_nm_callback(NET_TYPE type, NM_STATE_MSG nmmsg)
{
    TCOM_MSG_HEADER msg;
    unsigned int   network;

    if (NM_PUBLIC_NET != type)
    {
        return 0;
    }

    switch (nmmsg)
    {
        case NM_REG_MSG_CONNECTED:
            network = AP_MSG_NET_CONNECT;
            break;

        case NM_REG_MSG_DISCONNECTED:
            network = AP_MSG_NET_DISCONNECT;
            break;

        default:
            return -1;
    }

    msg.msgid    = network;
    msg.sender   = MPU_MID_AUTO;
    msg.receiver = MPU_MID_AUTO;
    msg.msglen   = 0;

    return tcom_send_msg(&msg, NULL);
}

static int ap_allow_sleep_handler(PM_EVT_ID id)
{
    return ap_allow_sleep;
}

int ap_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(&apinfo, 0 ,sizeof(apinfo));
            apinfo.sockfd = -1;
            apinfo.connect_st = 0;
            apinfo.network = 0;
            ap_allow_sleep = 0;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                unsigned int len;
                
                mpu_self_file_exists();
                
                len = sizeof(apinfo.url);
                ret |= cfg_get_para(CFG_ITEM_FTTSP_URL, apinfo.url, &len);

                len = sizeof(apinfo.port);
                ret |= cfg_get_para(CFG_ITEM_FTTSP_PORT, &apinfo.port, &len);

                ret |= tm_create(TIMER_REL, AP_MSG_TIMER_EVENT, MPU_MID_AUTO, &ap_check_timer);
                
                ret |= nm_reg_status_changed(NM_PUBLIC_NET, ap_nm_callback);
                ret |= pm_reg_handler(MPU_MID_AUTO, ap_allow_sleep_handler);
                break;
            }

        default:
            break;
    }

    ret |= ap_data_init(phase);

    return ret;
}


int ap_sock_init(void)
{
    int sockfd = -1;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        log_e(LOG_AUTO, " create socket fail: %s", strerror(errno));
        return -1;
    }

    return sockfd;
}

int ap_sock_connect(int sockfd, char *url, uint16_t port)
{
    int ret;
    struct sockaddr_in addr;
    static int con_interval = 10;

    if (con_interval < 10)
    {
        con_interval++;
        //return -1;
    }

    con_interval = 0;

    //nm_bind(sockfd, NM_PUBLIC_NET);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = inet_addr(url);

    log_i(LOG_AUTO, "socket(%d) is begin connect", sockfd);
    ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr));

    #if 0
    if (ret == 0)
    {
        log_e(LOG_AUTO, "socket(auto) is connected successful");
    }
    else
    {
        log_e(LOG_AUTO, "socket(auto) is connected failed");
    }

    #endif
    return ret;
}

/****************************************************************
 function:     ap_main
 description:  at module main function
 input:        none
 output:       none
 return:       NULL
 ****************************************************************/
static void *ap_main(void)
{
    int ret, tcom_fd, max_fd;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;
    static unsigned char recv_buf[2 * 1024];
    static unsigned int  recv_len = 0;

    prctl(PR_SET_NAME, "AUTO");

    tcom_fd = tcom_get_read_fd(MPU_MID_AUTO);

    if (tcom_fd < 0)
    {
        log_e(LOG_AUTO, "get autopilot recv fd failed");
        return NULL;
    }
    
    max_fd = tcom_fd;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(tcom_fd, &fds);
        if(apinfo.sockfd > 0)
        {
            if (apinfo.sockfd > tcom_fd)
            {
                max_fd = apinfo.sockfd;
            }
            FD_SET(apinfo.sockfd, &fds);
        }
        
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (ret)
        {
            if (FD_ISSET(tcom_fd, &fds))
            {
                ret = tcom_recv_msg(MPU_MID_AUTO, &msgheader, ap_msgbuf);

                if (ret != 0)
                {
                    log_e(LOG_AUTO, "tcom_recv_msg failed,ret:0x%08x", ret);
                    continue;
                }

                switch (msgheader.msgid)
                {
                    case MPU_MID_MID_PWDG:
                        pwdg_feed(MPU_MID_AUTO);
                        break;
                    case AP_MSG_NET_CONNECT:
                    {
                        apinfo.network = 1;
                        
                        if (apinfo.sockfd < 0)
                        {
                            apinfo.sockfd = ap_sock_init();
                            if (apinfo.sockfd < 0)
                            {
                                log_e(LOG_AUTO, "create sock failed");
                                break;
                            }
                            ret = ap_sock_connect(apinfo.sockfd, (char *)apinfo.url, apinfo.port);
                            if (ret == 0)
                            {
                                log_i(LOG_AUTO, "socket(%d) is connected successful",apinfo.sockfd);
                                apinfo.connect_st = 1;
                            }
                            else
                            {
                                log_i(LOG_AUTO, "socket(%d) is connecting", apinfo.sockfd);
                            }
                        }
                        tm_start(ap_check_timer, AP_CHECK_INTERVAL, TIMER_TIMEOUT_REL_PERIOD);
                        break;
                    }
                    case AP_MSG_NET_DISCONNECT:
                    {
                        apinfo.network = 0;
                        log_e(LOG_AUTO, "network is disconnect");
                        close(apinfo.sockfd);
                        apinfo.sockfd    = -1;
                        apinfo.connect_st =  0;
                        break;
                    }

                    case PM_MSG_RUNNING:
                    {
                        apinfo.waitcnt = 0;
                        
                        ap_allow_sleep = 0;
                        if(0 == apinfo.network)
                        {
                            break;
                        }
                        
                        if (apinfo.sockfd < 0)
                        {
                            apinfo.sockfd = ap_sock_init();
                            if (apinfo.sockfd < 0)
                            {
                                log_e(LOG_AUTO, "create sock failed");
                                break;
                            }
                        }
                        /* unconnected */
                        if (apinfo.connect_st == 0)
                        {
                            ret = ap_sock_connect(apinfo.sockfd, (char *)apinfo.url, apinfo.port);
                            if (ret == 0)
                            {
                                log_i(LOG_AUTO, "socket(%d) is connected successful",apinfo.sockfd);
                                apinfo.connect_st = 1;
                            }
                            else
                            {
                                log_i(LOG_AUTO, "socket(%d) is connecting",apinfo.sockfd);
                            }
                        }
                        break;
                    }
                    case PM_MSG_OFF:
                    case PM_MSG_SLEEP:
                    case PM_MSG_EMERGENCY:
                        close(apinfo.sockfd);
                        apinfo.sockfd    = -1;
                        apinfo.connect_st =  0;
                        apinfo.waitcnt = 0;
                        ap_allow_sleep = 1;
                        break;
                        
                    case AP_MSG_TIMER_EVENT:
                    {
                        if(ecu_selfupgrade_readversion == 1)
                        {
                           ecu_selfupgrade_readversion = 0;
                           foton_update_ecu_info();   
                           printf("wangxw\r\n");
                           apinfo.waitcnt = 0;
                        }
                        if(ecu_upgrade_finish_report_ecu_info_flag == 1)
                        {
                          ecu_upgrade_finish_report_ecu_info_flag = 0;
						  apinfo.waitcnt = 0;

                        }                       
                        if (apinfo.sockfd < 0)
                        {
                            apinfo.sockfd = ap_sock_init();
                            if (apinfo.sockfd < 0)
                            {
                                log_e(LOG_AUTO, "create sock failed");
                                break;
                            }
                        }
                        /* unconnected */
                        if (apinfo.connect_st == 0)
                        {
                            ret = ap_sock_connect(apinfo.sockfd, (char *)apinfo.url, apinfo.port);
                            if (ret == 0)
                            {
                                log_i(LOG_AUTO, "socket(%d) is connected successful",apinfo.sockfd);
                                apinfo.connect_st = 1;
                            }
                            else
                            {
                                log_i(LOG_AUTO, "socket(%d) is connecting",apinfo.sockfd);
                            }
                        }

                        /* connected */
                        if (apinfo.connect_st)
                        {
                            ret = ap_do_report_hb(apinfo.sockfd);
                            if (ret != 0)
                            {
                                apinfo.connect_st = 0;
                                log_e(LOG_AUTO, "socket send hb error, error:%s,ret:%d", strerror(errno), ret);
                            }
                            selfupgrade_mpu_only_report();
                            if(apinfo.waitcnt < 3)
                            {
                               // printf("wangxinwang\r\n");
                                ret = ap_do_report_ecu_info(apinfo.sockfd);
                                if (ret != 0)
                                {
                                    apinfo.connect_st = 0;
                                    log_e(LOG_AUTO, "socket send ecu info error, error:%s,ret:%d", strerror(errno), ret);
                                }
                            }
                        }
                        break;
                        
                    }
                }

                continue;
            }
            else if (FD_ISSET(apinfo.sockfd, &fds))
            {
                if (apinfo.connect_st)
                {
                    ret = recv(apinfo.sockfd, recv_buf + recv_len, sizeof(recv_buf) - recv_len, MSG_DONTWAIT);
                    if (ret == 0)
                    {
                        log_e(LOG_AUTO, "socket(AUTO) is close");
                        close(apinfo.sockfd);
                        apinfo.sockfd    = -1;
                        apinfo.connect_st =  0;
                    }
                    else if (ret < 0 && (errno == EINTR || errno == EAGAIN))
                    {
                        log_e(LOG_AUTO, "socket(AUTO) is recv error:%s", strerror(errno));
                        close(apinfo.sockfd);
                        apinfo.sockfd   = -1;
                        apinfo.connect_st =  0;
                    }
                    else
                    {
                        recv_len += ret;
                        protocol_dump(LOG_AUTO, "recv:", recv_buf, ret, 0);
                        ap_recv_proc(&apinfo, recv_buf, &recv_len);
                    }
                }

#if 0
                int error = 0;
                socklen_t errlen = sizeof(error);

                if (getsockopt(ap_sock, SOL_SOCKET, SO_ERROR, &error, &errlen) < 0)
                {
                    log_e(LOG_SOCK, "socket(AUTO) getsockopt fail: %s", strerror(errno));
                    close(ap_sock);
                    ap_sock    = -1;
                    ap_conn_st =  0;
                }

                if (0 != error)
                {
                    close(ap_sock);
                    ap_sock    = -1;
                    ap_conn_st =  0;
                    log_e(LOG_SOCK, "socket(AUTO) connect fail: %s", strerror(error));
                }
                else
                {
                    log_e(LOG_SOCK, "socket(AUTO) is connected");
                    ap_conn_st =  1;
                }
#endif
                continue;
            }

        }
        else if (0 == ret)  /* timeout */
        {
            continue; /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            break; /* thread exit abnormally */
        }

    }

    return NULL;
}

/****************************************************************
 function:     ap_run
 description:  startup GPS module
 input:        none
 output:       none
 return:       positive value indicates success;
 -1 indicates failed
 *****************************************************************/
int ap_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&ap_tid, &ta, (void *) ap_main, NULL);

    if (ret != 0)
    {
        return -1;
    }

    return 0;
}



