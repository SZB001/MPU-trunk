/****************************************************************
file:         wsrv_http.c
description:  the source file of web server http implementation
date:         2018/08/08
author        chenyin
****************************************************************/

#include <unistd.h>
#include <string.h>
#include <time.h>
#include "log.h"
#include "wsrv_api.h"
#include "wsrv_http.h"
#include "tcom_api.h"
#include "timer.h"
#include "dev_api.h"
#include "cfg_api.h"
//#include "geely_api.h"
#include "scom_msg_def.h"
#include "scom_tl.h"
#include "gps_api.h"
#include "PP_canSend.h"

extern timer_t restart_da_timer;

#define URI_LENGTH                  128
#define TIME_BUFFER_SIZE            32

/* gmobi request commands */
#define WSRV_CMD_GETVERSION         "getinfo"
#define WSRV_CMD_GETGPS             "getlocation"
#define WSRV_CMD_GETVIN             "getvin"
//#define WRSV_CMD_PREPARE            "prepare"
#define WSRV_CMD_ECUUPGRADE         "ecuupgrade"
#define WSRV_CMD_ECURESULT          "ecuresult"
#define WSRV_CMD_RESETWD            "resetwd"
#define WSRV_CMD_ALARM              "alarm"
#define WSRV_CMD_RTCTIME            "rtctime"
#define WSRV_CMD_MODEIN             "modein"
#define WSRV_CMD_MODEINRESULT       "modeinresult"
#define WSRV_CMD_MODEOUT            "modeout"


/* fill response body */
#define WSRV_VERSION_BODY           "{\"%s_sv\":\"%s\",\"%s_hv\":\"%s\",\"%s_sn\":\"%s\"}"
#define WSRV_GPS_BODY               "{\"la\":%lf,\"lo\":%lf}"
#define WSRV_VIN_BODY               "{\"vin\":\"%s\"}"
#define WSRV_ECURESULT_BODY         "{\"name\":\"%s\",\"result\":\"%s\"}"
#define WSRV_RTC_BODY               "{\"t\":%04d%02d%02dT%02d%02d%02dZ}"
#define WSRV_MODEINRESULT_BODY      "{\"r\":%d}"
#define WSRV_MODEOUTRESULT_BODY     "{\"r\":%d}"


/* web server http */
typedef enum
{
    HTTP_CODE_FILE_OK                   = 200,
    HTTP_CODE_FILE_FORBIDEN             = 403,
    HTTP_CODE_FILE_NOT_FOUND            = 404,
    HTTP_CODE_UNALLOW_METHOD            = 405,
    HTTP_CODE_FILE_TOO_LARGE            = 413,
    HTTP_CODE_URI_TOO_LONG              = 414,
    HTTP_CODE_UNSUPPORT_MIME_TYPE       = 415,
    HTTP_CODE_SERVER_INTERNAL_ERROR     = 500,
    HTTP_CODE_UNSUPPORT_HTTP_VERSION    = 505,
} HTTP_CODE;

/* http error response body */
#define HEADER_LENGTH   256
#define MIME_JSON       "application/json"
#define RSP_200_MSG     "HTTP/1.1 200 OK\r\nServer: tbox_webserver\r\nAllow: GET\r\nDate: %s\r\nContent-type: %s\r\nContent-Length: %d\r\n\r\n%s"

#define RSP_404_HEADER  "HTTP/1.1 404 File Not Found\r\nServer: tbox_webserver/Linux\r\nDate: %sContent-Type: text/html\r\nContent-Length: %d\r\n\r\n"
#define RSP_404_HTML    "<html><head></head><body>404 File Not Found<br/>please check your url,and try it again!</body></html>\r\n"
#define RSP_405_HEADER  "HTTP/1.1 405 Method Not Allow\r\nServer: tbox_webserver/Linux\r\nDate: %sContent-Type: text/html\r\nContent-Length: %d\r\n\r\n"
#define RSP_405_HTML    "<html><head></head><body>405 Method Not Allow<br/>please check your url,and try it again!</body></html>\n"
#define RSP_403_HEADER  "HTTP/1.1 403 File Forbiden\r\nServer: tbox_webserver/Linux\r\nDate: %sContent_Type: text/html\r\nContent-Length: %d\r\n\r\n"
#define RSP_403_HTML    "<html><head></head><body>403 File Forbiden<br/>maybe Jurisdiction is not right!</body></html>\r\n"
#define RSP_500_HEADER  "HTTP/1.1 500 Server Internal Error\r\nServer: tbox_webserver/Linux\r\nDate: %sContent-Type: text/html\r\nContent-Length: %d\r\n\r\n"
#define RSP_500_HTML    "<html><head></head><body>500 Server Internal Error<br/>please check your network!</body></html>\r\n"

