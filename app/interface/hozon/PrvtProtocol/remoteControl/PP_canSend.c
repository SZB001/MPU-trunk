/**********************************************
			合众远控can报文发送

ID ：0x3D2   TBOX主动认证请求
ID ：0x440  
ID : 0x445  远程控制命令报文
**********************************************/

#include <stdio.h>
#include "can_api.h"
#include "scom_msg_def.h"
#include "../../../../base/scom/scom_tl.h"
#include "log.h"
#include "PP_canSend.h"

static PP_can_msg_info_t canmsg_3D2;
static uint8_t ID440_data[8];
static uint8_t ID445_data[8];
static uint8_t ID526_data[8];


void hozon_can_440_send(void)
{
	PP_can_msg_info_t caninfo;
	caninfo.data[0] = 0x00;
	caninfo.data[1] = 0x11;
	caninfo.data[2] = 0x22;
	caninfo.data[3] = 0x33;
	caninfo.data[4] = 0x44;
	caninfo.data[5] = 0x55;
	caninfo.data[6] = 0x66;
	caninfo.data[7] = 0x77;
	int len = 0;
	unsigned char buf[64] = {0};
    memcpy(buf + len, caninfo.data, 8*sizeof(uint8_t));
    len += 8*sizeof(uint8_t);
	scom_tl_send_frame(SCOM_MPU_MCU_0x440, SCOM_TL_SINGLE_FRAME, 0, buf, len);
	log_o(LOG_HOZON,"440 send");
	
}

void hozon_can_on_send(void)
{
	int len = 1;
	unsigned char buf[1] ;
	buf[0] = 1;

	scom_tl_send_frame(SCOM_MPU_MCU_VIRTUAL_ON, SCOM_TL_SINGLE_FRAME, 0, buf, len);
	log_o(LOG_HOZON,"440 on");
	
}

int shell_can_init()
{
	shell_cmd_register("hozon_can_440", hozon_can_440_send, "set HOZON PrvtProt remote control data");
	shell_cmd_register("hozon_can_on", hozon_can_on_send, "set HOZON PrvtProt remote control data");
	return 0;
}
int PP_canSend_init(void)
{
	shell_can_init();
	memset(&canmsg_3D2,0,sizeof(PP_can_msg_info_t));
	canmsg_3D2.typ = PP_CAN_TYP_EVENT;  //事件报文
	canmsg_3D2.len =8;
	canmsg_3D2.id = CAN_ID_3D2;
	canmsg_3D2.port = 1;
	canmsg_3D2.period = 100;  //100ms
	canmsg_3D2.times_event = 1;
	return 0;
}

/*************************************************
	MPU发送虚拟on线唤醒MCU
**************************************************/
int PP_send_virtual_on_to_mcu(unsigned char on)
{
    int len = 0;
    unsigned char buf[64];
	
    if (on > 1)
    {
        log_e(LOG_HOZON, "par error");
    }

    buf[len++] = on;

    if (scom_tl_send_frame(SCOM_MPU_MCU_VIRTUAL_ON, SCOM_TL_SINGLE_FRAME, 0, buf, len))
    {
        log_e(LOG_HOZON, "Fail to send msg to MCU");
        return -2;
    }

    log_o(LOG_HOZON,
          "############### send virtual on to mcu:%u #################", on);

    return 0;
}


/***********************************************
PP_send_event_info_to_mcu 用于发送事件性报文3D2

************************************************/

int PP_send_event_info_to_mcu(PP_can_msg_info_t *caninfo)
{
    int len = 0;

    if (NULL == caninfo)
    {
        log_e(LOG_HOZON, "caninfo pointer is NULL");
        return -1;
    }
    if (caninfo->typ > PP_CAN_TYP_MAX || caninfo->len > 8 || caninfo->port > 4)
    {
        log_e(LOG_HOZON, "caninfo parameter error");
        return -1;
    }
    unsigned char buf[64];
    buf[len++] = caninfo->typ;
    memcpy(buf + len, &caninfo->id, sizeof(caninfo->id));
    len += sizeof(caninfo->id);
    buf[len++] = caninfo->port;
    buf[len++] = caninfo->len;
    memcpy(buf + len, caninfo->data, sizeof(caninfo->data));
    len += sizeof(caninfo->data);
    buf[len++] = caninfo->times_event;
    memcpy(buf + len, &caninfo->period, sizeof(caninfo->period));
    len += sizeof(caninfo->period);
	log_o(LOG_HOZON,"3D2 is sending");
	if (scom_tl_send_frame(SCOM_TL_CMD_CTRL, SCOM_TL_SINGLE_FRAME, 0, buf, len))
	{
	   log_e(LOG_HOZON, "Fail to send msg to MCU");
	   return -2;
	}
    return 0;
}

