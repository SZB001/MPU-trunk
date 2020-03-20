#include "remote_diag.h"
#include "log.h"
#include "tcom_api.h"
#include "uds_define.h"
#include "uds.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "remote_diag_api.h"
#include "remote_diag.h"
#include "hozon_PP_api.h"
#include "../hozon/PrvtProtocol/remoteDiag/PrvtProt_rmtDiag.h"
#include "timer.h"

#define REMOTE_DIAG_SERVICE_NUM 6
#define REMOTE_DIAG_TO_SDK_MSG_LEN 1000
#define SDK_TO_REMOTE_DIAG_MSG_LEN 1000

extern UDS_T    uds_client;


const unsigned char dtc_to_str_arr[4] = {'P', 'C', 'B', 'U'};


typedef int (*PARSING_SINGLE_TIME)(const uint8_t *did_value, RTCTIME *time);

const unsigned char Snapshot_no[REMOTE_DIAG_ECU_NUM] = 
{
    0,//REMOTE_DIAG_ALL = 0,//
    
    0,//REMOTE_DIAG_VCU,//
    0,//REMOTE_DIAG_BMS,//
    1,//REMOTE_DIAG_MCUp,
    1,//REMOTE_DIAG_OBCp,
    1,//REMOTE_DIAG_FLR,
    
    1,//REMOTE_DIAG_FLC,
    1,//REMOTE_DIAG_APA,
    1,//REMOTE_DIAG_ESCPluse,
    1,//REMOTE_DIAG_EPS,
    1,//REMOTE_DIAG_EHB,
    
    0,//REMOTE_DIAG_BDCM,
    1,//REMOTE_DIAG_GW,
    1,//REMOTE_DIAG_LSA,
    1,//REMOTE_DIAG_CLM,
    1,//REMOTE_DIAG_PTC,
    
    1,//REMOTE_DIAG_EACP,
    1,//REMOTE_DIAG_EGSM,
    1,//REMOTE_DIAG_ALM,
    0,//REMOTE_DIAG_WPC,
    1,//REMOTE_DIAG_IHU,
    
    1,//REMOTE_DIAG_ICU,
    1,//REMOTE_DIAG_IRS,
    1,//REMOTE_DIAG_DVR,
    0,//REMOTE_DIAG_TAP,
    1,//REMOTE_DIAG_MFCP,
    
    1,//REMOTE_DIAG_TBOX,
    0,//REMOTE_DIAG_ACU,
    1,//REMOTE_DIAG_PLG,

};

int comm_parsing_time(const uint8_t *did_value, RTCTIME *time);
int vcu_parsing_time(const uint8_t *did_value, RTCTIME *time);
int ehb_parsing_time(const uint8_t *did_value, RTCTIME *time);
int LSB_parsing_time(const uint8_t *did_value, RTCTIME *time);

PARSING_SINGLE_TIME parsing_time_arr[REMOTE_DIAG_ECU_NUM] = 
{
    NULL,//REMOTE_DIAG_ALL = 0,//
    
    vcu_parsing_time,//REMOTE_DIAG_VCU,//
    NULL,//REMOTE_DIAG_BMS,//
    comm_parsing_time,//REMOTE_DIAG_MCUp,
    comm_parsing_time,//REMOTE_DIAG_OBCp,
    comm_parsing_time,//REMOTE_DIAG_FLR,
    
    comm_parsing_time,//REMOTE_DIAG_FLC,
    comm_parsing_time,//REMOTE_DIAG_APA,
    LSB_parsing_time,//REMOTE_DIAG_ESCPluse,
    comm_parsing_time,//REMOTE_DIAG_EPS,
    ehb_parsing_time,//REMOTE_DIAG_EHB,
    
    NULL,//REMOTE_DIAG_BDCM,
    comm_parsing_time,//REMOTE_DIAG_GW,
    comm_parsing_time,//REMOTE_DIAG_LSA,
    comm_parsing_time,//REMOTE_DIAG_CLM,
    comm_parsing_time,//REMOTE_DIAG_PTC,
    
    comm_parsing_time,//REMOTE_DIAG_EACP,
    comm_parsing_time,//REMOTE_DIAG_EGSM,
    comm_parsing_time,//REMOTE_DIAG_ALM,
    NULL,//REMOTE_DIAG_WPC,
    comm_parsing_time,//REMOTE_DIAG_IHU,
    
    comm_parsing_time,//REMOTE_DIAG_ICU,
    comm_parsing_time,//REMOTE_DIAG_IRS,
    comm_parsing_time,//REMOTE_DIAG_DVR,
    NULL,//REMOTE_DIAG_TAP,
    comm_parsing_time,//REMOTE_DIAG_MFCP,
    
    LSB_parsing_time,//REMOTE_DIAG_TBOX,
    NULL,//REMOTE_DIAG_ACU,
    comm_parsing_time,//REMOTE_DIAG_PLG,
};

