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


extern UDS_T    uds_client;

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



/* 将接收到的远程诊断消息，解析为远程诊断模块可处理的标准请求格式 */
void parsing_remote_diag_msg(char * remote_diag_msg, TCOM_MSG_HEADER msg, remote_diag_request_arr_t * remote_diag_request_arr)
{
    
    shellprintf("remote diag get msg:%s", remote_diag_msg);
    unsigned int mlen = msg.msglen;

    if(msg.sender == MPU_MID_SHELL)/* shell命令发送的诊断消息 */
    {
        if(2 == mlen)
        {
            int current_size = 0;
            unsigned char request_msg[DIAG_REQUEST_LEN];
            StrToHex(request_msg, (unsigned char *)remote_diag_msg, mlen/2);
            memset(remote_diag_request_arr, 0x00, sizeof(remote_diag_request_arr_t));
            if(request_msg[0] == REMOTE_DIAG_ALL)/* 所有ECU*/
            {
                for(current_size=0;current_size<REMOTE_DIAG_ECU_NUM;current_size++)
                {
                    remote_diag_request_arr->remote_diag_request[current_size].baud = 500;
                    remote_diag_request_arr->remote_diag_request[current_size].port = 1;
                    remote_diag_request_arr->remote_diag_request[current_size].request_id = REMOTE_DIAG_CAN_ID[current_size][DIAG_REQUEST_ID_ROW];
                    remote_diag_request_arr->remote_diag_request[current_size].response_id = REMOTE_DIAG_CAN_ID[current_size][DIAG_RESPONSE_ID_ROW];
                    remote_diag_request_arr->remote_diag_request[current_size].level = SecurityAccess_LEVEL0;
                    remote_diag_request_arr->remote_diag_request[current_size].session = SESSION_TYPE_DEFAULT;
                    StrToHex(remote_diag_request_arr->remote_diag_request[current_size].diag_request, 
                            (unsigned char *)REPORT_SUPPORTED_DTC, strlen(REPORT_SUPPORTED_DTC)/2);/* 根据状态掩码报告DTC */
                    remote_diag_request_arr->remote_diag_request[current_size].diag_request_len = 
                        strlen((const char *)remote_diag_request_arr->remote_diag_request[current_size].diag_request);

                    log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                                 remote_diag_request_arr->remote_diag_request[current_size].baud,
                                 remote_diag_request_arr->remote_diag_request[current_size].port,
                                 remote_diag_request_arr->remote_diag_request[current_size].request_id,
                                 remote_diag_request_arr->remote_diag_request[current_size].response_id,
                                 remote_diag_request_arr->remote_diag_request[current_size].diag_request_len,
                                 remote_diag_request_arr->remote_diag_request[current_size].level,
                                 remote_diag_request_arr->remote_diag_request[current_size].session,
                                 remote_diag_request_arr->remote_diag_request[current_size].diag_request
                                );
                            
                }
                remote_diag_request_arr->remote_diag_request_size= current_size;
            }
            else
            {
                remote_diag_request_arr->remote_diag_request[0].baud = 500;
                remote_diag_request_arr->remote_diag_request[0].port = 1;
                remote_diag_request_arr->remote_diag_request[0].request_id = REMOTE_DIAG_CAN_ID[request_msg[0]][DIAG_REQUEST_ID_ROW];
                remote_diag_request_arr->remote_diag_request[0].response_id = REMOTE_DIAG_CAN_ID[request_msg[0]][DIAG_RESPONSE_ID_ROW];
                remote_diag_request_arr->remote_diag_request[0].level = SecurityAccess_LEVEL0;
                remote_diag_request_arr->remote_diag_request[0].session = SESSION_TYPE_DEFAULT;
                StrToHex(remote_diag_request_arr->remote_diag_request[0].diag_request, 
                        (unsigned char *)REPORT_SUPPORTED_DTC, strlen(REPORT_SUPPORTED_DTC)/2);/* 根据状态掩码报告DTC */
                remote_diag_request_arr->remote_diag_request[0].diag_request_len = 
                    strlen((const char *)remote_diag_request_arr->remote_diag_request[0].diag_request);
                    
                log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                                 remote_diag_request_arr->remote_diag_request[0].baud,
                                 remote_diag_request_arr->remote_diag_request[0].port,
                                 remote_diag_request_arr->remote_diag_request[0].request_id,
                                 remote_diag_request_arr->remote_diag_request[0].response_id,
                                 remote_diag_request_arr->remote_diag_request[0].diag_request_len,
                                 remote_diag_request_arr->remote_diag_request[0].level,
                                 remote_diag_request_arr->remote_diag_request[0].session,
                                 remote_diag_request_arr->remote_diag_request[0].diag_request
                                );
                                
                remote_diag_request_arr->remote_diag_request_size= 1;
            }
        }
        else
        {
            int current_size = 0;
            unsigned char request_msg[DIAG_REQUEST_LEN];
            memset(request_msg, 0x00, DIAG_REQUEST_LEN);
            memset(remote_diag_msg+mlen, 0x00, 1);
            char * temp = strtok(remote_diag_msg, "//");
            while(temp)
            {
                memset(remote_diag_request_arr->remote_diag_request[current_size].diag_request, 0x00, DIAG_REQUEST_LEN);
                
                sscanf(temp, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                                 &(remote_diag_request_arr->remote_diag_request[current_size].baud),
                                 &(remote_diag_request_arr->remote_diag_request[current_size].port),
                                 &(remote_diag_request_arr->remote_diag_request[current_size].request_id),
                                 &(remote_diag_request_arr->remote_diag_request[current_size].response_id),
                                 &(remote_diag_request_arr->remote_diag_request[current_size].diag_request_len),
                                 (int *)(&(remote_diag_request_arr->remote_diag_request[current_size].level)),
                                 (int *)(&(remote_diag_request_arr->remote_diag_request[current_size].session)),
                                 request_msg
                                );
        
                StrToHex(remote_diag_request_arr->remote_diag_request[current_size].diag_request, request_msg, strlen((const char *)request_msg) / 2);
                
                log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                                 remote_diag_request_arr->remote_diag_request[current_size].baud,
                                 remote_diag_request_arr->remote_diag_request[current_size].port,
                                 remote_diag_request_arr->remote_diag_request[current_size].request_id,
                                 remote_diag_request_arr->remote_diag_request[current_size].response_id,
                                 remote_diag_request_arr->remote_diag_request[current_size].diag_request_len,
                                 remote_diag_request_arr->remote_diag_request[current_size].level,
                                 remote_diag_request_arr->remote_diag_request[current_size].session,
                                 remote_diag_request_arr->remote_diag_request[current_size].diag_request
                                );
                current_size++;
                temp = strtok(NULL, "//");
            }
                             
            remote_diag_request_arr->remote_diag_request_size= current_size;

        }
    }
    else if(msg.sender == MPU_MID_REMOTE_DIAG)/* 平台发送的远程诊断消息*/
    {
        int current_size = 0;
        memset(remote_diag_request_arr, 0x00, sizeof(remote_diag_request_arr_t));
        if(remote_diag_msg[0] == REMOTE_DIAG_ALL)/* 所有ECU*/
        {
            for(current_size=0;current_size<REMOTE_DIAG_ECU_NUM;current_size++)
            {
                remote_diag_request_arr->remote_diag_request[current_size].baud = 500;
                remote_diag_request_arr->remote_diag_request[current_size].port = 1;
                remote_diag_request_arr->remote_diag_request[current_size].request_id = REMOTE_DIAG_CAN_ID[current_size][DIAG_REQUEST_ID_ROW];
                remote_diag_request_arr->remote_diag_request[current_size].response_id = REMOTE_DIAG_CAN_ID[current_size][DIAG_RESPONSE_ID_ROW];
                remote_diag_request_arr->remote_diag_request[current_size].level = SecurityAccess_LEVEL0;
                remote_diag_request_arr->remote_diag_request[current_size].session = SESSION_TYPE_DEFAULT;
                StrToHex(remote_diag_request_arr->remote_diag_request[current_size].diag_request, 
                        (unsigned char *)REPORT_SUPPORTED_DTC, strlen(REPORT_SUPPORTED_DTC)/2);/* 根据状态掩码报告DTC */
                remote_diag_request_arr->remote_diag_request[current_size].diag_request_len = 
                    strlen((const char *)remote_diag_request_arr->remote_diag_request[current_size].diag_request);

                log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                             remote_diag_request_arr->remote_diag_request[current_size].baud,
                             remote_diag_request_arr->remote_diag_request[current_size].port,
                             remote_diag_request_arr->remote_diag_request[current_size].request_id,
                             remote_diag_request_arr->remote_diag_request[current_size].response_id,
                             remote_diag_request_arr->remote_diag_request[current_size].diag_request_len,
                             remote_diag_request_arr->remote_diag_request[current_size].level,
                             remote_diag_request_arr->remote_diag_request[current_size].session,
                             remote_diag_request_arr->remote_diag_request[current_size].diag_request
                            );
                        
            }
            remote_diag_request_arr->remote_diag_request_size= current_size;
        }
        else if(remote_diag_msg[0] < REMOTE_DIAG_ECU_NUM)
        {
            remote_diag_request_arr->remote_diag_request[0].baud = 500;
            remote_diag_request_arr->remote_diag_request[0].port = 1;
            remote_diag_request_arr->remote_diag_request[0].request_id = REMOTE_DIAG_CAN_ID[(unsigned char)(remote_diag_msg[0])][DIAG_REQUEST_ID_ROW];
            remote_diag_request_arr->remote_diag_request[0].response_id = REMOTE_DIAG_CAN_ID[(unsigned char)(remote_diag_msg[0])][DIAG_RESPONSE_ID_ROW];
            remote_diag_request_arr->remote_diag_request[0].level = SecurityAccess_LEVEL0;
            remote_diag_request_arr->remote_diag_request[0].session = SESSION_TYPE_DEFAULT;
            StrToHex(remote_diag_request_arr->remote_diag_request[0].diag_request, 
                    (unsigned char *)REPORT_SUPPORTED_DTC, strlen(REPORT_SUPPORTED_DTC)/2);/* 根据状态掩码报告DTC */
            remote_diag_request_arr->remote_diag_request[0].diag_request_len = 
                strlen((const char *)remote_diag_request_arr->remote_diag_request[0].diag_request);
                
            log_o(LOG_REMOTE_DIAG, "baud:%d,port:%d,request_id:%X,response_id:%X,diag_request_len:%d,security_level:%d,session:%d,diag_request:%s",
                             remote_diag_request_arr->remote_diag_request[0].baud,
                             remote_diag_request_arr->remote_diag_request[0].port,
                             remote_diag_request_arr->remote_diag_request[0].request_id,
                             remote_diag_request_arr->remote_diag_request[0].response_id,
                             remote_diag_request_arr->remote_diag_request[0].diag_request_len,
                             remote_diag_request_arr->remote_diag_request[0].level,
                             remote_diag_request_arr->remote_diag_request[0].session,
                             remote_diag_request_arr->remote_diag_request[0].diag_request
                            );
                            
            remote_diag_request_arr->remote_diag_request_size= 1;
        }
        else
        {
            log_o(LOG_REMOTE_DIAG, "remote diag recv invalid msg!");
        }
    }
    

}