/********************************************************
PP_send_cycle_info_to_mcu 用于发送周期性报文 440 
********************************************************/
int PP_send_cycle_ID440_to_mcu(uint8_t *dt)
{
	int len = 0;
	unsigned char buf[64];
	memcpy(buf + len, dt, 8*sizeof(uint8_t));
    len += 8*sizeof(uint8_t);
	if (scom_tl_send_frame(SCOM_MPU_MCU_0x440, SCOM_TL_SINGLE_FRAME, 0, buf, len))
	{
	   log_e(LOG_HOZON, "Fail to send msg to MCU");
	   return -2;
	}
    return 0;
}

/********************************************************
PP_send_cycle_info_to_mcu 用于发送周期性报文 445 
********************************************************/
int PP_send_cycle_ID445_to_mcu(uint8_t *dt)
{
	int len = 0;
	unsigned char buf[64];
	memcpy(buf + len, dt, 8*sizeof(uint8_t));
    len += 8*sizeof(uint8_t);
	if (scom_tl_send_frame(SCOM_MPU_MCU_0x445, SCOM_TL_SINGLE_FRAME, 0, buf, len))
	{
	   log_e(LOG_HOZON, "Fail to send msg to MCU");
	   return -2;
	}
    return 0;
}
/********************************************************
PP_send_cycle_info_to_mcu 用于发送周期性报文 526
********************************************************/
int PP_send_cycle_ID526_to_mcu(uint8_t *dt)
{
	int len = 0;
	unsigned char buf[64];
	memcpy(buf + len, dt, 8*sizeof(uint8_t));
    len += 8*sizeof(uint8_t);
	if (scom_tl_send_frame(SOCM_MCU_MPU_0x526, SCOM_TL_SINGLE_FRAME, 0, buf, len))
	{
	   log_e(LOG_HOZON, "Fail to send msg to MCU");
	   return -2;
	}
    return 0;
}
/***************************************************************************
函数名：PP_canSend_setbit       功能：将8个字节中某一位置位
id    ：报文ID
bit   ：起始的bit位
bitl  ：占几个bit
data  ：具体的数据
*dt   ：此参数用于发送3D2报文的发送，其余ID的包文，此参数填NULL
****************************************************************************/
void PP_canSend_setbit(unsigned int id,int bit,int bitl,int data,uint8_t *dt)
{
	if(id == CAN_ID_440)
	{
		ID440_data[bit/8] |= ((uint8_t)data)<<(bit%8 - bitl + 1);
		PP_send_cycle_ID440_to_mcu(ID440_data);
	}
	else if(id == CAN_ID_445)
	{
		ID445_data[bit/8] |= ((uint8_t)data)<<(bit%8 - bitl + 1);
		PP_send_cycle_ID445_to_mcu(ID445_data);
	}
	else if(id == CAN_ID_526)
	{
		ID526_data[bit/8] |= ((uint8_t)data)<<(bit%8 - bitl + 1);
		PP_send_cycle_ID526_to_mcu(ID526_data);
	}
	else
	{
		int i;
		if(dt == NULL)
		{
			memset(canmsg_3D2.data,0,8*sizeof(uint8_t));
			log_o(LOG_HOZON,"3D2 is packing");
		}
		else
		{
			for(i=0;i<8;i++)
			{
				canmsg_3D2.data[i] = dt[i];
			}
		}
		PP_send_event_info_to_mcu(&canmsg_3D2);
	}
}

/***************************************************************************
PP_canSend_resetbit       功能：将8个字节中某一位置位
id    ：报文ID
bit   ：起始的bit位
bitl  ：占几个bit
****************************************************************************/
void PP_canSend_resetbit(unsigned int id,int bit,int bitl)
{
	int i;
	if(id == CAN_ID_440)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID440_data[bit/8] &= 0xFE << (bit/8 - i);
		}	
		PP_send_cycle_ID440_to_mcu(ID440_data);
	}
	else if (id == CAN_ID_445)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID445_data[bit/8] &= 0xFE << (bit/8 - i);
		}
		PP_send_cycle_ID445_to_mcu(ID445_data);
	}
	else if(id == CAN_ID_526)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID526_data[bit/8] &= 0xFE << (bit/8 - i);
		}
		PP_send_cycle_ID526_to_mcu(ID526_data);
	}
	else  //3D2
	{
		
	}
}