const unsigned int time_pos[REMOTE_DIAG_ECU_NUM] = 
{
    0,//REMOTE_DIAG_ALL = 0,//
    
    4,//REMOTE_DIAG_VCU,//
    0,//REMOTE_DIAG_BMS,//
    39,//REMOTE_DIAG_MCUp,
    7,//REMOTE_DIAG_OBCp,
    15,//REMOTE_DIAG_FLR,
    
    15,//REMOTE_DIAG_FLC,
    15,//REMOTE_DIAG_APA,
    15,//REMOTE_DIAG_ESCPluse,
    15,//REMOTE_DIAG_EPS,
    15,//REMOTE_DIAG_EHB,
    
    0,//REMOTE_DIAG_BDCM,
    16,//REMOTE_DIAG_GW,
    15,//REMOTE_DIAG_LSA,
    15,//REMOTE_DIAG_CLM,
    15,//REMOTE_DIAG_PTC,
    
    15,//REMOTE_DIAG_EACP,
    15,//REMOTE_DIAG_EGSM,
    15,//REMOTE_DIAG_ALM,
    0,//REMOTE_DIAG_WPC,
    15,//REMOTE_DIAG_IHU,
    
    15,//REMOTE_DIAG_ICU,
    15,//REMOTE_DIAG_IRS,
    15,//REMOTE_DIAG_DVR,
    0,//REMOTE_DIAG_TAP,
    15,//REMOTE_DIAG_MFCP,
    
    15,//REMOTE_DIAG_TBOX,
    0,//REMOTE_DIAG_ACU,
    15,//REMOTE_DIAG_PLG,
};

const unsigned char remote_diag_server_cmd[REMOTE_DIAG_SERVICE_NUM][10] =
{

    {"190209\0"},
    {"14FFFFFF\0"},
    {"1904\0"},
    {"14FFFFFF\0"},
    {"22\0"},
    {"2E\0"},
};

/**/
const unsigned char remote_diag_service_session[REMOTE_DIAG_SERVICE_NUM] =
{
    SESSION_TYPE_DEFAULT,
    SESSION_TYPE_DEFAULT,
    SESSION_TYPE_DEFAULT,
    SESSION_TYPE_DEFAULT,
    SESSION_TYPE_DEFAULT,
    SESSION_TYPE_EXTENDED,
};

/*远程诊断各服务需解锁的安全等级*/
const unsigned char remote_diag_service_security_level[REMOTE_DIAG_SERVICE_NUM] =
{
    SecurityAccess_LEVEL0,
    SecurityAccess_LEVEL0,
    SecurityAccess_LEVEL0,
    SecurityAccess_LEVEL0,
    SecurityAccess_LEVEL0,
    SecurityAccess_LEVEL1,
};

