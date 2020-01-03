/******************************************************
�ļ�����	PrvtProt_netstatus.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/10/21		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_NET_ST_H
#define		_PRVTPROT_NET_ST_H

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define PP_NETST_TSKTIME            60000
#define PP_NETST_CNT                3

/**********�곣������*********/


/***********�꺯��***********/


/*******************************************************
description�� struct definitions
*******************************************************/



/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
	uint8_t	 faultflag;
    uint8_t  newSt;
    uint8_t  oldSt;
    uint64_t timestamp;
}PP_net_status_t;

/******enum definitions******/

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/

#endif 
