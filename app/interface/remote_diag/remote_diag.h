#ifndef __REMOTE_DIAG_H__
#define __REMOTE_DIAG_H__
#include "mid_def.h"
#include "remote_diag_api.h"
#include "tcom_api.h"


#define REMOTE_DIAG_MSG_LEN 1024
#define DIAG_REQUEST_LEN 200
#define DIAG_REQUEST_SINGEL_MSG_LEN 500 /*单个远程诊断消息长度*/
#define DIAG_REQUEST_MSG_LEN 50000        /*总的远程诊断消息长度*/

#define DIAG_RESPONSE_LEN 200

#define MAX_REMOTE_REQUEST 128

#define REMOTE_DIAG_MSG_TIMEOUT_INTERVAL 1000

#define EXTEND_SESSION_CMD "1003" /* 切换扩展会话命令 */
#define SECURITY_LEVEL1_REQUEST_SEED "2703"  /* 安全访问等级1请求种子 */
#define SECURITY_LEVEL3_REQUEST_SEED "2709"  /* 安全访问等级3请求种子 */
#define REPORT_SUPPORTED_DTC "190A" /* 根据状态掩码报告 确认DTC和测试失败 DTC */

#define DIAG_REQUEST_ID_ROW   0
#define DIAG_RESPONSE_ID_ROW  1



typedef enum
{
	REMOTE_DIAG_ALL = 0,//
	REMOTE_DIAG_VCU,//
    REMOTE_DIAG_BMS,//
	REMOTE_DIAG_MCUp,
	REMOTE_DIAG_OBCp,
	REMOTE_DIAG_FLR,
	REMOTE_DIAG_FLC,
	REMOTE_DIAG_APA,
	REMOTE_DIAG_ESCPluse,
	REMOTE_DIAG_EPS,
	REMOTE_DIAG_EHB,
	REMOTE_DIAG_BDCM,
	REMOTE_DIAG_GW,
	REMOTE_DIAG_LSA,
	REMOTE_DIAG_CLM,
	REMOTE_DIAG_PTC,
	REMOTE_DIAG_EACP,
	REMOTE_DIAG_EGSM,
	REMOTE_DIAG_ALM,
	REMOTE_DIAG_WPC,
	REMOTE_DIAG_IHU,
	REMOTE_DIAG_ICU,
	REMOTE_DIAG_IRS,
	REMOTE_DIAG_DVR,
	REMOTE_DIAG_TAP,
	REMOTE_DIAG_MFCP,
	REMOTE_DIAG_TBOX,
	REMOTE_DIAG_ACU,
	REMOTE_DIAG_PLG,
	REMOTE_DIAG_ECU_NUM,
} REMOTE_DIAG_ECUTYPE;





/* 单条远程诊断命令执行状态 */
typedef enum
{
    REMOTE_DIAG_CMD_NOT_START = 0,//未开始
    REMOTE_DIAG_CMD_OPENING,/* 已发送配置命令，等待响应*/
    REMOTE_DIAG_CMD_OPENED,/* 配置成功，尚未发送UDS诊断请求*/
    REMOTE_DIAG_CMD_SESSION_CONTROLLING,/* 会话控制 切换会话中*/
    REMOTE_DIAG_CMD_SESSION_CONTROLLED, /* 会话控制已满足 */
    REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTING,/* 安全访问请求种子中*/
    REMOTE_DIAG_CMD_SECURITY_SEED_REQUESTED,/* 安全访问已请求种子，并接收到种子*/
    REMEOT_DIAG_CMD_SECURITY_KEY_SENDING,/* 安全访问，发送key中*/
    REMOTE_DIAG_CMD_SECURITY_KEY_SENDED,/* 安全访问已解锁 */
    REMOTE_DIAG_CMD_DIAGNOSING,/* 已发送诊断请求，等待诊断响应*/
    REMOTE_DIAG_CMD_DIAGNOSED,/* 已接收响应，结束 */
} REMOTE_DIAG_CMD_STATE_TYPE;


/* 单条远程诊断命令执行结果 */
typedef enum
{
    REMOTE_DIAG_CMD_RESULT_OK,/*已正确结束*/
    REMOTE_DIAG_CMD_RESULT_SESSION_CONTROL_FAILED,/* 会话控制失败 */
    REMOTE_DIAG_CMD_RESULT_SECURITY_ACCESS_FAILED,/* 安全解锁失败 */
    REMOTE_DIAG_CMD_RESULT_NEGATIVE,/*接收到否定响应*/
    REMOTE_DIAG_CMD_RESULT_TIMEOUT,/* 响应超时 */
}REMOTE_DIAG_CMD_RESULT_TYPE;

/* 远程诊断命令执行状态 */
typedef enum
{
    REMOTE_DIAG_IDLE = 0,//未开始
    REMOTE_DIAG_EXECUTING,//执行中
    REMOTE_DIAG_FINISHED,//执行完成，并已将远程诊断结果发送出去
} REMOTE_DIAG_STATE_TYPE;