const unsigned int REMOTE_DIAG_CAN_ID[REMOTE_DIAG_ECU_NUM][2] = 
{
    {0x000,0x000},//REMOTE_DIAG_ALL = 0,//
    
    {0x7E2,0x7EA},//REMOTE_DIAG_VCU,//
    {0x706,0x716},//REMOTE_DIAG_BMS,//
    {0x707,0x717},//REMOTE_DIAG_MCUp,
    {0x70A,0x71A},//REMOTE_DIAG_OBCp,
    {0x7C1,0x7D1},//REMOTE_DIAG_FLR,
    
    {0x7C2,0x7D2},//REMOTE_DIAG_FLC,
    {0x7C0,0x7D0},//REMOTE_DIAG_APA,
    {0x720,0x730},//REMOTE_DIAG_ESCPluse,
    {0x724,0x734},//REMOTE_DIAG_EPS,
    {0x722,0x732},//REMOTE_DIAG_EHB,
    
    {0x740,0x750},//REMOTE_DIAG_BDCM,
    {0x762,0x772},//REMOTE_DIAG_GW,
    {0x763,0x773},//REMOTE_DIAG_LSA,
    {0x74B,0x75B},//REMOTE_DIAG_CLM,
    {0x765,0x775},//REMOTE_DIAG_PTC,
    
    {0x74C,0x75C},//REMOTE_DIAG_EACP,
    {0x70B,0x71B},//REMOTE_DIAG_EGSM,
    {0x766,0x776},//REMOTE_DIAG_ALM,
    {0x786,0x796},//REMOTE_DIAG_WPC,
    {0x780,0x790},//REMOTE_DIAG_IHU,
    
    {0x781,0x791},//REMOTE_DIAG_ICU,
    {0x783,0x793},//REMOTE_DIAG_IRS,
    {0x787,0x797},//REMOTE_DIAG_DVR,
    {0x785,0x795},//REMOTE_DIAG_TAP,
    {0x782,0x792},//REMOTE_DIAG_MFCP,
    
    {0x7A0,0x7B0},//REMOTE_DIAG_TBOX,
    {0x746,0x756},//REMOTE_DIAG_ACU,
    {0x764,0x774},//REMOTE_DIAG_PLG,
};


static void str_to_dtc(unsigned char * dtc, unsigned char * dtc_str)
{
    StrToHex(dtc, &(dtc_str[1]), 4);
    uint8_t i = 0;
    for(i=0;i<4;i++)
    {
        if(dtc_str[0] == dtc_to_str_arr[i])
        {
            break;
        }
    }
    dtc[0] = dtc[0] + (i<<6);
    dtc[2] = dtc_str[5];
}


/* 将接收到的远程诊断消息，解析为远程诊断模块可处理的标准请求格式 */
int parsing_remote_diag_msg(char * remote_diag_msg, TCOM_MSG_HEADER msg, remote_diag_request_arr_t * remote_diag_request_arr)
{
    int res = 0;
    shellprintf("remote diag get msg:%s", remote_diag_msg);
    unsigned int mlen = msg.msglen;

    unsigned char request_msg[DIAG_REQUEST_LEN];
    memset(request_msg, 0x00, sizeof(request_msg));

    if(msg.sender == MPU_MID_SHELL)/* shell命令发送的诊断消息 */
    {
       StrToHex(request_msg, (unsigned char *)remote_diag_msg, mlen/2);
    }
    else
    {
       memcpy(request_msg, remote_diag_msg, msg.msglen);
    }

    uint8_t dtc[3] = {0};
    memset(remote_diag_request_arr, 0x00, sizeof(remote_diag_request_arr_t));

    char remote_diag_sid = request_msg[0];
    char ecutype = request_msg[1];

    if(ecutype < REMOTE_DIAG_ECU_NUM)/* 所有ECU*/
    {
       int ecutype_tmp = 0;
       unsigned char request_size = 0;
       
       for(ecutype_tmp=(ecutype==0?1:ecutype);ecutype_tmp<(ecutype==0?REMOTE_DIAG_ECU_NUM:ecutype + 1);ecutype_tmp++)
       {
           remote_diag_request_arr->remote_diag_request[request_size].baud = 500;
           remote_diag_request_arr->remote_diag_request[request_size].port = 1;
           remote_diag_request_arr->remote_diag_request[request_size].request_id = REMOTE_DIAG_CAN_ID[ecutype_tmp][DIAG_REQUEST_ID_ROW];
           remote_diag_request_arr->remote_diag_request[request_size].response_id = REMOTE_DIAG_CAN_ID[ecutype_tmp][DIAG_RESPONSE_ID_ROW];
           remote_diag_request_arr->remote_diag_request[request_size].level = SecurityAccess_LEVEL0;
           remote_diag_request_arr->remote_diag_request[request_size].session = SESSION_TYPE_DEFAULT;
           
           StrToHex(remote_diag_request_arr->remote_diag_request[request_size].diag_request, 
                   (unsigned char *)remote_diag_server_cmd[remote_diag_sid - 1], 
                   strlen((const char *)(remote_diag_server_cmd[remote_diag_sid - 1])) / 2);/* 根据状态掩码报告DTC */
                   
           remote_diag_request_arr->remote_diag_request[request_size].diag_request_len = 
               strlen((const char *)remote_diag_request_arr->remote_diag_request[request_size].diag_request);
           
           
           if(remote_diag_sid == 3)/* 读取DTC快照信息 */
           {
               unsigned char *diag_request = remote_diag_request_arr->remote_diag_request[request_size].diag_request;
               unsigned int *diag_request_len = &(remote_diag_request_arr->remote_diag_request[request_size].diag_request_len);
               
               str_to_dtc(dtc, &(request_msg[2]));
               log_buf_dump(LOG_REMOTE_DIAG, "request_msg", request_msg, 3);
               log_buf_dump(LOG_REMOTE_DIAG, "dtc", dtc, 3);
               memcpy(&(diag_request[*diag_request_len]), dtc, 3);
               *diag_request_len = *diag_request_len + 3;

               diag_request[*diag_request_len] = Snapshot_no[(unsigned char)ecutype];
               *diag_request_len = *diag_request_len + 1;
           }
           log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                        remote_diag_request_arr->remote_diag_request[request_size].baud,
                        remote_diag_request_arr->remote_diag_request[request_size].port,
                        remote_diag_request_arr->remote_diag_request[request_size].request_id,
                        remote_diag_request_arr->remote_diag_request[request_size].response_id,
                        remote_diag_request_arr->remote_diag_request[request_size].diag_request_len,
                        remote_diag_request_arr->remote_diag_request[request_size].level,
                        remote_diag_request_arr->remote_diag_request[request_size].session,
                        remote_diag_request_arr->remote_diag_request[request_size].diag_request
                       );

          request_size++;
                   
       }
       remote_diag_request_arr->remote_diag_request_size= request_size;
    }
    else
    {
       log_o(LOG_REMOTE_DIAG, "remote diag recv invalid msg锛?ecutype: %x,remote_diag_sid: %x", ecutype, remote_diag_sid);
       res = 1;
    }
    return res;
}



void StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
    char h1, h2;
    unsigned char s1, s2;
    int i;

    for (i = 0; i < nLen; i++)
    {
        h1 = pbSrc[2 * i];
        h2 = pbSrc[2 * i + 1];

        s1 = toupper(h1) - 0x30;

        if (s1 > 9)
        {
            s1 -= 7;
        }

        s2 = toupper(h2) - 0x30;

        if (s2 > 9)
        {
            s2 -= 7;
        }

        pbDest[i] = s1 * 16 + s2;
    }
}

/* 用于其它模块向远程诊断模块发送远程诊断请求 */
int remote_diag_request(unsigned short sender, char * diag_cmd, int diag_msg_len)
{
    TCOM_MSG_HEADER msg;


    msg.sender   = sender;
    msg.receiver = MPU_MID_REMOTE_DIAG;
    msg.msgid    = REMOTE_DIAG_REQUEST;
    msg.msglen   = diag_msg_len;
    ;
    if (tcom_send_msg(&msg, diag_cmd))
    {
        return REMOTE_DIAG_ERROR_INSIDE;
    }

    return REMOTE_DIAG_OK;
}


/* 用于其它模块向远程诊断模块发送消息 */
int send_msg_to_remote_diag(unsigned short sender, unsigned int msgid,
                            unsigned char *diag_cmd, int diag_msg_len)
{
    TCOM_MSG_HEADER msg;


    msg.sender   = sender;
    msg.receiver = MPU_MID_REMOTE_DIAG;
    msg.msgid    = msgid;
    msg.msglen   = diag_msg_len;

    if (tcom_send_msg(&msg, diag_cmd))
    {
        return REMOTE_DIAG_ERROR_INSIDE;
    }

    return REMOTE_DIAG_OK;
}


/* 用于UDS server 向 远程诊断模块发送远程诊断结果 */
int remote_diag_send_tbox_response(unsigned char *msg, unsigned int len)
{
    int ret;
    ret = send_msg_to_remote_diag(MPU_MID_UDS, REMOTE_DIAG_UDS_RESPONSE, msg, len);
    return ret;
}



