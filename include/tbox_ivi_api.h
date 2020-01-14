#ifndef __IVI_API_H__
#define __IVI_API_H__

#include <netinet/in.h>
#include "mid_def.h"
#include "init.h"

#define TBOX_PKI_IHU_EN                 0
#define SCDPATH                         "/ursdata/pem/HozonCA.cer"
#define ONEPATH                         "/ursdata/pem/TerminalCA.cer"


#define TBOX_PKI_IHU                     1

#define MAX_IVI_NUM                      1


#define IVI_SERVER_PORT                  5757

#define IVI_SERVER_PORT_PKI                  23000


#define IVI_GPS_TIME                     1000
#define IVI_MSG_SIZE                     2048

#define IVI_PKG_MARKER                 "#START*"
#define IVI_PKG_ESC                    "#END*"
#define IVI_PKG_S_MARKER_SIZE          (7)
#define IVI_PKG_E_MARKER_SIZE          (5)
#define IVI_PKG_CS_SIZE                (1)
#define IVI_PKG_ENCRY_SIZE             (1)
#define IVI_PKG_MSG_LEN                (2)
#define IVI_PKG_MSG_CNT                (4)

#define GPS_NMEA_SIZE                  (1024)
//#define TBOX_PKI_IHU  1
#define PKI_IDLE      0
#define PKI_INIT      1
#define PKI_ACCEPT    2
#define PKI_RECV      3
#define PKI_END       4

#define IVI_FOTA_PUSH_SUCCESS 1
#define IVI_FOTA_PUSH_FAIL    2

#define TBOX_HU_LINK_TIMEOUT  1  //HU与TBOX连接超时
#define TBOX_HU_LINK_NORMAL   0  //HU与TBOX连接正常
typedef enum
{
	UNKNOWN_YTPE = 0,
	ECALL_YTPE,
	BCALL_TYPE,
	ICALL_TYPE,
}TBOX_IVI_CALLTYPE;//控制方式

typedef enum
{
	unknown_action = 0,
	START_YTPE,
	END_TYPE,
}TBOX_IVI_CALLACTION;//控制方式


typedef struct
{
    int fd;
    unsigned int lasthearttime;
    struct sockaddr_in addr;
} ivi_client;

typedef struct
{
    uint64_t lasthearttime;  //记录心跳时间
	uint8_t stage;           //车机通信阶段
	uint8_t states;          //车机是否连接
	uint32_t accept_flag;    //accept连接成功标志
	uint8_t re_create_ssl_sock_flag; //重新连接标志
	uint8_t close_syscall_count;
} pki_client;

typedef struct{
	uint8_t ivi_net_faultflag;
	uint64_t ivi_net_timestamp;
}ivi_link_fault;

typedef struct{
	uint8_t call_type;
	uint8_t call_action;
}ivi_callrequest;

typedef struct{
	uint32_t eventid;
	uint32_t timestamp;
	uint8_t datatype;
	uint8_t cameraname;
	uint32_t aid;
	uint32_t mid;
	uint32_t effectivetime;
	uint32_t sizelimit;
}ivi_remotediagnos;

typedef struct{
	char *vin;
	  uint32_t eventid;
	  uint32_t timestamp;
	  uint32_t aid;
	  uint32_t mid;
	  uint32_t starttime;
	  uint32_t durationtime;
	  /*
	   *1:TBOX, 2:IHU
	   */
	  uint32_t channel;
	  /*
	   *1:ERROR, 2:WARN, 3:INFO, 4:DEBUG
	   */
	  uint32_t level;

}ivi_logfile;
typedef struct {
	uint32_t timestamp;
    uint32_t hour;
    uint32_t min;
    uint32_t id;
    uint32_t targetpower;
	uint16_t cmd;
  /*
  *true:effective, false:Invalid
  */
    uint8_t effectivestate;
}ivi_chargeAppointSt;

typedef enum IVI_MSG_EVENT
{
    IVI_MSG_GPS_EVENT = MPU_MID_IVI,
} IVI_MSG_EVENT;

/* initiaze thread communciation module */
int ivi_init(INIT_PHASE phase);

/* startup thread communciation module */
int ivi_run(void);

uint8_t tbox_ivi_get_call_action(void);

uint8_t tbox_ivi_get_call_type(void);

void tbox_ivi_clear_call_flag(void);

void tbox_ivi_clear_bcall_flag(void);

void tbox_ivi_clear_icall_flag(void);

extern void tbox_ivi_set_tspInformHU(ivi_remotediagnos *tsp);

extern void tbox_ivi_set_tsplogfile_InformHU(ivi_logfile *tsp);

extern void tbox_ivi_set_tspchager_InformHU(ivi_chargeAppointSt *tsp);

extern long tbox_ivi_getTimestamp(void);

extern void tbox_ivi_pki_renew_pthread();

extern uint8_t tbox_ivi_ecall_srs(void);

extern uint8_t tbox_ivi_ecall_key(void);

extern void tbox_ivi_ecall_srs_deal(uint8_t dt);

extern void tbox_ivi_ecall_key_deal(uint8_t dt);

extern void tbox_ivi_closesocket();

extern void tbox_ivi_push_fota_informHU(uint8_t flag);

extern uint8_t tbox_ivi_get_link_fault(uint64_t *timestamp);

extern uint8_t tbox_ivi_signal_type(void);

extern uint8_t tbox_ivi_signal_power(void);

#endif

