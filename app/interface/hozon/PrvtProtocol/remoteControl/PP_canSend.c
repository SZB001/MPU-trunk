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
static uint64_t ID440_data;
static uint64_t ID445_data;
static uint64_t ID526_data;
static uint8_t can_data[8];

static uint64_t lastsendtime;

int shell_can_init()
{	
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
	//初始化445报文数据
	ID445_data |= (uint64_t)1 << 46;
	ID445_data |= (uint64_t)1 << 54;
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

   		// log_o(LOG_HOZON,
  			//"############### send virtual on to mcu:%u #################", on);

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
void PP_can_unpack(uint64_t data,uint8_t *dt)
{
	int i;
	memset(dt,0,8*sizeof(uint8_t));
	for(i=7;i>=0;i--)
	{
		dt[i] = (uint8_t) (data >> (i*8));
	}

}
/***************************************************************************
函数名：PP_canSend_setbit       功能：将8个字节中某一位置位
id    ：报文ID
bit   ：起始的bit位
bitl  ：占几个bit
data  ：具体的数据
*dt   ：此参数用于发送3D2报文的发送，其余ID的包文，此参数填NULL
****************************************************************************/
void PP_canSend_setbit(unsigned int id,uint8_t bit,uint8_t bitl,uint8_t data,uint8_t *dt)
{
	//int i;
	if(id == CAN_ID_440)
	{
		ID440_data &= ~((uint64_t)((1<<bitl)-1) << (bit-bitl+1)) ; //再移位
		ID440_data |= (uint64_t)data << (bit-bitl+1);      //置位
		PP_send_virtual_on_to_mcu(1);
		PP_can_unpack(ID440_data,can_data);
		#if 0
		for(i=0;i<8;i++)
		{
			log_o(LOG_HOZON,"ID440_data[%d] = %d",i,can_data[i]);
		}
		#endif
		PP_send_cycle_ID440_to_mcu(can_data);
	}
	else if(id == CAN_ID_445)
	{
		PP_send_virtual_on_to_mcu(1);
		ID445_data &=  ~((uint64_t)((1<<bitl)-1) << (bit-bitl+1)) ; //再移位
		ID445_data |= (uint64_t)data << (bit-bitl+1);      //置位
		PP_can_unpack(ID445_data,can_data);
		#if 0
		for(i=0;i<8;i++)
		{
			log_o(LOG_HOZON,"ID445_data[%d] = %d",i,can_data[i]);
		}
		#endif
		PP_send_cycle_ID445_to_mcu(can_data);
	}
	else if(id == CAN_ID_526)
	{
		PP_send_virtual_on_to_mcu(1);
		ID526_data &=  ~((uint64_t)((1<<bitl)-1) << (bit-bitl+1)) ; //再移位
		ID526_data |= (uint64_t)data << (bit-bitl+1);      //置位
		PP_can_unpack(ID526_data,can_data);
		PP_send_cycle_ID526_to_mcu(can_data);
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
/***************************************************
		广播440 445报文
****************************************************/
void PP_can_send_cycle(void)
{
	if(tm_get_time() - lastsendtime > 50)    //50ms
	{
		PP_can_unpack(ID440_data,can_data);
		PP_send_cycle_ID440_to_mcu(can_data);
		PP_can_unpack(ID445_data,can_data);
		PP_send_cycle_ID445_to_mcu(can_data);
		lastsendtime = tm_get_time();
	}	
}
/**************************************
			唤醒MCU
***************************************/
void PP_can_mcu_awaken(void)
{
	int i;
	for(i=0;i<10;i++)
	{
		PP_send_virtual_on_to_mcu(1);
	}
	 log_o(LOG_HOZON,
  			"############### send virtual on to mcu:1 #################");
}

/**************************************
			休眠MCU
***************************************/
void PP_can_mcu_sleep(void)
{
	PP_send_virtual_on_to_mcu(0);
	 log_o(LOG_HOZON,
  			"############### send virtual on to mcu:0 #################");
}

/******************************************************
		远程控制报文发送
*******************************************************/
void PP_can_send_data(int type,uint8_t data,uint8_t para)
{
	switch(type)
	{
		case PP_CAN_DOORLOCK:
			PP_canSend_setbit(CAN_ID_440,17,2,data,NULL);
			break;
		case PP_CAN_SUNROOF:
			PP_canSend_setbit(CAN_ID_440,47,3,data,NULL);
			break;
		case PP_CAN_AUTODOOR:
			PP_canSend_setbit(CAN_ID_440,19,2,data,NULL);
			break;
		case PP_CAN_SEARCH:
			PP_canSend_setbit(CAN_ID_440,17,2,data,NULL);
			break;
		case PP_CAN_ENGINE:
			PP_canSend_setbit(CAN_ID_440,1,2,data,NULL);
			break;
		case PP_CAN_ACCTRL://空调控制
			{
				switch(data)
				{
					case CAN_OPENACC:
						PP_canSend_setbit(CAN_ID_445,1,1,1,NULL);//有效
						PP_canSend_setbit(CAN_ID_445,14,1,1,NULL);//开启
						break;
					case CAN_CLOSEACC:
						PP_canSend_setbit(CAN_ID_445,1,1,0,NULL);//有效
						PP_canSend_setbit(CAN_ID_445,14,1,0,NULL);//开启
						break;
					case CAN_SETACCTEP:
						PP_canSend_setbit(CAN_ID_445,47,6,para,NULL); //设置主驾温度
						PP_canSend_setbit(CAN_ID_445,55,6,para,NULL); //设置副驾温度
						break;
					default:
						break;
				}	
			}
			break;
		case PP_CAN_CHAGER:
			switch(data)
			{
				case CAN_STARTCHAGER:
					PP_canSend_setbit(CAN_ID_440,4,1,0,NULL);
					PP_canSend_setbit(CAN_ID_440,3,2,2,NULL);
				break;
				case CAN_STOPCHAGER:
					PP_canSend_setbit(CAN_ID_440,3,2,1,NULL);
					break;
				case CAN_SETAPPOINT:
					PP_canSend_setbit(CAN_ID_440,4,1,1,NULL);
					break;
				case CAN_CANCELAPPOINT:
					PP_canSend_setbit(CAN_ID_440,4,1,0,NULL);
					break;
				case CAN_CLEANCHARGE:
					PP_canSend_setbit(CAN_ID_440,3,2,0,NULL);
					break;
				default:
					break;	
			}
			break;
		case PP_CAN_FORBID:
			PP_canSend_setbit(CAN_ID_440,31,2,data,NULL);
			break;
		case PP_CAN_SEATHEAT:
			PP_canSend_setbit(CAN_ID_445,para,3,data,NULL);
			break;
		case PP_CAN_OTAREQ:
			PP_canSend_setbit(CAN_ID_440,39,2,data,NULL);
			break;
		case PP_CAN_CERTIFICATE:
			PP_canSend_setbit(CAN_ID_440,23,1,data,NULL);
		default:
			break;

	}
}

/*****************************************************
		远程控制认证报文发送
*****************************************************/
void PP_can_send_identificat(uint8_t type,uint8_t *dt)
{
	switch(type)
	{
		case PP_CAN_RANDOM:
			PP_canSend_setbit(CAN_ID_3D2,0,0,0,dt);
			break;
		case PP_CAN_XTEAENCIPHER:
			PP_canSend_setbit(CAN_ID_3D2,0,0,0,dt);
			break;
		default:
			break;	
	}
}