/* 远程诊断模块向外发送消息 请求 T-Box UDS 模块，或者 MCU*/
int remote_diag_send_request(unsigned char *msg, unsigned int len)
{
    int ret;
    char msg_temp[DIAG_REQUEST_LEN];
    remote_diag_request_t *request = NULL;
    request = get_current_diag_cmd_request();

    if (1 == is_remote_diag_tbox()) /*诊断tbox*/
    {
        TCOM_MSG_HEADER msghdr;

        /* send message to the receiver */
        msghdr.sender     = MPU_MID_REMOTE_DIAG;
        msghdr.receiver   = MPU_MID_UDS;
        msghdr.msgid      = UDS_SCOM_MSG_IND;
        msghdr.msglen     = len + 7;

        /* msg type*/
        msg_temp[0] = MSG_ID_UDS_IND;

        /*CAN ID*/
        msg_temp[1] = (request->request_id & 0x000000ff);
        msg_temp[2] = (request->request_id & 0x0000ff00) >> 8;
        msg_temp[3] = (request->request_id & 0x00ff0000) >> 16;
        msg_temp[4] = (request->request_id & 0xff000000) >> 24;

        /*msg len*/
        msg_temp[5] = (len) % 0xff;
        msg_temp[6] = (len) / 0xff;

        memcpy(&(msg_temp[7]), msg, len);
        ret = tcom_send_msg(&msghdr, msg_temp);

        if (ret != 0)
        {
            log_e(LOG_REMOTE_DIAG, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
                  msghdr.msgid, msghdr.receiver, ret);

        }
    }
    else/* 诊断其它ECU*/
    {
        ret = uds_data_request(&uds_client, MSG_ID_UDS_REQ, request->request_id, msg, len);

        if (ret != 0)
        {
            log_e(LOG_REMOTE_DIAG, "send remote diag request to mcu failed!");

        }
    }

    return ret;

}


/*用于远程诊断执行之后的结果输出*/
int remote_diag_excuted_result_output(void)
{
    int ret = 0;

    PP_rmtDiag_queryInform_cb();/* 向PP模块发送指令已执行完成 */
    log_o(LOG_REMOTE_DIAG, "call PP_rmtDiag_queryInform_cb");

    return ret;
}


static void HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
    char ddl, ddh;
    int i;

    for (i = 0; i < nLen; i++)
    {
        ddh = 48 + pbSrc[i] / 16;
        ddl = 48 + pbSrc[i] % 16;

        if (ddh > 57)
        {
            ddh = ddh + 7;
        }

        if (ddl > 57)
        {
            ddl = ddl + 7;
        }

        pbDest[i * 2] = ddh;
        pbDest[i * 2 + 1] = ddl;
    }

    pbDest[nLen * 2] = '\0';
}

static void dtc_to_str(unsigned char * dtc_str, unsigned char * DTC)
{
    unsigned char DTC_temp[3] = {0};
    memcpy(DTC_temp, DTC, 3);
    dtc_str[0] = dtc_to_str_arr[(DTC_temp[0] & 0xC0)>>6];
    
    DTC_temp[0] = DTC_temp[0] & 0x3F;
    HexToStr(&(dtc_str[1]), DTC_temp, 2);
}

/*
input:
obj:ecu type

return:
const char clear_result_success = 0;
const char clear_result_failed = 1;
*/
int PP_get_remote_clearDTCresult(uint8_t obj, unsigned char *failureType)
{
    
    const char clear_result_success = 0;
    const char clear_result_failed = 1;
    
    int ret = clear_result_failed;
    
    int response_size = 0;

    remote_diag_response_arr_t * response_arr = get_remote_diag_response();

    
    /* 轮询查找匹配结果 */
    for(response_size=0;response_size<response_arr->remote_diag_response_size;response_size++)
    {
        
        /* 已匹配结果 */
        if(response_arr->remote_diag_response[response_size].request_id == REMOTE_DIAG_CAN_ID[obj][DIAG_REQUEST_ID_ROW])
        {

            if(REMOTE_DIAG_CMD_RESULT_OK == response_arr->remote_diag_response[response_size].result_type)
            {
                ret = clear_result_success;
                *failureType = PP_RMTDIAG_ERROR_NONE;
            }
            else if(REMOTE_DIAG_CMD_RESULT_NEGATIVE == response_arr->remote_diag_response[response_size].result_type)
            {
                ret = clear_result_failed;
                *failureType = PP_RMTDIAG_ERROR_ECURESERRCODE;
            }
            else if(REMOTE_DIAG_CMD_RESULT_TIMEOUT == response_arr->remote_diag_response[response_size].result_type)
            {
                ret = clear_result_failed;
                *failureType = PP_RMTDIAG_ERROR_ECUNORES;
            }
            
            /* 读取之后就销毁 */
            int response_cpy_size =  response_size;
            for(;response_cpy_size<response_arr->remote_diag_response_size;response_cpy_size++)
            {
                memcpy(&(response_arr->remote_diag_response[response_cpy_size]), 
                                   &(response_arr->remote_diag_response[response_cpy_size + 1]), 
                                   sizeof(remote_diag_response_t));
            }

            break;
        }
    }

    log_i(LOG_REMOTE_DIAG, "ret:%d, failureType:%d", ret, *failureType);
    return ret;
}

