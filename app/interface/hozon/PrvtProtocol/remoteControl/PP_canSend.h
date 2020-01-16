#ifndef		_PP_CANSEND_H
#define		_PP_CANSEND_H

#define CAN_ID_3D2 0x3D2
#define CAN_ID_440 0x440
#define CAN_ID_445 0x445
#define CAN_ID_526 0x526

/***********doorctrl***********/
#define CAN_CLEANDOOR 0
#define CAN_CLOSEDOOR 1
#define CAN_OPENDOOR  2
/**********search**************/
#define CAN_CLEANSEARCH   0
#define CAN_SEARCHVEHICLE 3
/***********sunroofctrl*********/
#define CAN_SUNROOFCLEAN   0
#define CAN_SUNROOFOPEN    1
#define CAN_SUNROOFCLOSE   2
#define CAN_SUNROOFUP      3
#define CAN_SUNROOFSTOP    4
/***********Sunshade************/
#define CAN_SUNSHADECLEAN  0
#define CAN_SUNSHADECLOSE  2
/**********auto****************/
#define CAN_CLEANAUTODOOR 0
#define CAN_CLOSEAUTODOOR 2
#define CAN_OPENAUTODOOR  1
/************seatheat************/
#define CAN_NOREQSEAT      0
#define CAN_CLOSESEATHEAT  1 
#define CAN_SEATHEATFIRST  2
#define CAN_SEATHEATSECOND 3
#define CAN_SEATHEATTHIRD  4
#define CAN_SEATHEATMAIN   28
#define CAN_SEATHEATPASS   31
/**********engine****************/
#define CAN_ENGINECLEAN 0
#define CAN_STARTENGINE 1
#define CAN_CLOSEENGINE 2
/************chager**************/
#define CAN_CLEANCHARGE 0
#define CAN_STOPCHAGER  1
#define CAN_STARTCHAGER 2
#define CAN_SETAPPOINT 3
#define CAN_CANCELAPPOINT 4
/************forbid**************/
#define CAN_FORBIDCLEAN  0
#define CAN_STARTFORBID  1
#define CAN_NOFORBID     2
/************ACC****************/
#define CAN_OPENACC          1
#define CAN_OPNEACCFIAL      2
#define CAN_SETACCTEP        3
#define CAN_CLOSEACC         4
#define CAN_CLOSEACCCLEAN     5
#define CAN_APPOINTACC       6
#define CAN_CANCELAPPOINTACC 7
#define CAN_ACCMD_INVAILD    8

/**********BLUE****************/
#define CAN_BLUECLEAN 0
#define CAN_BLUESTART 1

/*********TBOX SosState************/
#define CAN_SOSOFF   0
#define CAN_SOSON    1

/**********TBOX HVSHUT************/
#define CAN_NOREQUEHV 0
#define CAN_REQUEHV   1

typedef enum
{
    PP_CAN_DOORLOCK = 0,
	PP_CAN_SUNROOF,
	PP_CAN_SUNSHADE,
    PP_CAN_AUTODOOR,
    PP_CAN_SEARCH,
    PP_CAN_ENGINE,
    PP_CAN_ACCTRL,
    PP_CAN_CHAGER,
    PP_CAN_FORBID,
    PP_CAN_SEATHEAT,
    PP_CAN_OTAREQ,
    PP_CAN_CERTIFICATE,
    PP_CAN_BLUESTART,
    PP_CAN_SOS,
    PP_CAN_HV,
    PP_CAN_CTRL_TYP_MAX,
} PP_can_ctrl_typ;

typedef enum
{
	VIRTUAL_WAIT = 0,
	VIRTUAL_SEND,
}PP_CAN_VIRTUAL;

typedef enum
{
	PP_CAN_RANDOM = 0,
	PP_CAN_XTEAENCIPHER,
    PP_CAN_IDENTIFICAT_TYP_MAX,
} PP_can_identificat_typ;

typedef enum
{
    PP_CAN_TYP_EVENT = 1,//事件型报文
    PP_CAN_TYP_MIX,//混合报文
    PP_CAN_TYP_MAX,
} PP_can_typ;

typedef struct
{
    PP_can_typ typ;//报文类型
    unsigned int id;//报文ID
    unsigned char port;//报文发送端口
    unsigned char data[8];//报文类型
    unsigned char len;//报文长度
    unsigned char times_event;//事件报文发送的次数
    unsigned int period;//事件报文发送周期
} PP_can_msg_info_t;


extern int scom_tl_send_frame(unsigned char msg_type, unsigned char frame_type,
                       unsigned short frame_no,
                       unsigned char *data, unsigned int len);

extern int PP_canSend_init(void);

extern void PP_can_send_data(int type,uint8_t data,uint8_t para);

extern void PP_can_clear_data(int type);

extern void PP_can_send_cycle(void);

extern void PP_can_mcu_awaken(void);

extern void PP_can_mcu_sleep(void);

extern void PP_can_send_identificat(uint8_t type,uint8_t *dt);

extern void PP_can_send_mileage(uint8_t *dt);

extern uint8_t PP_get_virtual_flag();


#endif

