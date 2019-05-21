/******************************************************
文件名：	PrvtProt_rmtCtrl.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMT_CTRL_H
#define		_PRVTPROT_RMT_CTRL_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/
//控制对象
#define PP_RMTCTRL_UNKNOW				0xFF
//vehicle control
#define PP_RMTCTRL_DOORLOCK				0x00//车门锁
#define PP_RMTCTRL_DOORLOCKOPEN			0x0000//车门锁打开
#define PP_RMTCTRL_DOORLOCKCLOSE		0x0001//车门锁关闭

#define PP_RMTCTRL_PNRSUNROOF			0x01//panoramic sunroof全景天窗
#define PP_RMTCTRL_PNRSUNROOFOPEN		0x0100//panoramic sunroof open
#define PP_RMTCTRL_PNRSUNROOFCLOSE		0x0101//panoramic sunroof close
#define PP_RMTCTRL_PNRSUNROOFUPWARP		0x0102//panoramic sunroof upwarp
#define PP_RMTCTRL_PNRSUNROOFSTOP		0x0103//panoramic sunroof stop

#define PP_RMTCTRL_AUTODOOR				0x02//Automatic doors感应式电动门
#define PP_RMTCTRL_AUTODOOROPEN			0x0200//Automatic doors open
#define PP_RMTCTRL_AUTODOORCLOSE		0x0201//Automatic doors	close

//The comfort and convenience
#define PP_RMTCTRL_RMTSRCHVEHICLE		0x03//Remote search vehicle
#define PP_RMTCTRL_RMTSRCHVEHICLEOPEN	0x0300//Remote search vehicle open

#define PP_RMTCTRL_DETECTCAMERA			0x04//Driver detection camera
#define PP_RMTCTRL_CAMERAPHOTO			0x0400//摄像头拍照
#define PP_RMTCTRL_RECORDVIDEO			0x0401//视频录制

#define PP_RMTCTRL_DATARECORDER			0x05//automobile data recorder
#define PP_RMTCTRL_RECORDERPHOTO		0x0500//行车记录仪拍照
#define PP_RMTCTRL_RECORDERVIDEO		0x0501//行车记录仪录制视频

//Air conditioning related
#define PP_RMTCTRL_AC					0x06//Air conditioning
#define PP_RMTCTRL_ACOPEN				0x0600//Air conditioning open
#define PP_RMTCTRL_ACCLOSE				0x0601//Air conditioning close
#define PP_RMTCTRL_ACAPPOINTOPEN		0x0602//appointment to open
#define PP_RMTCTRL_ACCANCELAPPOINT		0x0603//cancel appointment
#define PP_RMTCTRL_SETTEMP				0x0604//set temperature
#define PP_RMTCTRL_MAINHEATOPEN			0x0605//open main driving heatin
#define PP_RMTCTRL_MAINHEATCLOSE		0x0606//close main driving heatin
#define PP_RMTCTRL_PASSENGERHEATOPEN	0x0607//open PASSENGER driving heatin
#define PP_RMTCTRL_PASSENGERHEATCLOSE	0x0608//close PASSENGER driving heatin

//Energy related
#define PP_RMTCTRL_CHARGE				0x07//充电
#define PP_RMTCTRL_STARTCHARGE			0x0700//开始充电
#define PP_RMTCTRL_STOPCHARGE			0x0701//停止充电
#define PP_RMTCTRL_APPOINTCHARGE		0x0702//预约充电
#define PP_RMTCTRL_CHRGCANCELAPPOINT	0x0703//取消预约

#define PP_RMTCTRL_HIGHTENSIONCTRL		0x08//高压电控制
#define PP_RMTCTRL_POWERON				0x0800//上电
#define PP_RMTCTRL_POWEROFF				0x0801//下电

#define PP_RMTCTRL_ENGINECTRL			0x09//发动机控制
#define PP_RMTCTRL_BANSTART				0x0900//
#define PP_RMTCTRL_ALOWSTART			0x0901//

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
	RMTCTRL_DOORLOCK = 0,//车门锁
	RMTCTRL_PANORSUNROOF,//全景天窗
	RMTCTRL_AUTODOOR,//自动感应门
	RMTCTRL_RMTSRCHVEHICLE,//
	DETECTCAMERA,
	DATARECORDER,
	RMTCTRL_AC,
	RMTCTRL_CHARGE,
	RMTCTRL_HIGHTENSIONCTRL,
	RMTCTRL_ENGINECTRL,
	RMTCTRL_OBJ_MAX
}PP_RMTCTRL_OBJ;

typedef enum
{
	CTRLDOORLOCK_IDLE = 0,//空闲
	CTRLDOORLOCK_OPEN_WAIT,//开门锁等待状态
	CTRLDOORLOCK_CLOSE_WAIT,//关门锁等待状态

}PP_RMTCTRL_DOORLOCK_STATE;


typedef struct
{
	uint8_t req;
	uint8_t reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_rmtCtrlSt_t; /*remote control结构体*/

/* application data struct */
typedef struct
{
	long rvcReqType;
	uint8_t rvcReqParams[255];
	uint8_t rvcReqParamslen;
}__attribute__((packed))  App_rmtCtrlReq_t;


typedef struct
{
	App_rmtCtrlReq_t CtrlReq;
}__attribute__((packed))  PrvtProt_App_rmtCtrl_t;

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_rmtCtrl_init(void);
extern int PP_rmtCtrl_mainfunction(void *task);
//extern void PP_rmtCtrl_SetEcallReq(unsigned char req);

#endif 