int charcmp(const uint8_t *src,const uint8_t *dst, const int len)
{
    int ret=0;
    int len_temp = len;
    while(!(ret = *(unsigned char *)src - *(unsigned char *)dst) && len_temp)
    {
        ++src;
        ++dst;
        len_temp--;
    }
        
    if(ret<0)
        ret=-1;
    else if(ret>0)
        ret=1;
    return(ret);
}
int vcu_parsing_time(const uint8_t *did_value, RTCTIME *time)
{
    int ret = 0;
    if((did_value[0] == 0xF0) || (did_value[1] == 0x20))
    {
        time->year = 2018 + did_value[2];
        time->mon = did_value[3];
        time->mday = did_value[4];
        time->hour = did_value[5];
        time->min = did_value[6];
        time->sec = did_value[7];
        log_i(LOG_REMOTE_DIAG, "time->year:%d", time->year);
        log_i(LOG_REMOTE_DIAG, "time->mon:%d", time->mon);
        log_i(LOG_REMOTE_DIAG, "time->mday:%d", time->mday);
        log_i(LOG_REMOTE_DIAG, "time->hour:%d", time->hour);
        log_i(LOG_REMOTE_DIAG, "time->min:%d", time->min);
        log_i(LOG_REMOTE_DIAG, "time->sec:%d", time->sec);
    }
    else
    {
        memset(time, 0x00, sizeof(RTCTIME));
        ret = 1;
        log_e(LOG_REMOTE_DIAG, "did do not match error!");
    }
    return ret;
}

int LSB_parsing_time(const uint8_t *did_value, RTCTIME *time)
{
    int ret = 0;
    if((did_value[0] == 0xF0) || (did_value[1] == 0x20))
    {
        time->year = (((unsigned short)did_value[3])<<8) + did_value[2];
        time->mon = did_value[4];
        time->mday = did_value[5];
        time->hour = did_value[6];
        time->min = did_value[7];
        time->sec = did_value[8];
        log_i(LOG_REMOTE_DIAG, "time->year:%d", time->year);
        log_i(LOG_REMOTE_DIAG, "time->mon:%d", time->mon);
        log_i(LOG_REMOTE_DIAG, "time->mday:%d", time->mday);
        log_i(LOG_REMOTE_DIAG, "time->hour:%d", time->hour);
        log_i(LOG_REMOTE_DIAG, "time->min:%d", time->min);
        log_i(LOG_REMOTE_DIAG, "time->sec:%d", time->sec);
    }
    else
    {
        memset(time, 0x00, sizeof(RTCTIME));
        ret = 1;
        log_e(LOG_REMOTE_DIAG, "did do not match error!");
    }
    return ret;
}


int comm_parsing_time(const uint8_t *did_value, RTCTIME *time)
{
    int ret = 0;
    if((did_value[0] == 0xF0) || (did_value[1] == 0x20))
    {
        time->year = (((unsigned short)did_value[2])<<8) + did_value[3];
        time->mon = did_value[4];
        time->mday = did_value[5];
        time->hour = did_value[6];
        time->min = did_value[7];
        time->sec = did_value[8];
        log_i(LOG_REMOTE_DIAG, "time->year:%d", time->year);
        log_i(LOG_REMOTE_DIAG, "time->mon:%d", time->mon);
        log_i(LOG_REMOTE_DIAG, "time->mday:%d", time->mday);
        log_i(LOG_REMOTE_DIAG, "time->hour:%d", time->hour);
        log_i(LOG_REMOTE_DIAG, "time->min:%d", time->min);
        log_i(LOG_REMOTE_DIAG, "time->sec:%d", time->sec);
    }
    else
    {
        memset(time, 0x00, sizeof(RTCTIME));
        ret = 1;
        log_e(LOG_REMOTE_DIAG, "did do not match error!");
    }
    return ret;
}

