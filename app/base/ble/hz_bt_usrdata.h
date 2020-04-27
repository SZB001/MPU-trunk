#ifndef NGCP_PB_USR_H_
#define NGCP_PB_USR_H_

#include "hz_bt.pb-c.h"
#include "can_api.h"

#define NGCP_MAX_WIRELESS    1000
#define NGCP_RTDATA_MAXSZ    (1)
#define COM_APP_PEM_ROOT_DIR            "/usrdata/pem/HozonCA.cer"
#define COM_APP_PEM_TPONE_DIR            "/usrdata/pem/PhoneCA.cer"

#define BT_BYTE_SZ_8       8
#define BT_BYTE_SZ_16      16
#define BT_BYTE_SZ_32      32
#define BT_BYTE_SZ_64      64
#define BT_BYTE_SZ_128     128
#define BT_BYTE_SZ_256     256
#define BT_BYTE_SZ_1024    1024
#define BT_BYTE_SZ_VIN     17
#define BT_BYTE_SZ_ICCID   20

#define BT_UNAUTH               0
#define BT_AUTH_SUCCESS         1
#define BT_AUTH_FAIL            2

#define HZ_P_NO_ENCY			0
#define HZ_P_ENCY 				1


#define HZ_P_HEAD 				"#START*"
#define HZ_P_END 				"#END*"

/*************ApplicationHeader*********/

#define PROTOCOL_VER			0x01      
//msg_type
enum
{
   
    BT_AH_MSG_TYPE_VEH_SE_FUN  		= 1,
    BT_AH_MSG_TYPE_SE_FUN_RESP  	= 2, //tbox auth respond
    BT_MSG_TYPE_VEH_DOOR  			= 3,
    BT_MSG_TYPE_PAN_SUNROOf  		= 4,
    BT_MSG_TYPE_ELEC_DOOR  			= 5,
    BT_MSG_TYPE_REMOTE_FIND_CAR 	= 6,
    BT_MSG_TYPE_CHARGE 				= 7,
    BT_VAH_MSG_TYPE_POWER_CONTRO 	= 8,
    BT_AH_MSG_TYPE_ACK 				= 9,  //tbox cmd respond
    BT_AH_MSG_TYPE_MAX
};

/*****************ACK******************/
//msg_type
enum
{
    BT_ACK_MSG_TYPE_VEH_DOOR 			= 1,
	BT_ACK_MSG_TYPE_PAN_SUNROOf 		= 2,
	BT_ACK_MSG_TYPE_ELEC_DOOR 			= 3,
	BT_ACK_MSG_TYPE_REMOTE_FIND_CAR 	= 4,
	BT_ACK_MSG_TYPE_CHARGE 				= 5,
    BT_ACK_MSG_TYPE_POWER_CONTRO 		= 6,
    BT_ACK_MSG_TYPE_MAX
};


//TODO ngcp fn
typedef enum
{
    USRDATA_GENLACK = 0,
    USRDATA_LOGIN,
    USRDATA_LOGINACK,
    USRDATA_HEART,
    USRDATA_RTDATA,
    USRDATA_DBCREQ,
    USRDATA_DBCRES,
    USRDATA_DBCRESULT,
    USRDATA_RMTCTRL,
} usrdata_type_t;

typedef struct
{
    uint8_t src;
    uint8_t year;
    uint8_t mon;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    int32_t msec;
} data_time_t;

typedef struct
{
    data_time_t time;
    int32_t sn;
} vehi_logout_t;

typedef struct
{
    uint8_t msgid[2];
    uint8_t result;
    size_t n_dbccheck;
    int dbccheck;
} vehi_login_ack_t;



typedef struct
{
	int year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} valid_time_t;

typedef struct
{
	uint8_t auth_result;
	size_t failure_reasons; 
	int verdata_len;
	uint8_t verification_data[BT_BYTE_SZ_1024];
} bt_auth_respond_t;

typedef struct
{
	size_t msg_type;
	size_t state;
	valid_time_t time;
} bt_control_cmd_t;

typedef struct
{
	unsigned char sekey[512];
	unsigned short sekey_len;
} bt_info_t;

typedef struct
{
	int32_t msg_type;
	int32_t state;
	int32_t execution_result;
	int32_t failure_reasons;
} bt_ack_t;
typedef struct
{
	int32_t ldoor1_state;
	int32_t ldoor2_state;
	int32_t rdoor1_state;
	int32_t rdoor2_state;
} bt_vihe_door_t;

typedef struct
{
	int32_t vehicle_door_state;
  	int32_t sunroof_state;
  	int32_t electric_door_state;
  	int32_t fine_car_state;
  	int32_t power_control_state;
  	int32_t remaining_capacity;
  	int32_t remaining_mileage;
	int32_t total_mileage;
	int32_t car_gear_position;
} bt_vihe_info_t;

