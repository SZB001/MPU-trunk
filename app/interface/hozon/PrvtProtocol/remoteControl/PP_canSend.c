/**********************************************
			鍚堜紬杩滄帶can鎶ユ枃鍙戦��

ID 锛�0x3D2   TBOX涓诲姩璁よ瘉璇锋眰
ID 锛�0x440  
ID : 0x445  杩滅▼鎺у埗鍛戒护鎶ユ枃
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


void hozon_can_440_send(void)
{
#if 0
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
#endif 

	PP_canSend_setbit(CAN_ID_440,17,2,2,NULL);
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
	canmsg_3D2.typ = PP_CAN_TYP_EVENT;  //浜嬩欢鎶ユ枃
	canmsg_3D2.len =8;
	canmsg_3D2.id = CAN_ID_3D2;
	canmsg_3D2.port = 1;
	canmsg_3D2.period = 100;  //100ms
	canmsg_3D2.times_event = 1;
	return 0;
}

/*************************************************
	MPU鍙戦�佽櫄鎷無n绾垮敜閱扢CU
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

PP_send_event_info_to_mcu 鐢ㄤ簬鍙戦�佷簨浠舵�ф姤鏂�3D2

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
PP_send_cycle_info_to_mcu 鐢ㄤ簬鍙戦�佸懆鏈熸�ф姤鏂� 440 
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
PP_send_cycle_info_to_mcu 鐢ㄤ簬鍙戦�佸懆鏈熸�ф姤鏂� 445 
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
PP_send_cycle_info_to_mcu 鐢ㄤ簬鍙戦�佸懆鏈熸�ф姤鏂� 526
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
	log_o(LOG_HOZON,"PP_can_unpack");
	memset(dt,0,8*sizeof(uint8_t));
	for(i=7;i>=0;i--)
	{
		dt[i] = (uint8_t) (data >> (i*8));
		log_o(LOG_HOZON,"dt[%d]=%d",i,dt[i]);
	}

}
/***************************************************************************
鍑芥暟鍚嶏細PP_canSend_setbit       鍔熻兘锛氬皢8涓瓧鑺備腑鏌愪竴浣嶇疆浣�
id    锛氭姤鏂嘔D
bit   锛氳捣濮嬬殑bit浣�
bitl  锛氬崰鍑犱釜bit
data  锛氬叿浣撶殑鏁版嵁
*dt   锛氭鍙傛暟鐢ㄤ簬鍙戦��3D2鎶ユ枃鐨勫彂閫侊紝鍏朵綑ID鐨勫寘鏂囷紝姝ゅ弬鏁板～NULL
****************************************************************************/
void PP_canSend_setbit(unsigned int id,uint8_t bit,uint8_t bitl,uint8_t data,uint8_t *dt)
{
	int i;
	if(id == CAN_ID_440)
	{
		ID440_data &= ~(uint64_t)(((1<<bitl)-1) << (bit-1)) ; //鍐嶇Щ浣�
		ID440_data |= (uint64_t)data << (bit-1);      //缃綅
		
		PP_can_unpack(ID440_data,can_data);
		for(i=0;i<8;i++)
		{
			log_o(LOG_HOZON,"ID440_data[%d] = %d",i,can_data[i]);
		}
		PP_send_cycle_ID440_to_mcu(can_data);
	}
	else if(id == CAN_ID_445)
	{
		ID445_data &= ~(((1<<bitl)-1) << (bit-1)) ; //鍐嶇Щ浣�
		ID445_data |= (uint64_t)data << (bit-1);      //缃綅
		PP_can_unpack(ID445_data,can_data);
		PP_send_cycle_ID445_to_mcu(can_data);
	}
	else if(id == CAN_ID_526)
	{
		ID526_data &= ~(((1<<bitl)-1) << (bit-1)) ; //鍐嶇Щ浣�
		ID526_data |= (uint64_t)data << (bit-1);      //缃綅
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
#if 0
/***************************************************************************
PP_canSend_resetbit       鍔熻兘锛氬皢8涓瓧鑺備腑鏌愪竴浣嶇疆浣�
id    锛氭姤鏂嘔D
bit   锛氳捣濮嬬殑bit浣�
bitl  锛氬崰鍑犱釜bit
****************************************************************************/
void PP_canSend_resetbit(unsigned int id,int bit,int bitl)
{
	int i;
	if(id == CAN_ID_440)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID440_data[bit/8] &= ~(1 << (bit/8 - i));
		}	
		PP_send_cycle_ID440_to_mcu(ID440_data);
	}
	else if (id == CAN_ID_445)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID445_data[bit/8] &= ~(1 << (bit/8 - i));
		}
		PP_send_cycle_ID445_to_mcu(ID445_data);
	}
	else if(id == CAN_ID_526)
	{
		for(i = 0 ; i < bitl ; i++)
		{
			ID526_data[bit/8] &= ~(1 << (bit/8 - i));
		}
		PP_send_cycle_ID526_to_mcu(ID526_data);
	}
	else  //3D2
	{
		
	}
}
#endif
void PP_can_send_data(int type,uint8_t data,uint8_t para)
{
	switch(type)
	{
		case PP_CAN_DOORLOCK:
			PP_canSend_setbit(CAN_ID_440,17,2,data,NULL);
			log_o(LOG_HOZON,"data = %d",data);
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

			PP_canSend_setbit(CAN_ID_440,data,1,para,NULL);
			break;
		case PP_CAN_ACCTRL:
			break;
		case PP_CAN_CHAGER:
			PP_canSend_setbit(CAN_ID_440,3,1,data,NULL);
			break;
		case PP_CAN_FORBID:
			break;
		case PP_CAN_SEATHEAT:
			PP_canSend_setbit(CAN_ID_440,para,2,data,NULL);
			break;
		default:
			break;

	}
}
#if 0
void PP_can_clear_data(int type)
{
	switch(type)
	{
		case PP_CAN_DOORLOCK:
			PP_canSend_resetbit(CAN_ID_440,17,2);
			break;
		case PP_CAN_SUNROOF:
			PP_canSend_resetbit(CAN_ID_440,47,3);
			break;
		case PP_CAN_AUTODOOR:
			PP_canSend_resetbit(CAN_ID_440,17,2);
			break;
		case PP_CAN_SEARCH:
			break;
		case PP_CAN_ENGINE:
			break;
		case PP_CAN_ACCTRL:
			break;
		case PP_CAN_CHAGER:
			break;
		case PP_CAN_FORBID:
			break;
		case PP_CAN_SEATHEAT:
			break;
		default:
			break;

	}
}
#endif