extern int fota_ecu_get_ver(unsigned char *name, char *s_ver, int *s_siz, char *h_ver, int *h_siz, char *sn, int *sn_siz);

static int wsrv_socket_recv(int fd, unsigned char *buf, int len)
{
    int res, rdl = 0;

    while (rdl < len)
    {
        res = recv(fd, buf + rdl, len - rdl, MSG_DONTWAIT);

        if (res == 0) // tcp disconnect
        {
            return rdl ? rdl : -1;
        }
        else if (res > 0)
        {
            rdl += res;
        }
        else
        {
            if (errno == EINTR || errno == EAGAIN)
            {
                return rdl;
            }

            return rdl ? rdl : -1;
        }
    }

    return 0;
}

static int wsrv_socket_send(int cli_fd, char *rsp_buf, int len)
{
    int res, wrl = 0;

    while (len - wrl > 0)
    {
        res = send(cli_fd, rsp_buf + wrl, len - wrl, MSG_DONTWAIT);

        if (res <= 0)
        {
            if (errno != EINTR && errno != EAGAIN)
            {
                return -1;
            }
        }
        else
        {
            wrl += res;
        }
    }

    if (wrl != len)
    {
        return -1;
    }

    return len;
}

char *get_time_str(char *time_buf)
{
    time_t now_sec;
    struct tm *time_now;
    char *ptr = NULL;

    if (time(&now_sec) == -1)
    {
        log_e(LOG_WSRV, "get time error");

        return NULL;
    }

    time_now = gmtime(&now_sec);

    if (time_now == NULL)
    {
        log_e(LOG_WSRV, "localtime get error");

        return NULL;
    }

    ptr = asctime(time_now);

    if (ptr == NULL)
    {
        log_e(LOG_WSRV, "asctime get error");

        return NULL;
    }

    strcpy(time_buf, ptr);

    return time_buf;
}

static void set_error_information(char *rsp_buf, HTTP_CODE err_code) // TODO
{
    char time_buf[32] = {0};
    char head_buf[HEADER_LENGTH] = {0};

    if (!get_time_str(time_buf))
    {
        log_e(LOG_WSRV, "get server time failed!!!");
    }

    switch (err_code)
    {
        case HTTP_CODE_FILE_NOT_FOUND:
            sprintf(head_buf, RSP_404_HEADER, time_buf, strlen(RSP_404_HTML));
            sprintf(rsp_buf, "%s%s", head_buf, RSP_404_HTML);

            break;

        case HTTP_CODE_UNALLOW_METHOD:
            sprintf(head_buf, RSP_405_HEADER, time_buf, strlen(RSP_405_HTML));
            sprintf(rsp_buf, "%s%s", head_buf, RSP_405_HTML);

            break;

        case HTTP_CODE_FILE_FORBIDEN:
            sprintf(head_buf, RSP_403_HEADER, time_buf, strlen(RSP_403_HTML));
            sprintf(rsp_buf, "%s%s", head_buf, RSP_403_HTML);

            break;

        case HTTP_CODE_SERVER_INTERNAL_ERROR:
            sprintf(head_buf, RSP_500_HEADER, time_buf, strlen(RSP_500_HTML));
            sprintf(rsp_buf, "%s%s", head_buf, RSP_500_HTML);

            break;

        default:
            break;
    }

    log_i(LOG_WSRV, "response: %s", rsp_buf);
}

static void set_normal_information(char *rsp_buf, char *body_buf, char *mime_type)
{
    char time_buf[32] = {0};

    if (!get_time_str(time_buf))
    {
        log_e(LOG_WSRV, "get server time failed!!!");
    }

    if (!body_buf)
    {
        sprintf(rsp_buf, RSP_200_MSG, time_buf, mime_type, 0, "");
    }
    else
    {
        sprintf(rsp_buf, RSP_200_MSG, time_buf, mime_type, strlen(body_buf), body_buf);
    }

    log_i(LOG_WSRV, "response: %s", rsp_buf);
}

// TODO
static void deal_after_send()
{
    // send MSG to other module
}

