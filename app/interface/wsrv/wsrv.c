/****************************************************************
file:         wsrv.c
description:  the source file of tbox web server implementation
date:         2018/08/08
author        chenyin
****************************************************************/
#include "com_app_def.h"
#include "wsrv_api.h"
#include "wsrv_http.h"
#include "tcom_api.h"
#include "timer.h"
#include "PP_canSend.h"
#include "dev_api.h"
#include "cfg_api.h"
#include "scom_msg_def.h"
#include "scom_tl.h"
#include "../../base/dev/dev_mcu_cfg.h"
#include "pm_api.h"
#include "../hozon/PrvtProtocol/PrvtProt.h"
#include "../hozon/PrvtProtocol/PrvtProt_lock.h"

static pthread_t wsrv_tid;
static unsigned char msgbuf[TCOM_MAX_MSG_LEN];
static timer_t
recreate_timer; // if create_webserver_socket() faild, after WSRV_RECREATE_INTERVAL(5S) recall create_webserver_socket()
timer_t restart_da_timer;
static int sock_tcp_fd = -1;
static WSRV_CLIENT clients[WSRV_MAX_CLIENT_NUM];
//static pthread_mutex_t wsrv_mutex;

static int set_socket_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0)
    {
        log_e(LOG_WSRV, "Fail to fcntl, error:%s", strerror(errno));
        return -1; // TODO
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        log_e(LOG_WSRV, "Fail to fcntl, error:%s", strerror(errno));
        return -1; // TODO
    }

    return 0;
}

