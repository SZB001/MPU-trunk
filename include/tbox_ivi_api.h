#ifndef __IVI_API_H__
#define __IVI_API_H__

#include <netinet/in.h>
#include "mid_def.h"
#include "init.h"

#define MAX_IVI_NUM                      1
#define IVI_SERVER_PORT                  5757
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

typedef struct
{
    int fd;
    unsigned int lasthearttime;
    struct sockaddr_in addr;
} ivi_client;

typedef struct{
	int ecall;
	int bcall;
	int icall;
	int action;
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

int tbox_ivi_get_call_action(void);

int tbox_ivi_get_call_type(void);

void tbox_ivi_clear_call_flag(void);

extern void tbox_ivi_set_tspInformHU(ivi_remotediagnos *tsp);

extern void tbox_ivi_set_tsplogfile_InformHU(ivi_logfile *tsp);

extern void tbox_ivi_set_tspchager_InformHU(ivi_chargeAppointSt *tsp);
#endif

