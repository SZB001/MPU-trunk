/******************************************************
文件名：	
描述：	合众tsp对接socket链路的建立、断开、收/发数据处理	
Data			Vasion			author
2019/4/17		V1.0			liujian
*******************************************************/

/*******************************************************
description： include the header file
*******************************************************/
#include "sockproxy_rxdata.h"

/*******************************************************
description： function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static sockProxyObj_t SPObj[SP_MAX];

/*******************************************************
description： global variable definitions
*******************************************************/

/*******************************************************
description： static variable definitions
*******************************************************/
static void ClrSockproxyData_Queue(void);

/******************************************************
description： function code
******************************************************/

/******************************************************
*函数名：SockproxyData_Init
*形  参：
*返回值：
*描  述：
*备  注：
******************************************************/
void SockproxyData_Init(void)
{
	ClrSockproxyData_Queue();//清缓存队列
}


/******************************************************
*函数名：WrSockproxyData_Queue
*形  参：
*返回值：
*描  述：写数据到数据队列
*备  注：
******************************************************/
int WrSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len)
{
	int Lng;
	
	if(len > SP_DATA_LNG) return -1;
	
	for(Lng = 0U;Lng< len;Lng++)
	{
		SPObj[obj].SPCache[SPObj[obj].HeadLabel].data[Lng] = data[Lng];
	}
	SPObj[obj].SPCache[SPObj[obj].HeadLabel].len = len;
	SPObj[obj].SPCache[SPObj[obj].HeadLabel].NonEmptyFlg = 1U;/*置非空标志*/
	SPObj[obj].HeadLabel++;
	if(SP_QUEUE_LNG == SPObj[obj].HeadLabel)
	{
		SPObj[obj].HeadLabel = 0U;
	}
	
	return 0;
}


/******************************************************
*函数名：RdSockproxyData_Queue
*形  参：
*返回值：
*描  述：读取数据
*备  注：
******************************************************/
int RdSockproxyData_Queue(unsigned char  obj,unsigned char* data,int len)
{	
	int Lng;
	if(0U == SPObj[obj].SPCache[SPObj[obj].TialLabel].NonEmptyFlg) 
	{
		return 0;//无数据
	}
		
	if(SPObj[obj].SPCache[SPObj[obj].TialLabel].len > len) 
	{
		return -1;//溢出错误
	}

	for(Lng = 0U;Lng < SPObj[obj].SPCache[SPObj[obj].TialLabel].len;Lng++)
	{
		data[Lng] =SPObj[obj].SPCache[SPObj[obj].TialLabel].data[Lng];
	}
	SPObj[obj].SPCache[SPObj[obj].TialLabel].NonEmptyFlg = 0U;	/*清非空标志*/
		
	SPObj[obj].TialLabel++;
	if(SP_QUEUE_LNG == SPObj[obj].TialLabel)
	{
		SPObj[obj].TialLabel = 0U;
	}
	return Lng;
}

/******************************************************
*函数名：ClrUnlockLogCache_Queue
*形  参：
*返回值：
*描  述：清数据队列
*备  注：
******************************************************/
static void ClrSockproxyData_Queue(void) 
{
	unsigned char  i,j;

	for(i = 0U;i < SP_MAX;i++)
	{
		for(j = 0U;j < SP_QUEUE_LNG;j++)
		{
			SPObj[i].SPCache[j].NonEmptyFlg = 0U;
		}
		SPObj[i].HeadLabel = 0U;
		SPObj[i].TialLabel = 0U;
	}
}
