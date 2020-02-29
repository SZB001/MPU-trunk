/******************************************************
�ļ�����	CanMsgUL.h

������	

Data			  Vasion			author
2019/2/6		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_CANMSGUL_H
#define		_PRVTPROT_CANMSGUL_H

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/
#define PP_CANMSGUL_MSGNUM_MAX   2000
#define PP_CANMSGUL_PACKTIME	 60000
#define PP_CANMSGUL_BUFNUM       5
#define PP_CANMSGUL_CANBUSNUM    2
#define PP_CANMSGUL_USSLEEPTIME  10

#define PP_CANMSGUL_PATH      "/media/sdcard/CanMsgFileUL/"

#define PP_CANMSGUL_HEADER_DATA    "date Tue Feb 11 11:29:01 am 2020\r\n"
#define PP_CANMSGUL_HEADER_BASETS  "base hex timestamps absolute\r\ninternal events logged\r\n"
#define PP_CANMSGUL_HEADER_VER     "// version 7.0.0\r\n"

#define PP_CANMSGUL_ENDTRIGBOLCK   "End TriggerBlock"
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
    uint64_t miscUptime;
    union
    {
        uint32_t MsgID; /* CAN Message ID (11-bit or 29-bit) */
        struct
        {
            uint32_t msgid : 31;
            uint32_t exten : 1;
        };
    };
    
    unsigned int  len   : 7;   /* data length */
    unsigned int  brs   : 1;   /* bit rate switch */
    unsigned int  esi   : 1;   /* error state indicator */
    unsigned int  canFD : 1;   /* canFD flag */
    unsigned int  isRTR : 1;   /* RTR flag */
    unsigned int  isEID : 1;   /* EID flag */
    unsigned int  port  : 3;   /* can port */
    unsigned int  isRx  : 1;   /* tx or rx flag */ 
    uint8_t  Data[64];
} __attribute__((packed)) CanMsg_t;


typedef struct
{
	uint8_t successflag;
    int cnt;
	CanMsg_t msg[PP_CANMSGUL_MSGNUM_MAX];
}__attribute__((packed))  PP_CanMsgUL_Buf_t;

typedef struct
{
    uint8_t  firstframeflag;
    long long BaseTstamp;
	long long BaseClctime;
    long long BaseStatistictime;
    uint8_t  renameflag;
    uint8_t  index;
	PP_CanMsgUL_Buf_t buffer[PP_CANMSGUL_BUFNUM];
}__attribute__((packed))  PP_CanMsgUL_t;


typedef struct
{
	char Wmon[5];
}PP_CanMsgUL_Mon_t;

typedef struct
{
	char Wday[5];
}PP_CanMsgUL_week_t;

typedef struct
{
	long long fsttime;
    long long lsttime;
    char canP;
    float relT;//relative time
	int Dcnt;//“D” stands for CAN Data Frames 
    int Rcnt;//“R” stands for CAN Remote Frames 
    int XDcnt;//“XD” stands for CAN Extended Data Frames 
    int XRcnt;//“XR” stands for CAN Extended Remote Frames 
    int Ecnt;//“E” stands for Error Frames 
    int Ocnt;//“O” stands for Overload Frames 
    float Bcnt;//“B” stands for Busload
}PP_CanMsgUL_busStatistic_t;

/******enum definitions******/

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void InitPP_CanMsgUL_Parameter(void);
extern int PP_CanMsgUL_run(void);
#endif 
