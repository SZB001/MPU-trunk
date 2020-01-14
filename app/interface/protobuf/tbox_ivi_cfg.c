
#include <string.h>
#include "tbox_ivi_api.h"
#include <time.h>
#include <sys/time.h>

extern ivi_callrequest callrequest; 
extern int nm_get_signal(void);
extern int nm_get_net_type(void);
extern int at_get_sim_status(void);


uint8_t tbox_ivi_get_call_type(void) //获取通话的类型
{
	return callrequest.call_type;
}
uint8_t tbox_ivi_get_call_action(void) //获取通话的类型
{

	return callrequest.call_action;
}

void tbox_ivi_clear_call_flag(void)
{
	memset(&callrequest,0 ,sizeof(ivi_callrequest));
}
void tbox_ivi_clear_bcall_flag(void)
{
	//callrequest.bcall = 0;
}

void tbox_ivi_clear_icall_flag(void)
{
	//callrequest.icall = 0;
}

long tbox_ivi_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return (long)(timestamp.tv_sec);
}

uint8_t tbox_ivi_signal_type(void)
{
	int signal_type = 0;
	int temp = 0;
	temp = nm_get_net_type();
	if( at_get_sim_status() == 2 )
    {
        signal_type = 0;
    }
    else
    {
         if(temp == 0)
         {
             signal_type = 1;
         }
         else if(temp == 2)
         {
             signal_type= 2;
          }
          else if(temp == 7)
          {
              signal_type = 3;
          }
          else
          {
              signal_type = 0xfe;
          }
     }	
	return signal_type;
}

uint8_t tbox_ivi_signal_power(void)
{
	uint8_t signal_power = 0;
	int temp = 0;
	temp = ((double) nm_get_signal())/31*100;
	if((temp >= 0) && (temp < 20))
	{
		signal_power = 0;
	}
	else if((temp >= 20) && (temp < 40))
	{
		signal_power = 1;
	}
	else if((temp >= 40) && (temp < 60))
	{
		signal_power = 2;
	}
	else if((temp >= 60) && (temp < 80))
	{
		signal_power = 3;
	}
	else
	{
		signal_power = 4;
	}
	return signal_power;
}