int ehb_parsing_time(const uint8_t *did_value, RTCTIME *time)
{
    int ret = 0;
    if((did_value[0] == 0x01) || (did_value[1] == 0x0B))
    {
        time->year = 2000 + did_value[2];
        time->mon = did_value[3];
        time->mday = did_value[4];
        time->hour = did_value[5];
        time->min = did_value[6];
        time->sec = did_value[7];
        log_i(LOG_REMOTE_DIAG, "time->year:%d", time->year);
        log_i(LOG_REMOTE_DIAG, "time->mon:%d", time->mon);
        log_i(LOG_REMOTE_DIAG, "time->mday:%d", time->mday);
        log_i(LOG_REMOTE_DIAG, "time->hour:%d", time->hour);
        log_i(LOG_REMOTE_DIAG, "time->min:%d", time->min);
        log_i(LOG_REMOTE_DIAG, "time->sec:%d", time->sec);

    }
    else
    {
        memset(time, 0x00, sizeof(RTCTIME));
        ret = 1;
        log_e(LOG_REMOTE_DIAG, "did do not match error!");
    }
    return ret;
}

int remote_diag_changetime(RTCTIME *time, uint32_t * timesec)
{
    struct tm tm;

    tm.tm_year = time->year - 1900;
    tm.tm_mon = time->mon - 1;
    tm.tm_mday = time->mday;
    tm.tm_hour = time->hour;
    tm.tm_min = time->min;
    tm.tm_sec = time->sec;

    *timesec = mktime(&tm);
    
    return 0;
}

int PP_get_dtc_time_result(uint8_t obj, PP_rmtDiag_faultcode_t *faultcode)
{
    int ret = 1;
    int response_size = 0;
    int byte_size = 2;/*apart from 59 04 XX XX XX 09 XX XX*/
    unsigned char DTC_temp[5] = {0};
    unsigned char * response;
    remote_diag_response_arr_t * response_arr = get_remote_diag_response();
    
    /* 轮询查找匹配结果 */
    for(response_size=0;response_size<response_arr->remote_diag_response_size;response_size++)
    {
        /* 已匹配结果 */
        if(response_arr->remote_diag_response[response_size].request_id == REMOTE_DIAG_CAN_ID[obj][DIAG_REQUEST_ID_ROW])
        {
            if(REMOTE_DIAG_CMD_RESULT_OK == response_arr->remote_diag_response[response_size].result_type)
            {
                response = response_arr->remote_diag_response[response_size].diag_response;
                dtc_to_str(DTC_temp,response+byte_size);

                log_buf_dump(LOG_REMOTE_DIAG, "DTC_temp", DTC_temp, 5);
                log_buf_dump(LOG_REMOTE_DIAG, "faultcode->diagcode", faultcode->diagcode, 5);
                log_i(LOG_REMOTE_DIAG, "faultcode->lowByte:%x, *(response+byte_size+2):%x", 
                faultcode->lowByte, *(response+byte_size+2));

                if((DTC_temp[0] == faultcode->diagcode[0])
                &&(DTC_temp[1] == faultcode->diagcode[1])
                &&(DTC_temp[2] == faultcode->diagcode[2])
                &&(DTC_temp[3] == faultcode->diagcode[3])
                &&(DTC_temp[4] == faultcode->diagcode[4])
                &&(faultcode->lowByte == *(response+byte_size+2)))/* Get DTC corresponding snapshot information */
                {
                    byte_size = byte_size + 6;
                    byte_size += time_pos[obj];
                    RTCTIME time;
                    memset(&time, 0x00, sizeof(RTCTIME));
                    if(parsing_time_arr[obj] != NULL)
                    {
                        parsing_time_arr[obj](response+byte_size, &time);
                        remote_diag_changetime(&time, &(faultcode->diagTime));
                        ret = 0;
                    }
                    else
                    {
                        log_e(LOG_REMOTE_DIAG, "parsing_time_arr is NULL error!");
                    }
                }
                else
                {
                    log_e(LOG_REMOTE_DIAG, "dtc does not match error!");
                }
            }

            /* 读取之后就销毁 */
            int response_cpy_size =  response_size;
            for(;response_cpy_size<response_arr->remote_diag_response_size;response_cpy_size++)
            {
                memcpy(&(response_arr->remote_diag_response[response_cpy_size]), 
                                   &(response_arr->remote_diag_response[response_cpy_size + 1]), 
                                   sizeof(remote_diag_response_t));
            }
        }
    }
    if(ret != 0)
    {
        log_e(LOG_REMOTE_DIAG, "PP_get_dtc_time_result error:ret:%d", ret);
    }
    return ret;
}

