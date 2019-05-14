/******************************************************
文件名：	
描述：
Data			  Vasion			author
2019/04/17		   V1.0			    liujian
*******************************************************/
#ifndef		__PRVT_PROT_QUEUE_H
#define		__PRVT_PROT_QUEUE_H
/*******************************************************
description： include the header file
*******************************************************/


/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/


/**********宏常量定义*********/
#define PP_DATA_LNG  1456U/*数据队列中数据长*/
#define PP_QUEUE_LNG  1U/*数据队列长*/

/***********宏函数***********/

/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_XCALL = 0,//
	PP_REMOTE_CFG,
	PP_MAX
}PP_RX_OBJ;

/*****struct definitions*****/
typedef struct
{
	unsigned char  NonEmptyFlg;	/*数据非空标志*/
	int	  len;/*数据长*/
	unsigned char  data[PP_DATA_LNG];/*数据*/
}PPCache_t;/*数据队列结构体*/

typedef struct
{
	unsigned char  HeadLabel;/*头标签*/
	unsigned char  TialLabel;/*尾标签*/
	PPCache_t PPCache[PP_DATA_LNG];
}PPObj_t;/*接收对象结构体*/

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_queue_Init(void);
extern int WrPP_queue(unsigned char  obj,unsigned char* data,int len);
extern int RdPP_queue(unsigned char  obj,unsigned char* data,int len);

#endif