/* 远程诊断线程消息定义 */
typedef enum
{
    REMOTE_DIAG_REQUEST = MPU_MID_REMOTE_DIAG,//平台发送的远程诊断请求
    REMOTE_DIAG_UDS_RESPONSE,/* Tbox UDS 的远程诊断响应 */
    REMOTE_DIAG_SET_MCU_RESPONSE,/* 设置MCU UDS参数的响应 */
    REMOTE_DIAG_MCU_RESPONSE,/* MCU返回的其它ECU的响应 */
    REMOTE_DIAG_REQUEST_TIMEOUT,/* 请求已发出，响应超时*/
} REMOTE_DIAG_MSG_TYPE;

typedef struct
{
    unsigned int baud;
    unsigned int port;
    unsigned int request_id;
    unsigned int response_id;
    unsigned int diag_request_len;
    unsigned char level;
    unsigned char session;
    unsigned char diag_request[DIAG_REQUEST_LEN];/*UDS msg*/
} remote_diag_request_t;

typedef struct
{
    remote_diag_request_t remote_diag_request[MAX_REMOTE_REQUEST];
    int remote_diag_request_size;
} remote_diag_request_arr_t;

typedef struct
{
    unsigned int request_id;
    unsigned int response_id;
    unsigned char diag_response[DIAG_RESPONSE_LEN];/*UDS msg*/
    unsigned int diag_response_len;
    REMOTE_DIAG_CMD_RESULT_TYPE result_type;
} remote_diag_response_t;

typedef struct
{
    remote_diag_response_t remote_diag_response[MAX_REMOTE_REQUEST];
    int remote_diag_response_size;
} remote_diag_response_arr_t;


typedef struct
{
    union
    {
        char msg[REMOTE_DIAG_MSG_LEN];
    };
} remote_diag_msg_t;

typedef enum
{
    REMOTE_DIAG_OK = 0,
    REMOTE_DIAG_ERROR_PAR_INVALID,//参数错误
    REMOTE_DIAG_ERROR_WAKEUP_TMO, //唤醒车辆超时
    REMOTE_DIAG_ERROR_SECUR_TMO,  //安全认证超时
    REMOTE_DIAG_ERROR_INSIDE,     //内部错误，如通信错误
    REMOTE_DIAG_ERROR_CONDITION,  //条件不满足
    REMOTE_DIAG_ERROR_EX_FAIL,    //执行失败
} remote_diag_error_t;

typedef struct
{
    /* 远程诊断请求 */
    remote_diag_request_arr_t remote_diag_request_arr;

    /* 远程诊断响应 */
    remote_diag_response_arr_t remote_diag_response_arr;
    
    /* 当前诊断命令序号，用于接收响应处理 */
    int current_cmd_no;

    /* 当前诊断命令状态 */
    REMOTE_DIAG_CMD_STATE_TYPE current_cmd_state;

    /* 当前远程诊断模块状态 */
    REMOTE_DIAG_STATE_TYPE remote_diag_state;
} remote_diag_state_t;

void parsing_remote_diag_msg(char * remote_diag_msg, TCOM_MSG_HEADER msg, remote_diag_request_arr_t * remote_diag_request_arr);
int remote_diag_msg_output(void);
int update_remote_diag_cmd_no(void);
int is_remote_diag_need(void);
int update_remote_diag_state(void);
REMOTE_DIAG_STATE_TYPE get_remote_diag_state(void);
remote_diag_response_arr_t * get_remote_diag_response(void);
remote_diag_response_t * get_current_diag_cmd_response(void);
remote_diag_request_t * get_current_diag_cmd_request(void);
REMOTE_DIAG_CMD_STATE_TYPE get_current_diag_cmd_state(void);
int remote_diag_cmd_set_error(REMOTE_DIAG_CMD_RESULT_TYPE ret_type, char * remote_diag_msg, unsigned int mlen);
int remote_diag_cmd_state_update(void);
void StrToHex(unsigned char * pbDest, unsigned char * pbSrc, int nLen);
void HexToStr(unsigned char *pbDest, unsigned char *pbSrc, int nLen);

int is_remote_diag_tbox(void);
int remote_diag_send_response(unsigned short sender, unsigned int   msgid, unsigned char *msg, unsigned int len);
int remote_diag_mcuuds_request_process(void);
int remote_diag_single_cmd_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen);
int remote_diag_error_process(REMOTE_DIAG_CMD_RESULT_TYPE ret_type, char * remote_diag_msg, unsigned int mlen);
void init_remote_diag_state(void);
int remote_diag_process_start(void);
remote_diag_request_arr_t * get_remote_diag_request(void);
remote_diag_state_t * get_remote_diag_state_t(void);
int remote_diag_send_request(unsigned char *msg, unsigned int len);
void remote_diag_process(unsigned int msgid, char * remote_diag_msg, unsigned int mlen);

void remote_diag_calcKey(char      * seed, char * key, unsigned char len);



#endif //__REMOTE_DIAG_H__
