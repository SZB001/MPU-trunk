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
#define CAN_SEARCHVEHICLE 3

/***********sunroofctrl*********/
#define CAN_SUNROOFCLEAN   0
#define CAN_SUNROOFOPEN    1
#define CAN_SUNROOFCLOSE   2
#define CAN_SUNROOFUP      3
#define CAN_SUNROOFSTOP    4
/**********auto****************/
#define CAN_CLEANAUTODOOR 0
#define CAN_CLOSEAUTODOOR 2
#define CAN_OPENAUTODOOR  1
/************seatheat************/
#define CAN_CLOSESEATHEAT  0 
#define CAN_SEATHEATFIRST  1
#define CAN_SEATHEATSECOND 2
#define CAN_SEATHEATTHIRD  3
#define CAN_SEATHEATMAIN   28
#define CAN_SEATHEATPASS   30
/**********engine****************/
#define CAN_ENGINECLEAN 0
#define CAN_ENGINEREQ   1
#define CAN_STARTENGINE 0
#define CAN_CLOSEENGINE 1

/************chager**************/
#define CAN_STOPCHAGER 	0
#define CAN_STARTCHAGER 1

typedef enum
{
    PP_CAN_DOORLOCK = 0,
	PP_CAN_SUNROOF,
    PP_CAN_AUTODOOR,
    PP_CAN_SEARCH,
    PP_CAN_ENGINE,
    PP_CAN_ACCTRL,
    PP_CAN_CHAGER,
    PP_CAN_FORBID,
    PP_CAN_SEATHEAT,
    PP_CAN_CTRL_TYP_MAX,
} PP_can_ctrl_typ;


typedef enum
{
    PP_CAN_TYP_EVENT = 1,//浜嬩欢鍨嬫姤鏂�
    PP_CAN_TYP_MIX,//娣峰悎鎶ユ枃
    PP_CAN_TYP_MAX,
} PP_can_typ;

typedef struct
{
    PP_can_typ typ;//鎶ユ枃绫诲瀷
    unsigned int id;//鎶ユ枃ID
    unsigned char port;//鎶ユ枃鍙戦�佺鍙�
    unsigned char data[8];//鎶ユ枃绫诲瀷
    unsigned char len;//鎶ユ枃闀垮害
    unsigned char times_event;//浜嬩欢鎶ユ枃鍙戦�佺殑娆℃暟
    unsigned int period;//浜嬩欢鎶ユ枃鍙戦�佸懆鏈�
} PP_can_msg_info_t;


extern int scom_tl_send_frame(unsigned char msg_type, unsigned char frame_type,
                       unsigned short frame_no,
                       unsigned char *data, unsigned int len);

extern int PP_canSend_init(void);

extern void PP_canSend_setbit(unsigned int id,uint8_t bit,uint8_t bitl,uint8_t data,uint8_t *dt);

extern void PP_canSend_resetbit(unsigned int id,int bit,int bitl);

extern void PP_can_send_data(int type,uint8_t data,uint8_t para);

extern void PP_can_clear_data(int type);
#endif

