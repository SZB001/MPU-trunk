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

typedef enum IVI_MSG_EVENT
{
    IVI_MSG_GPS_EVENT = MPU_MID_IVI,
} IVI_MSG_EVENT;


/* initiaze thread communciation module */
int ivi_init(INIT_PHASE phase);

/* startup thread communciation module */
int ivi_run(void);

#endif

