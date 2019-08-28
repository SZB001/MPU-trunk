
#include <string.h>
#include "tbox_ivi_api.h"
#include <time.h>
extern ivi_callrequest callrequest; 

int tbox_ivi_get_call_type(void) //获取通话的类型
{

	if(callrequest.ecall == 1)
	{
		return 0;  // ecall
	}
	else if (callrequest.bcall == 1)
	{
		return 1;  // bcall
	}
	else if (callrequest.icall == 1)
	{
		return 2;  //icall
	}
	else
	{
		return 3;
	}
	
}
int tbox_ivi_get_call_action(void) //获取通话的类型
{

	if(callrequest.action == 1)
	{
		return 1;  // 打电话
	}
	else if (callrequest.action == 2)
	{
		return 2;  // 挂电话
	}
	return 0;
}

void tbox_ivi_clear_call_flag(void)
{
	memset(&callrequest,0 ,sizeof(ivi_callrequest));
}
void tbox_ivi_clear_bcall_flag(void)
{
	callrequest.bcall = 0;
}

void tbox_ivi_clear_icall_flag(void)
{
	callrequest.icall = 0;
}

long tbox_ivi_getTimestamp(void)
{
	struct timeval timestamp;
	gettimeofday(&timestamp, NULL);
	
	return (long)(timestamp.tv_sec);
}

