#ifndef		_PP_IDENTIFICAT_H
#define		_PP_IDENTIFICAT_H
/******************************************************
文件名：		PP_identificat.h

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
typedef enum
{
		PP_stage1 = 0,
	
		
		PP_stage2 ,
	
		
		PP_stage3 ,
	
		PP_stage4 ,
		
		PP_stage5 ,
		
} PP_STAGE_TYPE;


extern int PP_get_identificat_flag(void);

extern int PP_identificat_mainfunction(void);


#endif