typedef struct
{
	int32_t charge_state;
	int32_t charge_reservation;
	int32_t reservation_hour;
	int32_t reservation_minute;
	int32_t remaining_charge_hour;
	int32_t remaining_charge_minute;
	float battery_temperature;
} bt_vihe_charge_t;


typedef struct
{
	int32_t vehicle_air_state;
	float air_temperature;
	int32_t air_conditioning_mode;
	int32_t air_condition_reservation;
	int32_t vehicle_mainseat_state;
	int32_t vehicle_secondseat_state;
	int32_t reservation_hour1;
	int32_t reservation_minute1;
	int32_t reservation_hour2;
	int32_t reservation_minute2;
	int32_t reservation_hour3;
	int32_t reservation_minute3;
	float vehicle_temperature;
    float outside_temperature;
    int32_t airwindshield;
} bt_vihe_air_t;
typedef struct
{
	float ltire_pressure1;
	float ltire_temp1;
	float ltire_pressure2;
	float ltire_temp2;
	float rtire_pressure1;
	float rtire_temp1;
	float rtire_pressure2;
	float rtire_temp2;
} bt_vihe_tire_t;

typedef struct
{
	bt_vihe_info_t 	vihe_info;
	bt_vihe_air_t 	vihe_air;
	bt_vihe_charge_t vihe_charge;
	bt_vihe_door_t vihe_door;
	bt_vihe_tire_t vihe_tire; 
} bt_info_state_t;


typedef struct
{
	uint8_t protocol_version; 
	uint8_t msg_type;
	valid_time_t Timestamp; 
	size_t MsgCarrierLen;
	union
    {
		uint8_t MsgCarrier[BT_BYTE_SZ_1024];
		
		/* auth */
        struct
        {
            uint8_t  authentication_type;     
			uint8_t  bt_addr[6];   
			uint8_t  bt_name[30];     
			uint8_t  imsi[25];   
            uint8_t  data1[BT_BYTE_SZ_1024 - 62];     /* data area */
        }__attribute__((packed));

		/* CMD */
        struct
        {
            uint8_t  auth_result;   
			uint8_t  failure_reasons;      
            uint8_t  ver_data2[BT_BYTE_SZ_1024 -2];     /* data area */
        }__attribute__((packed));
	};	
} bt_recv_t;

typedef struct
{
	uint8_t protocol_version; 
	uint8_t msg_type;
	valid_time_t Timestamp; 
    uint8_t  auth_result;   
	uint8_t  failure_reasons;  
	int  ver_data1_len; 
    uint8_t  ver_data1[BT_BYTE_SZ_1024 -6];     /* data area */
	bt_ack_t ack;
	bt_vihe_info_t 		vehi_info;
	bt_vihe_charge_t 	vehi_charge;
	bt_vihe_air_t 		vehi_air;
	bt_vihe_door_t 		vehi_door;
	bt_vihe_tire_t 		vehi_tire;
} bt_send_t;

typedef struct
{
	uint8_t 		ency;
	int 			msg_cnt;
	uint8_t 		sekey[256];
	int 			se_len;
	int 			type;  //O:AHTH 1 :CMD
	bt_send_t       hz_send;
} hz_protocol_t;


extern hz_protocol_t g_hz_protocol;

/* public interface */
extern int hz_pb_bytes_set(ProtobufCBinaryData *des, uint8_t *buf, int len);
extern int pb_bytes_op_set(ProtobufCBinaryData **des, uint8_t *buf, int len);
extern int pb_string_op_set(char **des, char *buf, int len);
extern int pb_boolean_op_set(protobuf_c_boolean **des, size_t *no, int *val, int len);
extern int pb_int32_set(int32_t **des, size_t *no, int32_t *val, int len);
extern int pb_double_set(double **des, size_t *no, double *val, int len);



int pb_authentication_response_set(AuthenticationResponse **des, bt_auth_respond_t *src);

int hz_protocol_process(unsigned char *pucInData,unsigned int *pulInLen,unsigned char *pucOutData,unsigned int *pulOutLen);
int pb_appliheader_set(ApplicationHeader **des, bt_send_t *src);
int pb_TimeStamp_set(TimeStamp **des, bt_send_t *src);
int pb_ack_set(ACK **des, bt_send_t *src);

void reset_hz_data(void);
int bt_send_cmd_pack(bt_ack_t state, bt_info_state_t indata,  uint8_t *out, size_t *out_len);
unsigned char bt_get_auth_flag(void);

int pb_vihe_info_set(VehicleInfor **des, bt_send_t *src);
int pb_vihe_charge_set(VehicleChargeInfor **des, bt_send_t *src);
int pb_vihe_air_set(VehicleAirInfor **des, bt_send_t *src);
int pb_vihe_tire_set(VehicleTireInfor **des, bt_send_t *src);
int pb_vihe_door_set(VehicleLrdoorInfor **des, bt_send_t *src);



#endif /* NGCP_PB_USR_H_ */
