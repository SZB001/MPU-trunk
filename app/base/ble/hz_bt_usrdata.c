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

int pb_vihe_info_set(VehicleInfor **des, bt_send_t *src)
{
	VehicleInfor *pobj = (VehicleInfor*) calloc(1, sizeof(VehicleInfor));
    if (pobj)
    {
		vehicle_infor__init(pobj);
		//pobj->vehiclie_door_state = src->vehi_info.vehiclie_door_state;
		pobj->sunroof_state = src->vehi_info.sunroof_state;
		pobj->electric_door_state = src->vehi_info.electric_door_state;
		pobj->fine_car_state = src->vehi_info.fine_car_state;
		//pobj->charge_state = src->vehi_info.charge_state;
		//pobj->power_state = src->vehi_info.power_state;
        *des = pobj;
    }
	return 0;
}
int pb_vihe_door_set(VehicleLrdoorInfor **des, bt_send_t *src)
{
	VehicleLrdoorInfor *pobj = (VehicleLrdoorInfor*) calloc(1, sizeof(VehicleLrdoorInfor));
    if (pobj)
    {
		vehicle_lrdoor_infor__init(pobj);
        pobj->ldoor1_state = src->vehi_door.ldoor1_state;
        pobj->ldoor2_state = src->vehi_door.ldoor2_state;
		pobj->rdoor1_state= src->vehi_door.rdoor1_state;
		pobj->rdoor2_state = src->vehi_door.rdoor2_state;
        *des = pobj;
    }
	return 0;
}

int pb_vihe_charge_set(VehicleChargeInfor **des, bt_send_t *src)
{
	VehicleChargeInfor *pobj = (VehicleChargeInfor*) calloc(1, sizeof(VehicleChargeInfor));
    if (pobj)
    {
		vehicle_charge_infor__init(pobj);
        pobj->charge_reservation = src->vehi_charge.charge_reservation;
        pobj->charge_state = src->vehi_charge.charge_state;
		pobj->remaining_charge_hour= src->vehi_charge.remaining_charge_hour;
		pobj->remaining_charge_minute = src->vehi_charge.remaining_charge_minute;
		pobj->reservation_hour = src->vehi_charge.reservation_hour;
		pobj->reservation_minute = src->vehi_charge.reservation_minute;
		pobj->battery_temperature = src->vehi_charge.battery_temperature;
        *des = pobj;
    }
	return 0;
}
int pb_vihe_tire_set(VehicleTireInfor **des, bt_send_t *src)
{
	VehicleTireInfor *pobj = (VehicleTireInfor*) calloc(1, sizeof(VehicleTireInfor));
    if (pobj)
    {
		vehicle_tire_infor__init(pobj);
        pobj->ltire_pressure1 = src->vehi_tire.ltire_pressure1;
        pobj->ltire_temp1 = src->vehi_tire.ltire_temp1;
		pobj->ltire_pressure2= src->vehi_tire.ltire_pressure2;
		pobj->ltire_temp2 = src->vehi_tire.ltire_temp2;
		pobj->rtire_pressure1 = src->vehi_tire.rtire_pressure1;
		pobj->rtire_temp1 = src->vehi_tire.rtire_temp1;
		pobj->rtire_pressure2 = src->vehi_tire.rtire_pressure2;
		pobj->rtire_temp2 = src->vehi_tire.rtire_temp2;
        *des = pobj;
    }
	return 0;
}

int pb_vihe_air_set(VehicleAirInfor **des, bt_send_t *src)
{
	VehicleAirInfor *pobj = (VehicleAirInfor*) calloc(1, sizeof(VehicleAirInfor));
    if (pobj)
    {
		vehicle_air_infor__init(pobj);
		pobj->air_conditioning_mode = src->vehi_air.air_conditioning_mode;
		pobj->air_condition_reservation = src->vehi_air.air_condition_reservation;
		pobj->air_temperature = src->vehi_air.air_temperature;
		pobj->reservation_hour1 = src->vehi_air.reservation_hour1;
		pobj->reservation_hour2 = src->vehi_air.reservation_hour2;
		pobj->reservation_hour3 = src->vehi_air.reservation_hour3;
		pobj->reservation_minute1 = src->vehi_air.reservation_minute1;
		pobj->reservation_minute2 = src->vehi_air.reservation_minute2;
		pobj->reservation_minute3 = src->vehi_air.reservation_minute3;
		pobj->vehicle_air_state = src->vehi_air.vehicle_air_state;
		pobj->vehicle_mainseat_state = src->vehi_air.vehicle_mainseat_state;
		pobj->vehicle_secondseat_state = src->vehi_air.vehicle_secondseat_state;
		pobj->vehicle_temperature = src->vehi_air.vehicle_temperature;
		pobj->outside_temperature = src->vehi_air.outside_temperature;
		pobj->airwindshield = src->vehi_air.airwindshield;
        *des = pobj;
    }
	return 0;
}


