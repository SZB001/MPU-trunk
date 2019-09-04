/******************************************************
鏂囦欢鍚嶏細		PP_identificat.h

鎻忚堪锛�	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/
#ifndef		_PP_IDENTIFICAT_H
#define		_PP_IDENTIFICAT_H
/*******************************************************
description锛� include the header file
*******************************************************/

#define PP_AUTH_FAIL		(-1)
#define PP_AUTH_SUCCESS		(1)


typedef enum
{
	PP_stage_idle = 0,
	PP_stage_start,
	PP_stage2,
	PP_stage_waitrandom,
	PP_stage_sendenptdata,
	PP_stage_waitauthokst
} PP_STAGE_TYPE;

extern int PP_get_identificat_flag(void);
extern int PP_identificat_mainfunction(void);
extern void PP_identificat_init(void);
#endif

