/******************************************************
�ļ�����	PrvtProt_lock.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/10/21		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_LOCK_H
#define		_PRVTPROT_LOCK_H

/*******************************************************
description�� macro definitions
*******************************************************/
//设置lock，返回状态
#define PP_LOCK_OK                 0
#define PP_LOCK_ERR_FOTAREADVER   -1
#define PP_LOCK_ERR_FOTAUPDATE    -2
#define PP_LOCK_ERR_VEHICTRLING   -3
#define PP_LOCK_ERR_DIAGING       -4

//mask
#define PP_LOCK_MASK_TSPDIAG        0x00000001
#define PP_LOCK_MASK_ACTIVE         0x00000002
#define PP_LOCK_MASK_CLEAN          0x00000004
#define	PP_LOCK_MASK_READECUVER     0x00000008
#define PP_LOCK_MASK_FOTAUPDATE     0x00000010
#define PP_LOCK_MASK_AC             0x00000020
#define PP_LOCK_MASK_AUTODOOR       0x00000040
#define PP_LOCK_MASK_BLEKEYSTART    0x00000080
#define PP_LOCK_MASK_CAMERA         0x00000100
#define PP_LOCK_MASK_CHRG           0x00000200
#define PP_LOCK_MASK_DOORLOCK       0x00000400
#define PP_LOCK_MASK_SEARCHVEHI     0x00000800
#define PP_LOCK_MASK_ENGINE         0x00001000
#define PP_LOCK_MASK_FORBIDSTART    0x00002000
#define PP_LOCK_MASK_SUNROOF        0x00004000
#define PP_LOCK_MASK_RMTSTART       0x00008000
#define PP_LOCK_MASK_SEATHEAT       0x00010000

#define PP_LOCK_MASK_DIAG_ALL       (PP_LOCK_MASK_TSPDIAG | PP_LOCK_MASK_ACTIVE | PP_LOCK_MASK_CLEAN)
#define PP_LOCK_MASK_OTA_ALL        (PP_LOCK_MASK_READECUVER | PP_LOCK_MASK_FOTAUPDATE)
#define PP_LOCK_MASK_CTRL_ALL       (PP_LOCK_MASK_AC | PP_LOCK_MASK_AUTODOOR | PP_LOCK_MASK_BLEKEYSTART | \
                                     PP_LOCK_MASK_CAMERA | PP_LOCK_MASK_CHRG | PP_LOCK_MASK_DOORLOCK    | \
                                     PP_LOCK_MASK_SEARCHVEHI | PP_LOCK_MASK_ENGINE | PP_LOCK_MASK_FORBIDSTART | \
                                     PP_LOCK_MASK_SUNROOF | PP_LOCK_MASK_RMTSTART | PP_LOCK_MASK_SEATHEAT)
                                    

/*******************************************************
description�� struct definitions
*******************************************************/



/*******************************************************
description�� typedef definitions
*******************************************************/
/*****struct definitions*****/
typedef struct
{
	uint8_t	    obj;
    uint64_t	mask;
}PrvtProt_lock_mask_t;
/******enum definitions******/
typedef enum
{
	PP_LOCK_DIAG_TSPDIAG = 0,//tsp请求诊断
    PP_LOCK_DIAG_ACTIVE,//主动诊断
    PP_LOCK_DIAG_CLEAN,//清故障码
	PP_LOCK_OTA_READECUVER,//读取ecu版本
    PP_LOCK_OTA_FOTAUPDATE,//fota升级
    PP_LOCK_VEHICTRL_AC,//
    PP_LOCK_VEHICTRL_AUTODOOR,//
    PP_LOCK_VEHICTRL_BLEKEYSTART,//
    PP_LOCK_VEHICTRL_CAMERA,//
    PP_LOCK_VEHICTRL_CHRG,//
    PP_LOCK_VEHICTRL_DOORLOCK,//
    PP_LOCK_VEHICTRL_SEARCHVEHI,//
    PP_LOCK_VEHICTRL_ENGINE,//
    PP_LOCK_VEHICTRL_FORBIDSTART,//
    PP_LOCK_VEHICTRL_SUNROOF,//
    PP_LOCK_VEHICTRL_RMTSTART,//
    PP_LOCK_VEHICTRL_SEAT,//
    PP_LOCK_OBJ_MAX//
} PP_LOCK_OBJ;

/******enum definitions******/

/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern int setPP_lock_odcmtxlock(unsigned char obj);
extern void clearPP_lock_odcmtxlock(unsigned char obj);
extern void InitPP_lock_parameter(void);
extern void showPP_lock_mutexlockstatus(void);
#endif 
