#ifndef		_PP_CANSEND_H
#define		_PP_CANSEND_H

#define CAN_ID_3D2 0x3D2
#define CAN_ID_440 0x440
#define CAN_ID_445 0x445
#define CAN_ID_526 0x526


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

extern void PP_canSend_setbit(unsigned int id,int bit,int bitl,int data,unsigned char *dt);

extern void PP_canSend_resetbit(unsigned int id,int bit,int bitl);


#endif

