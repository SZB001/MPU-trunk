/******************************************************
文件名：	PrvtProt_cfg.c

描述：	企业私有协议（浙江合众）	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include "../PrvtProt_SigParse.h"
#include "PPrmtCtrl_cfg.h"



/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/


/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/

/******************************************************
description： function code
******************************************************/

/******************************************************
*函数名：PP_rmtCtrl_cfg_AuthStatus
*形  参：
*返回值：int
*描  述：   返回认证状态
*备  注：
******************************************************/
unsigned char PP_rmtCtrl_cfg_AuthStatus(void)
{
	return PrvtProt_SignParse_autheSt();
}
