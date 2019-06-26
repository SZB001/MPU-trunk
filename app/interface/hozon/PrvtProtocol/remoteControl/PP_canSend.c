

#include "PP_canSend.h"

#include <stdio.h>
#include "can_api.h"
#include "log.h"

#define RMTCTRL_CAN_PORT 1//can2


//static uint8_t candata[8] = { 0 };

extern int can_do_send(unsigned char port, CAN_SEND_MSG *msg);

static CAN_SEND_MSG msg;

static uint64_t p_time;

void PP_canSend_Mainfunction(void)
{
	if(tm_get_time() - p_time > 20)
	{
		msg.MsgID     = 0x440;   
		msg.DLC       = 8;       
		msg.isEID     = 0;       
		msg.isRTR     = 0;
		can_do_send(RMTCTRL_CAN_PORT,&msg);
		p_time = tm_get_time();
	}
}

void PP_canSend_setbit(int bit,int bitl,int data)
{
	msg.Data[bit/8] |= ((uint8_t)data)<<(bit%8 - bitl + 1);
}

void PP_canSend_resetbit(int bit,int bitl)
{
	int i;
	for(i = 0 ; i < bitl ; i++)
	{
		msg.Data[bit/8] &= 0xFE << (bit/8 - i);
	}	
}