unsigned int wsrv_calc_wake_time(RTCTIME abstime, 
                                      unsigned int year,
                                      unsigned int mon,
                                      unsigned int day,
                                      unsigned int hour,
                                      unsigned int min)
{
    unsigned int time_wake = 0;
    unsigned int abstimetick = 0;
    unsigned int settimetick = 0;

    settimetick = hour * 60 + min;
    abstimetick = abstime.hour * 60 + min;

    if(settimetick >= abstimetick)
    {
        time_wake = settimetick - abstimetick;
    }
    else
    {
        time_wake = 24 * 60 + settimetick - abstimetick;
    }

    log_i(LOG_WSRV, "OTA Set Alarm Time : %d min", time_wake);

    return time_wake;
}

static int process_cmd(int *p_cli_fd, char *cmd_buf, char *args_buf, char *data_buf)
{
    char tmp_buf[WSRV_MAX_BUFF_SIZE] = {0};
    char rsp_buf[WSRV_MAX_BUFF_SIZE] = {0};
    char body_buf[WSRV_MAX_BUFF_SIZE] = {0};
    char dev_buf[8] = {0};
    int ret = 0;

    // TODO
    unsigned char fw_ver[64] = {0};
    char mpu_ver[64] = {0};
    unsigned char mcu_run_ver[64] = {0};
    unsigned char mcu_upg_ver[64] = {0};
    unsigned char gl_vin[18] = {0};
    //unsigned int t_sn; // tbox sn
    unsigned char file_path[64] = {0};

    unsigned int cfg_len;
    char s_ver[64] = {0};
    char h_ver[64] = {0};
    char sn[64] = {0};
    unsigned int tbox_sn = 0;
    int s_len;
    int h_len;
    int sn_len;
    unsigned int timer_wake;
    RTCTIME abstime;

    log_i(LOG_WSRV, "fd: %d, cmd: %s, args: %s, data: %s", *p_cli_fd, cmd_buf, args_buf, data_buf);

    // TODO
    if (0 == strcmp(cmd_buf, WSRV_CMD_GETVERSION))
    {
        sscanf(args_buf, "dev=%s", dev_buf);

        if (0 == strcmp(dev_buf, "tbox"))
        {
            ret = upg_get_fw_ver(fw_ver, sizeof(fw_ver));
            strcpy(mpu_ver, dev_get_version());
            ret |= upg_get_mcu_run_ver(mcu_run_ver, sizeof(mcu_run_ver));
            cfg_get_para(CFG_ITEM_SN_NUM, (unsigned char *)&tbox_sn, &sn_len);

            if (0 == ret)
            {
                // TODO: make the rules that return the ver info
                memcpy(tmp_buf, mpu_ver, 23);
                memcpy(tmp_buf + 23, mcu_run_ver, 23);
                sprintf(body_buf, "{\"%s_sv\":\"%s\",\"%s_hv\":\"%s\",\"%s_sn\":\"%d\"}", 
                                    dev_buf, tmp_buf, dev_buf, fw_ver, dev_buf, tbox_sn);
            }
        }
        else if (0 == strcmp(dev_buf, "vcu"))
        {
            fota_ecu_get_ver((unsigned char *)"vcu", s_ver, &s_len, h_ver, &h_len, sn, &sn_len);
            sprintf(body_buf, WSRV_VERSION_BODY, dev_buf, s_ver, dev_buf,  h_ver, dev_buf, sn);
        }
        else if (0 == strcmp(dev_buf, "gw"))
        {
            fota_ecu_get_ver((unsigned char *)"gw", s_ver, &s_len, h_ver, &h_len, sn, &sn_len);
            sprintf(body_buf, WSRV_VERSION_BODY, dev_buf, s_ver, dev_buf,  h_ver, dev_buf, sn);
        }
        else if (0 == strcmp(dev_buf, "mfcp"))
        {
            fota_ecu_get_ver((unsigned char *)"mfcp", s_ver, &s_len, h_ver, &h_len, sn, &sn_len);
            sprintf(body_buf, WSRV_VERSION_BODY, dev_buf, s_ver, dev_buf,  h_ver, dev_buf, sn);
        }
        else if (0 == strcmp(dev_buf, "dvr"))
        {
            fota_ecu_get_ver((unsigned char *)"dvr", s_ver, &s_len, h_ver, &h_len, sn, &sn_len);
            sprintf(body_buf, WSRV_VERSION_BODY, dev_buf, s_ver, dev_buf,  h_ver, dev_buf, sn);
        }
        else
        {
            ret = -1;
        }

        if (ret < 0)
        {
            set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
        }
        else
        {
            set_normal_information(rsp_buf, body_buf, MIME_JSON);
        }

    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_GETGPS))
    {
        GPS_DATA snap;
        
        gps_get_snap(&snap);
        
        sprintf(body_buf, WSRV_GPS_BODY, snap.latitude, snap.longitude);

        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_GETVIN))
    {
        cfg_len = sizeof(gl_vin);

        if (0 != cfg_get_para(CFG_ITEM_GB32960_VIN, gl_vin, &cfg_len))
        {
            ret = -1;
        }

        if (ret < 0)
        {
            set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
        }
        else
        {
            sprintf(body_buf, WSRV_VIN_BODY, gl_vin);

            set_normal_information(rsp_buf, body_buf, MIME_JSON);
        }
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_ECUUPGRADE))
    {
        extern int fota_upgrade(unsigned char *file_path);
        // TODO: analyse XML file and upgrade ecu
        log_e(LOG_WSRV, " ######### cmd:%s arg:%s data:%s", cmd_buf, args_buf, data_buf);

        sscanf(data_buf, "{\"config\":\"%s", file_path);

        if (0 != file_path[0])
        {
            file_path[strlen(file_path) - 2] = '/';
            file_path[strlen(file_path) - 1] = 0;
            log_e(LOG_WSRV, " ######### file_path:%s", file_path);

            //call upgrade function
            fota_upgrade(file_path);
        }

        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_ECURESULT))
    {
        int upgraderesult;
        extern int get_upgrade_result(unsigned char *name);

        upgraderesult = get_upgrade_result((unsigned char *)tmp_buf);
        log_e(LOG_WSRV, "********** rcv get resault request, %d", upgraderesult);

        // TODO: fake data
        if (0 == upgraderesult)
        {
            sprintf(body_buf, WSRV_ECURESULT_BODY, tmp_buf, "success");
        }
        else if (2 == upgraderesult)
        {
            sprintf(body_buf, WSRV_ECURESULT_BODY, tmp_buf, "upgrading");
        }
        else if (-1 == upgraderesult)
        {
            sprintf(body_buf, WSRV_ECURESULT_BODY, tmp_buf, "fail");
        }

        if (ret < 0)
        {
            set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
        }
        else
        {
            set_normal_information(rsp_buf, body_buf, MIME_JSON);
        }
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_RESETWD))
    {
        tm_start(restart_da_timer, WSRV_NO_ACK_TIMEOUT, TIMER_TIMEOUT_REL_ONCE);

        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_ALARM))
    {
        unsigned int year = 0;
        unsigned int mon = 0;
        unsigned int mday = 0;
        unsigned int hour = 0;
        unsigned int min = 0;
        unsigned int sec = 0;

        if(6 == sscanf(args_buf, "timestamp=%04d%02d%02dT%02d%02d%02dZ", &year, &mon, &mday, &hour, &min, &sec))
        {
            log_e(LOG_WSRV, "OTA Set Alarm Time, %04d%02d%02dT%02d%02d%02dZ", year, mon, mday, hour, min, sec);
            ret = tm_get_abstime(&abstime);

            timer_wake = wsrv_calc_wake_time(abstime, year, mon, mday, hour, min);

            if(0 != timer_wake)
            {
                ret = scom_tl_send_frame(SCOM_TL_CMD_WAKE_TIME, SCOM_TL_SINGLE_FRAME, 0,
                                         (unsigned char *)&timer_wake, sizeof(timer_wake));
                if (0 != ret)
                {
                    log_e(LOG_PM, "set timer wake failed");
                }
            }
            else
            {
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }

        if (ret < 0)
        {
            set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
        }
        else
        {
            set_normal_information(rsp_buf, body_buf, MIME_JSON);
        }
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_RTCTIME))
    {
        ret = tm_get_abstime(&abstime);
    
        sprintf(body_buf, WSRV_RTC_BODY, abstime.year,
                                         abstime.mon,
                                         abstime.mday,
                                         abstime.hour,
                                         abstime.min,
                                         abstime.sec);

        if (ret < 0)
        {
            set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
        }
        else
        {
            set_normal_information(rsp_buf, body_buf, MIME_JSON);
        }
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_MODEIN))
    {
        PP_can_send_data(PP_CAN_OTAREQ, 0x02, NULL);
        
        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_MODEINRESULT))
    {
        sprintf(body_buf, WSRV_MODEINRESULT_BODY, 1);

        //Start Check BDM_PowerMode And BDM_TBOX_OTAModeFailSts
        //If BDM_PowerMode = 0x2=ON And BDM_TBOX_OTAModeFailSts 0x0 = No failure
        //And No Time Out, Means OTA Mode In OK
    
        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else if (0 == strcmp(cmd_buf, WSRV_CMD_MODEOUT))
    {
        PP_can_send_data(PP_CAN_OTAREQ, 0x01, NULL);
    
        sprintf(body_buf, WSRV_MODEOUTRESULT_BODY, 1);
    
        set_normal_information(rsp_buf, body_buf, MIME_JSON);
    }
    else
    {
        log_e(LOG_WSRV, "unknow command!!!");
        set_error_information(rsp_buf, HTTP_CODE_SERVER_INTERNAL_ERROR);
    }

    ret = wsrv_socket_send(*p_cli_fd, rsp_buf, strlen(rsp_buf));

    if (ret < 0)
    {
        log_e(LOG_WSRV, "socket send error!!!");
        ret = -1; // TODO
    }

    // after send data success, clost this socket immediately
    // because gmobi DA will create a new socket to send command
    shutdown(*p_cli_fd, SHUT_RDWR);
    close(*p_cli_fd);
    *p_cli_fd = -1;

    deal_after_send();

    return 0;
}