static int create_webserver_socket(void)
{
    int i = 0;
    int opt = 1;
    struct sockaddr_in srv_addr;

    for (i = 0; i < WSRV_MAX_CLIENT_NUM; i++)
    {
        clients[i].fd = -1;
    }

    bzero(&srv_addr, sizeof(srv_addr));

    sock_tcp_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_tcp_fd < 0)
    {
        log_e(LOG_WSRV, "Fail to socket, error:%s", strerror(errno));
        return -1; // TODO
    }

    bzero(&srv_addr, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(WEB_SERVER_PORT);
    srv_addr.sin_addr.s_addr = INADDR_ANY;

    if (setsockopt(sock_tcp_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof(opt)) < 0)
    {
        log_e(LOG_WSRV, "Fail to setsockop, error:%s", strerror(errno));
        return -1; // TODO
    }

    if (set_socket_nonblock(sock_tcp_fd) < 0)
    {
        log_e(LOG_WSRV, "Fail to fcntl, error:%s", strerror(errno));
        return -1; // TODO
    }

    if (bind(sock_tcp_fd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0)
    {
        log_e(LOG_WSRV, "Fail to bind, error:%s", strerror(errno));
        return -1; // TODO
    }

    if (listen(sock_tcp_fd, 1) < 0)
    {
        log_e(LOG_WSRV, "Fail to listen, error:%s", strerror(errno));
        return -1; // TODO
    }

    log_i(LOG_WSRV, "wsrv module create server socket success");

    return 0;
}

static int wsrv_timeout(unsigned int timer_msgid)
{
    int ret = 0;

    if (timer_msgid == TIMER_WSRV_RECREATE)
    {
        log_e(LOG_WSRV, "web server recreate server socket");
        ret = create_webserver_socket();

        if (ret != 0)
        {
            if (sock_tcp_fd > 0)
            {
                close(sock_tcp_fd);
                sock_tcp_fd = -1;
            }

            tm_start(recreate_timer, WSRV_RECREATE_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
        }
    }
    else if (timer_msgid == TIMER_WSRV_RESTART_DA)
    {
        log_e(LOG_WSRV, "gmobi da app not feeddog last %dS, restart it!", WSRV_NO_ACK_TIMEOUT);

        system("killall -9 otamaster");
        //system("otamaster");

        tm_start(restart_da_timer, WSRV_NO_ACK_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);
    }

    return ret;
}

int wrsv_mode_in(int argc, const char **argv)
{
    unsigned char mode;

    //(0:runing 1:listen 2:sleep 3:auto)
    mode = 0;
    
    dev_set_from_mpu(MCU_CFG_ID_SYSMODE, &mode, sizeof(mode));
    
    PP_can_send_data(PP_CAN_OTAREQ, 0x02, 0);

    return 0;
}

int wrsv_mode_in_result(int argc, const char **argv)
{

    return 0;

}

int wrsv_mode_out(int argc, const char **argv)
{
    unsigned char mode;

    PP_can_send_data(PP_CAN_OTAREQ, 0x01, 0);
    
    //(0:runing 1:listen 2:sleep 3:auto)
    mode = 3;
    
    dev_set_from_mpu(MCU_CFG_ID_SYSMODE, &mode, sizeof(mode));
    
    return 0;
}

/****************************************************************
 * function:     wrsv_sleep_available
 * description:  
 * input:        none
 * output:       none
 * return:       0 indicates can not enter sleep status;
 *               1 indicates can enter sleep status;
 *****************************************************************/
static int wrsv_sleep_available(PM_EVT_ID id)
{
    return 1;
}

static int wrsv_set_ota_mode(int argc, const char **argv)
{  
    unsigned int otamodein = 0;

    if (argc != 1)
    {
        shellprintf(" usage:set ota mode en error\r\n");
        return -1;
    }

    sscanf(argv[0], "%u", &otamodein);
    cfg_set_para(CFG_ITEM_EN_OTAMODEIN, (unsigned char *)&otamodein, 1);
    shellprintf(" set ota mode ok\r\n");
    sleep(1);
    //system("reboot");
    return 0;
}

int wsrv_init(INIT_PHASE phase)
{
    int ret = 0;
    int i = 0;
    unsigned int len;
    unsigned char otamodein;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            for (i = 0; i < WSRV_MAX_CLIENT_NUM; i++)
            {
                clients[i].fd = -1;
            }

            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= pm_reg_handler(MPU_MID_WSRV, wrsv_sleep_available);
            //ret |= shell_cmd_register("modein", wrsv_mode_in, "OTA Mode In");
            //ret |= shell_cmd_register("moderesult", wrsv_mode_in_result, "OTA Mode In Resullt");
            //ret |= shell_cmd_register("modeout", wrsv_mode_out, "OTA Mode Out");

            ret |= shell_cmd_register("setotamode", wrsv_set_ota_mode, "Set Ota Mode");

            
            ret = tm_create(TIMER_REL, TIMER_WSRV_RECREATE, MPU_MID_WSRV, &recreate_timer);

            if (ret != 0)
            {
                log_e(LOG_WSRV, "create timer TIMER_WSRV_RECREATE failed ret=0x%08x", ret);
                return ret;
            }

            ret = tm_create(TIMER_REL, TIMER_WSRV_RESTART_DA, MPU_MID_WSRV, &restart_da_timer);

            if (ret != 0)
            {
                log_e(LOG_WSRV, "create timer TIMER_WSRV_RESTART_DA failed ret=0x%08x", ret);
                return ret;
            }

            len = 1;
	        ret |= cfg_get_para(CFG_ITEM_EN_OTAMODEIN, &otamodein, &len);

            if(1 == otamodein)
            {
                if(PP_LOCK_OK == setPP_lock_odcmtxlock(PP_LOCK_OTA_FOTAUPDATE))
                {
                    log_i(LOG_WSRV, "When Init, Get OTA Mode In, So Get Lock");
                }
                else
                {
                    log_e(LOG_WSRV, "When Init, Get OTA Mode In, But Other Task Doing, So Can Not Get Lock");
                }
            }

            break;
    }

    return 0;
}

void *wsrv_main(void)
{
    int i = 0;
    int max_fd, msg_fd, ret;
    fd_set fds;
    TCOM_MSG_HEADER msgheader;

    int new_conn_fd = -1;
    struct sockaddr_in cli_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    prctl(PR_SET_NAME, "WSRV");

    ret = create_webserver_socket();

    if (ret != 0)
    {
        if (sock_tcp_fd > 0)
        {
            close(sock_tcp_fd);
            sock_tcp_fd = -1;
        }

        tm_start(recreate_timer, WSRV_RECREATE_INTERVAL, TIMER_TIMEOUT_REL_ONCE);
    }

    tm_start(restart_da_timer, WSRV_NO_ACK_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);

    msg_fd = tcom_get_read_fd(MPU_MID_WSRV);

    if (msg_fd  < 0)
    {
        log_e(LOG_WSRV, "tcom_get_read_fd failed");
        return NULL;
    }

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(msg_fd, &fds);

        if (sock_tcp_fd > 0)
        {
            FD_SET(sock_tcp_fd, &fds);
        }

        max_fd = msg_fd;

        if (sock_tcp_fd > max_fd)
        {
            max_fd = sock_tcp_fd;
        }

        for (i = 0; i < WSRV_MAX_CLIENT_NUM; i++)
        {
            if (clients[i].fd <= 0)
            {
                continue;
            }

            FD_SET(clients[i].fd, &fds);

            if (max_fd < clients[i].fd)
            {
                max_fd = clients[i].fd;
            }
        }

        /* monitor the incoming data */
        ret = select(max_fd + 1, &fds, NULL, NULL, NULL);

        /* the file deccriptor is readable */
        if (ret > 0)
        {

            /* thread message */
            if (FD_ISSET(msg_fd, &fds))
            {
                if (0 == tcom_recv_msg(MPU_MID_WSRV, &msgheader, msgbuf))
                {

                    if (MPU_MID_TIMER == msgheader.sender)
                    {
                        wsrv_timeout(msgheader.msgid);
                    }
                    else if (MPU_MID_MID_PWDG == msgheader.msgid)
                    {
                        pwdg_feed(MPU_MID_WSRV);
                    }
                    else if(PM_MSG_RUNNING == msgheader.msgid)
                    {
						log_i(LOG_WSRV, "kill otamaster");
                        system("killall -9 otamaster");
                    }
                    else if((PM_MSG_SLEEP == msgheader.msgid) ||
                            (PM_MSG_EMERGENCY == msgheader.msgid) ||
                            (PM_MSG_OFF == msgheader.msgid))
                    {
                        ;
                    }
                }
            }

            /* server accept */
            if (FD_ISSET(sock_tcp_fd, &fds))
            {
                bzero(&cli_addr, sizeof(cli_addr));
                new_conn_fd = accept(sock_tcp_fd, (struct sockaddr *)&cli_addr, &addrlen);
                log_i(LOG_WSRV, "new client comes ,fd=%d\n", new_conn_fd);

                if (new_conn_fd < 0)
                {
                    log_e(LOG_WSRV, "Fail to accept");
                    continue;
                }

                for (i = 0; i < WSRV_MAX_CLIENT_NUM; i++)
                {
                    if (clients[i].fd == -1)
                    {
                        set_socket_nonblock(new_conn_fd);

                        clients[i].fd = new_conn_fd;
                        memset(clients[i].req_buf, 0, WSRV_MAX_BUFF_SIZE);

                        log_i(LOG_WSRV, "add client_fd[%d]=%d", i, clients[i].fd);
                        break;
                    }
                }

                if (i >= WSRV_MAX_CLIENT_NUM)
                {
                    log_e(LOG_WSRV, "connect full!");
                    close(new_conn_fd);
                }
            }

            for (i = 0; i < WSRV_MAX_CLIENT_NUM; i++)
            {
                if (-1 == clients[i].fd)
                {
                    continue;
                }

                /* client request */
                if (FD_ISSET(clients[i].fd, &fds))
                {
                    log_i(LOG_WSRV, "start handle client %d request", i);

                    /* TODO: maybe it's better to create a thread? */
                    if (wsrv_http_handle(&clients[i]) < 0)
                    {
                        close(clients[i].fd);
                        clients[i].fd = -1;
                    }
                }
            }
        }
        else if (0 == ret)   /* timeout */
        {
            continue;   /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            log_e(LOG_WSRV, "wsrv_main exit, error:%s", strerror(errno));
            break;  /* thread exit abnormally */
        }
    }

    return NULL;
}

int wsrv_run(void)
{
    int ret;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    /* create thread and monitor the incoming data */
    ret = pthread_create(&wsrv_tid, &ta, (void *)wsrv_main, NULL);

    if (ret != 0)
    {
        log_e(LOG_WSRV, "pthread_create failed, error:%s", strerror(errno));
        return ret;
    }

    return 0;
}

