/****************************************************************
 file:          hz_bt_usrdata.c
 description:   
 date:          2019/5/15
 author:        liuquanfu
 ****************************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include "log.h"
#include "hz_bt.pb-c.h"
#include "hz_bt_usrdata.h"
#define PROTOCOL_VERSION 				0x0
/******************************************************************************
* Function Name  : hz_pb_bytes_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int hz_pb_bytes_set(ProtobufCBinaryData *des, uint8_t *buf, int len)
{
    if (len)
    {
        if ((des->data = (uint8_t*) calloc(1, len)) == NULL)
        {
            return -1;
        }
        memcpy(des->data, buf, len);
        des->len = len;

        return 0;
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_bytes_op_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_bytes_op_set(ProtobufCBinaryData **des, uint8_t *buf, int len)
{
    ProtobufCBinaryData *pobj = (ProtobufCBinaryData*) calloc(1, sizeof(ProtobufCBinaryData));

    if (pobj)
    {
        if (hz_pb_bytes_set(pobj, buf, len))
        {
            free(pobj);
        }
        else
        {
            *des = pobj;
            return 0;
        }
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_string_op_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_string_op_set(char **des, char *buf, int len)
{
    char *pobj = (char*) calloc(1, len + 1);

    if (pobj)
    {
        strcpy(pobj, buf);
        *des = pobj;

        return 0;
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_boolean_op_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_boolean_op_set(protobuf_c_boolean **des, size_t *no, int *val, int len)
{
    protobuf_c_boolean *tmp = (protobuf_c_boolean*) calloc(1, len * sizeof(protobuf_c_boolean));

    if (tmp)
    {
        memcpy(tmp, val, len * sizeof(protobuf_c_boolean));
        *no = len;
        *des = tmp;

        return 0;
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_int32_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_int32_set(int32_t **des, size_t *no, int32_t *val, int len)
{
    int32_t *tmp = (int32_t*) calloc(1, len * sizeof(int32_t));

    if (tmp)
    {
        memcpy(tmp, val, len * sizeof(int32_t));
        *no = len;
        *des = tmp;

        return 0;
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_double_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_double_set(double **des, size_t *no, double *val, int len)
{
    double *tmp = (double*) calloc(1, len * sizeof(double));

    if (tmp)
    {
        memcpy(tmp, val, len * sizeof(double));
        *no = len;
        *des = tmp;

        return 0;
    }

    return -1;
}
/******************************************************************************
* Function Name  : pb_appliheader_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_appliheader_set(ApplicationHeader **des, bt_send_t *src)
{
    ApplicationHeader *pobj = (ApplicationHeader*) calloc(1, sizeof(ApplicationHeader));

    if (pobj)
    {
        application_header__init(pobj);
		pobj->protocol_version = src->protocol_version;
		pobj->msg_type = src->msg_type;

       
        *des = pobj;
    }

    return 0;
}
/******************************************************************************
* Function Name  : pb_TimeStamp_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_TimeStamp_set(TimeStamp **des, bt_send_t *src)
{
    //int res = -1;
    TimeStamp *pobj = (TimeStamp*) calloc(1, sizeof(TimeStamp));

    if (pobj)
    {
        time_stamp__init(pobj);
		
		log_i(LOG_BLE, " src->Timestamp.year=%d \r\n", src->Timestamp.year);
		log_i(LOG_BLE, " src->Timestamp.month=%d \r\n", src->Timestamp.month);
		log_i(LOG_BLE, " src->Timestamp.day=%d \r\n", src->Timestamp.day);
		log_i(LOG_BLE, " src->Timestamp.hour=%d \r\n", src->Timestamp.hour);
		
		log_i(LOG_BLE, " src->Timestamp.second =%d \r\n", src->Timestamp.second );
		
		pobj->year = src->Timestamp.year;
		pobj->month = src->Timestamp.month;
		pobj->day = src->Timestamp.day;
		pobj->hour = src->Timestamp.hour;
		pobj->minute = src->Timestamp.minute;
		pobj->second = src->Timestamp.second;
   
         *des = pobj;
      
    }

    return 0;
}
/******************************************************************************
* Function Name  : pb_ack_set
* Description	 :	
* Input 		 :	
* Return		 : NONE
******************************************************************************/
int pb_ack_set(ACK **des, bt_send_t *src)
{
	ACK *pobj = (ACK*) calloc(1, sizeof(ACK));

    if (pobj)
    {
        ack__init(pobj);
		pobj->ack_state = src->ack.state;
		pobj->msg_type = src->ack.msg_type;
        *des = pobj;
    }

	return 0;
}