static int process_client_request(int *p_cli_fd, const char *req_buf, unsigned int *req_len)
{
    /*
     * eg: POST /upgrade?dev=tbox HTTP1.1\r\n
     * method_buf:  POST
     * uri_buf:     upgrade?dev=tbox
     * cmd_buf:     upgrade
     * arg_buf:     dev=tbox
     * dev_buf:     tbox
     * ver_buf:     HTTP1.1
     * data_buf:    '{config:/data/upgradefiles/cfg.xml}'
     */
    char method_buf[8] = {0};
    char uri_buf[URI_LENGTH] = {0};
    char cmd_buf[URI_LENGTH] = {0};
    char args_buf[URI_LENGTH] = {0};
    char ver_buf[16] = {0};
    char data_buf[WSRV_MAX_BUFF_SIZE] = {0};
    char *ptr;
    char *post_body;
    unsigned int post_body_len = 0;

    sscanf(req_buf, "%s /%s %[^\r\n]", method_buf, uri_buf, ver_buf);
    sscanf(uri_buf, "%[^\?]\?%s", cmd_buf, args_buf);

    ptr = strstr(req_buf, "\r\n\r\n");
    strcpy(data_buf, ptr + 4);

    log_i(LOG_WSRV, "::%s\n::%s\n::%s\n::%s\n::%s\n::%s",
          method_buf, uri_buf, ver_buf,
          cmd_buf, args_buf, data_buf); // useful data

    /* check client request protocol */
    if (strncmp(ver_buf, "HTTP", 4) != 0)
    {
        log_e(LOG_WSRV, "not http protocol!!!");

        return -1;
    }

    if (memcmp(method_buf, "POST", 4) == 0)
    {
        post_body = strstr(req_buf, "Content-Length: ");

        if (NULL == post_body)
        {
            log_i(LOG_WSRV, "No Content-Length:  string");
        }
        else
        {
            //Get Post Method Body Len
            //Ex: Content-Length: 30
            //    post_body_len = 3 * 10 + 0;
            post_body_len = (*(post_body + 16) - '0') * 10 + (*(post_body + 17) - '0');

            if (post_body_len != strlen(data_buf))
            {
                *req_len = strlen(req_buf);

                log_i(LOG_WSRV, "post_body_len = %d", post_body_len);
                log_i(LOG_WSRV, "data_buf_len = %d", strlen(data_buf));
                log_i(LOG_WSRV, "*req_len = %d", *req_len);
                log_i(LOG_WSRV, "wait post body");

                return 0;
            }
            else
            {
                *req_len = 0;
                log_i(LOG_WSRV, "Set req_len 0");
            }
        }
    }

    /* process request cmd */
    if (process_cmd(p_cli_fd, cmd_buf, args_buf, data_buf) < 0)
    {
        log_e(LOG_WSRV, "process cmd failed!!!");

        return -1;
    }

    return 0;
}

int wsrv_http_handle(WSRV_CLIENT *cli)
{
    int ret;

    /* recv request data from client socket fd */
    if ((ret = wsrv_socket_recv(cli->fd, cli->req_buf + cli->req_len, sizeof(cli->req_buf))) < 0)
    {
        log_e(LOG_WSRV, "socket recv error!!!");

        return -1; // TODO
    }

    log_i(LOG_WSRV, "client request: %s", cli->req_buf);

    /* handle client request */
    if (process_client_request(&cli->fd, (const char *)cli->req_buf, &cli->req_len) < 0)
    {
        log_e(LOG_WSRV, "client request unknow!!!");

        return -1; // TODO
    }

    return 0;
}

