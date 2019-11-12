/*******************************************************************************

		头文件 PP_SendWakeUptime.h

******************************************************************************/
#ifndef	_PP_SENDWAKEUPTIME_H
#define	_PP_SENDWAKEUPTIME_H

typedef struct {
	uint8_t period;
	uint8_t hour;
	uint8_t min;
}waketime;

typedef struct
{
	uint8_t  week;
	uint8_t  mask;
} PP_wake_t; /*�ṹ��*/



extern void PP_Send_WakeUpTime_to_Mcu(void);
extern int PP_waketime_to_min(waketime * pt);
extern uint8_t  GetPP_Wake_Sleep();

#endif