int remote_diag_msg_output(void)
{
    remote_diag_response_arr_t * response_arr = NULL;
    response_arr = get_remote_diag_response();
    int size = 0;
    
    for(size=0;size<(response_arr->remote_diag_response_size);size++)
    {
        log_o(LOG_REMOTE_DIAG, "current remote diag cmd no:%d,cmd result type:%d\n", size, response_arr->remote_diag_response[size].result_type);
        log_buf_dump(LOG_UDS, ">>>>>>>>>>>>>remote diag recv mcu uds response msg>>>>>>>>>>>>>", response_arr->remote_diag_response[size].diag_response,
                                         response_arr->remote_diag_response[size].diag_response_len);
    }
    PP_rmtDiag_queryInform_cb();/* 向PP模块发送指令已执行完成 */
    return 0;
}


void StrToHex(unsigned char * pbDest, unsigned char * pbSrc, int nLen)
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


void HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
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


/* 用于其它模块向远程诊断模块发送远程诊断请求 */
int remote_diag_request(unsigned short sender, char * diag_cmd, int diag_msg_len)
{
    TCOM_MSG_HEADER msg;


    msg.sender   = sender;
    msg.receiver = MPU_MID_REMOTE_DIAG;
    msg.msgid    = REMOTE_DIAG_REQUEST;
    msg.msglen   = strlen(diag_cmd);
    ;
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
    TCOM_MSG_HEADER msghdr;

    /* send message to the receiver */
    msghdr.sender     = MPU_MID_UDS;
    msghdr.receiver   = MPU_MID_REMOTE_DIAG;
    msghdr.msgid      = REMOTE_DIAG_UDS_RESPONSE;
    msghdr.msglen     = len;

    ret = tcom_send_msg(&msghdr, msg);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
              msghdr.msgid, msghdr.receiver, ret);

        return ret;
    }
    
    return 0;

}