int PP_get_remote_result(uint8_t obj, PP_rmtDiag_Fault_t * pp_rmtdiag_fault)
{
    int ret = 0;
    int response_size = 0;
    int byte_size = 3;/*apart from 59 0A 09*/
    unsigned char dtc_num = 0;
    unsigned char DTC_temp[4];
    unsigned char * response;
    remote_diag_response_arr_t * response_arr = get_remote_diag_response();

    pp_rmtdiag_fault->sueecss = 0;/* 默认执行失败 满足成功条件再置成功 */
    
    /* 轮询查找匹配结果 */
    for(response_size=0;response_size<response_arr->remote_diag_response_size;response_size++)
    {
        
        /* 已匹配结果 */
        if(response_arr->remote_diag_response[response_size].request_id == REMOTE_DIAG_CAN_ID[obj][DIAG_REQUEST_ID_ROW])
        {

            if(REMOTE_DIAG_CMD_RESULT_OK == response_arr->remote_diag_response[response_size].result_type)
            {
                response = response_arr->remote_diag_response[response_size].diag_response;
                while(byte_size<(response_arr->remote_diag_response[response_size].diag_response_len))
                {
                    memcpy(DTC_temp,response+byte_size,4);
                    byte_size = byte_size + 4;
                    
                    if((DTC_temp[3] & 0x09) == 0x08)/* 历史故障 */
                    {
                        dtc_to_str(pp_rmtdiag_fault->faultcode[dtc_num].diagcode,DTC_temp);
                        pp_rmtdiag_fault->faultcode[dtc_num].diagTime = 0;
                        pp_rmtdiag_fault->faultcode[dtc_num].faultCodeType = 1;/* 历史故障 */
                        pp_rmtdiag_fault->faultcode[dtc_num].lowByte = DTC_temp[2];
                        dtc_num++;
                    }
                    else if((DTC_temp[3] & 0x09) == 0x09)/* 当前故障*/
                    {
                        dtc_to_str(pp_rmtdiag_fault->faultcode[dtc_num].diagcode,DTC_temp);
                        
                        pp_rmtdiag_fault->faultcode[dtc_num].diagTime = 0;
                        pp_rmtdiag_fault->faultcode[dtc_num].faultCodeType = 0;/* 当前故障 */
                        pp_rmtdiag_fault->faultcode[dtc_num].lowByte = DTC_temp[2];
                        dtc_num++;
                    }
                    else
                    {
                    }
                }
                /* 执行结果中有该ECU的结果，且该结果正常，则执行成功，其它情况都认为执行失败 */
                pp_rmtdiag_fault->sueecss = 1;
                pp_rmtdiag_fault->failureType = PP_RMTDIAG_ERROR_NONE;
            }
            else if(REMOTE_DIAG_CMD_RESULT_NEGATIVE == response_arr->remote_diag_response[response_size].result_type)
            {
                pp_rmtdiag_fault->sueecss = 0;
                pp_rmtdiag_fault->failureType = PP_RMTDIAG_ERROR_ECURESERRCODE;
            }
            else if(REMOTE_DIAG_CMD_RESULT_TIMEOUT == response_arr->remote_diag_response[response_size].result_type)
            {
                pp_rmtdiag_fault->sueecss = 0;
                pp_rmtdiag_fault->failureType = PP_RMTDIAG_ERROR_ECUNORES;
            }

            /* 读取之后就销毁 */
            int response_cpy_size =  response_size;
            for(;response_cpy_size<response_arr->remote_diag_response_size;response_cpy_size++)
            {
                memcpy(&(response_arr->remote_diag_response[response_cpy_size]), 
                                   &(response_arr->remote_diag_response[response_cpy_size + 1]), 
                                   sizeof(remote_diag_response_t));
            }

            break;
        }
    }
    pp_rmtdiag_fault->faultNum = dtc_num;
    log_i(LOG_REMOTE_DIAG, "pp_rmtdiag_fault->sueecss:%d, failureType:%d", pp_rmtdiag_fault->sueecss, pp_rmtdiag_fault->failureType);
    return ret;
}