int remote_diag_send_response(unsigned short sender, unsigned int   msgid, unsigned char *msg, unsigned int len)
{
    int ret;
    TCOM_MSG_HEADER msghdr;

    /* send message to the receiver */
    msghdr.sender     = sender;
    msghdr.receiver   = MPU_MID_REMOTE_DIAG;
    msghdr.msgid      = msgid;
    msghdr.msglen     = len;

    ret = tcom_send_msg(&msghdr, msg);

    if (ret != 0)
    {
        log_e(LOG_SCOM, "send message(msgid:%u) to moudle(0x%04x) failed, ret:%u",
              msghdr.msgid, msghdr.receiver, ret);

        return ret;
    }
    
    return 0;

}

/* 向MCU发送远程诊断请求 */
int remote_diag_mcuuds_request_process(void)
{
    remote_diag_request_t * request = NULL;
    request = get_current_diag_cmd_request();
    
    uds_data_request(&uds_client, MSG_ID_UDS_REQ, request->request_id, request->diag_request, request->diag_request_len);
   
    return 0;
}

/* 发送远程诊断请求给 T-Box UDS 模块，或者 MCU*/
int remote_diag_send_request(unsigned char *msg, unsigned int len)
{
    int ret;
    char msg_temp[DIAG_REQUEST_LEN];
    remote_diag_request_t * request = NULL;
    request = get_current_diag_cmd_request();
    
    if(1==is_remote_diag_tbox())/*诊断tbox*/
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
        msg_temp[2] = (request->request_id & 0x0000ff00) >>8;
        msg_temp[3] = (request->request_id & 0x00ff0000) >>16;
        msg_temp[4] = (request->request_id & 0xff000000) >>24;

        /*msg len*/
        msg_temp[5] = (len)%0xff;
        msg_temp[6] = (len)/0xff;

        memcpy(&(msg_temp[7]),msg,len);
        ret = tcom_send_msg(&msghdr, msg_temp);
        log_e(LOG_REMOTE_DIAG, "ret = tcom_send_msg(&msghdr, msg_temp)");
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

static void dtc_to_str(unsigned char * dtc_str, unsigned char * DTC_temp)
{
    unsigned char dtc_to_str[4] = {'P', 'C', 'B', 'U'};
    dtc_str[0] = dtc_to_str[(DTC_temp[0] & 0xC0)>>6];
    /*
    switch(DTC_temp[0] & 0xC0)
    {
        case 0x00:
            dtc_str[0] = 'P';
            break;
        case 0x40:
            dtc_str[0] = 'C';
            break;
        case 0x80:
            dtc_str[0] = 'B';
            break;
        case 0xC0:
            dtc_str[0] = 'U';
            break;
        default:
            break;
    }*/
    
    DTC_temp[0] = DTC_temp[0] & 0x3F;
    HexToStr(&(dtc_str[1]), DTC_temp, 2);
}
int PP_get_remote_result(uint8_t obj, PP_rmtDiag_Fault_t * pp_rmtdiag_fault)
{
    int ret = 0;
    int response_size = 0;
    int byte_size = 3;
    unsigned char dtc_num = 0;
    unsigned char DTC_temp[4];
    unsigned char * response;
    remote_diag_response_arr_t * response_arr = get_remote_diag_response();
    /* 轮询查找匹配结果 */
    for(response_size=0;response_size<response_arr->remote_diag_response_size;response_size++)
    {
        
        /* 已匹配结果 */
        if(response_arr->remote_diag_response[response_size].request_id == REMOTE_DIAG_CAN_ID[obj][DIAG_REQUEST_ID_ROW])
        {
            response = response_arr->remote_diag_response[response_size].diag_response;
            while(byte_size<(response_arr->remote_diag_response[response_size].diag_response_len))
            {
                memcpy(DTC_temp,response+byte_size,4);
                byte_size = byte_size + 4;
                
                if(DTC_temp[3] == 0x08)/* 历史故障 */
                {
                    dtc_to_str(pp_rmtdiag_fault->faultcode[dtc_num].diagcode,DTC_temp);
                    pp_rmtdiag_fault->faultcode[dtc_num].diagTime = 0;
                    pp_rmtdiag_fault->faultcode[dtc_num].faultCodeType = 1;/* 历史故障 */
                    pp_rmtdiag_fault->faultcode[dtc_num].lowByte = DTC_temp[2];
                    dtc_num++;
                }
                else if(DTC_temp[3] == 0x09)/* 当前故障*/
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
            
        }
    }
    pp_rmtdiag_fault->faultNum = dtc_num;
    return ret;
}



