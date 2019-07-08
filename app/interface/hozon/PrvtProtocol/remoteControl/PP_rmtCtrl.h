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


//执行状态
#define PP_RMTCTRL_EXECUTEDWAIT 	0//等待执行
#define PP_RMTCTRL_EXECUTEDSTART 	1//开始执行
#define PP_RMTCTRL_EXECUTEDFINISH	2//执 行完成
#define PP_RMTCTRL_EXECUTEDFAIL		3//执行失 败


//指令执行失败类型
#define PP_RMTCTRL_ACCNOOFF			0x01//
#define PP_RMTCTRL_BCDMAUTHFAIL		0x01//
#define PP_RMTCTRL_TIMEOUTFAIL		0x01//
#define PP_RMTCTRL_CHRGGUNUNCONNT		0x01//
/***********宏函数***********/
typedef void (*PP_rmtCtrlInitObj)(void);//初始化
typedef int (*PP_rmtCtrlmainFuncObj)(void* x);//

typedef void (*PP_ctrlStInform_cb)(void* x);//控制状态通知回调
/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	RMTCTRL_TSP = 1, //tsp
	RMTCTRL_BLUETOOTH, //蓝牙
	RMTCTRL_HU,//车机
	RMTCTRL_TBOX  //TBOX
}PP_RMTCTRL_CTRLSTYLE;//控制方式

typedef enum
{
	RMTCTRL_IDLE = 0,//
	RMTCTRL_IDENTIFICAT_QUERY,//֤
	RMTCTRL_IDENTIFICAT_LAUNCH,//֤
	RMTCTRL_COMMAND_LAUNCH,    //
}PP_RMTCTRL_COMMAND_OBJ;

typedef enum
{
	RMTCTRL_DOORLOCK = 0,//车门锁
	RMTCTRL_PANORSUNROOF,//全景天窗
	RMTCTRL_AUTODOOR,//自动感应门
	RMTCTRL_RMTSRCHVEHICLE,//寻车
	RMTCTRL_HIGHTENSIONCTRL,//高压电
	RMTCTRL_AC,//空调
	RMTCTRL_CHARGE,//充电
	RMTCTRL_ENGINECTRL,//禁止启动
	RMTCTRL_SEATHEATINGCTRL,//座椅加热
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
	int  gpsSt;//gps状态 0-无效；1-有效
	long gpsTimestamp;//gps时间戳
	long latitude;//纬度 x 1000000,当GPS信号无效时，值为0
	long longitude;//经度 x 1000000,当GPS信号无效时，值为0
	long altitude;//高度（m）
	long heading;//车头方向角度，0为正北方向
	long gpsSpeed;//速度 x 10，单位km/h
	long hdop;//水平精度因子 x 10
}__attribute__((packed)) PP_rmtCtrlgpsposition_t;

typedef struct
{
	int	driverDoor	/* OPTIONAL */;
	int	 driverLock;
	int	passengerDoor	/* OPTIONAL */;
	int	 passengerLock;
	int	rearLeftDoor	/* OPTIONAL */;
	int	 rearLeftLock;
	int	rearRightDoor	/* OPTIONAL */;
	int	 rearRightLock;
	int	bootStatus	/* OPTIONAL */;
	int	 bootStatusLock;
	int	driverWindow	/* OPTIONAL */;
	int	passengerWindow	/* OPTIONAL */;
	int	rearLeftWindow	/* OPTIONAL */;
	int	rearRightWinow	/* OPTIONAL */;
	int	sunroofStatus	/* OPTIONAL */;
	int	 engineStatus;
	int	 accStatus;
	long	accTemp	/* OPTIONAL */;//取值范围：18-36
	long	accMode	/* OPTIONAL */;//取值范围：0-3
	long	accBlowVolume	/* OPTIONAL */;//取值范围：0-7
	long	 innerTemp;//取值范围：0-125
	long	 outTemp;//取值范围：0-125
	int	 sideLightStatus;
	int	 dippedBeamStatus;
	int	 mainBeamStatus;
	int	 hazardLightStus;
	long	frtRightTyrePre	/* OPTIONAL */;//取值范围：0-45
	long	frtRightTyreTemp	/* OPTIONAL */;//取值范围：0-168
	long	frontLeftTyrePre	/* OPTIONAL */;//取值范围：0-45
	long	frontLeftTyreTemp	/* OPTIONAL */;//取值范围：0-168
	long	rearRightTyrePre	/* OPTIONAL */;//取值范围：0-45
	long	rearRightTyreTemp	/* OPTIONAL */;//取值范围：0-165
	long	rearLeftTyrePre	/* OPTIONAL */;//取值范围：0-45
	long	rearLeftTyreTemp	/* OPTIONAL */;//取值范围：0-165
	long 	batterySOCExact;//取值范围：0-10000
	long	chargeRemainTim	/* OPTIONAL */;//取值范围：0-65535
	long	availableOdomtr;//取值范围：0-65535
	long	engineRunningTime	/* OPTIONAL */;//取值范围：0-65535
	int	 	bookingChargeSt;
	long	bookingChargeHour	/* OPTIONAL */;//取值范围：0-23
	long	bookingChargeMin	/* OPTIONAL */;//取值范围：0-59
	long	chargeMode	/* OPTIONAL */;//取值范围：0-255
	long	chargeStatus	/* OPTIONAL */;//取值范围：0-255
	long	powerMode	/* OPTIONAL */;//取值范围：0-255
	long	speed;//取值范围：0-2500
	long	totalOdometer;//取值范围：0-1000000
	long	batteryVoltage;//取值范围：0-10000
	long	batteryCurrent;//取值范围：0-10000
	long	batterySOCPrc;//取值范围：0-100
	int	 dcStatus;
	long	 gearPosition;//取值范围：0-255
	long	 insulationRstance;//取值范围：0-60000
	long	 acceleratePedalprc;//取值范围：0-100
	long	 deceleratePedalprc;//取值范围：0-100
	int	 canBusActive;
	int	 bonnetStatus;
	int	 lockStatus;
	int	 gsmStatus;
	long	wheelTyreMotrSt	/* OPTIONAL */;//取值范围：0-255
	long	 vehicleAlarmSt;//取值范围：0-255
	long	 currentJourneyID;//取值范围：0-2147483647
	long	 journeyOdom;//取值范围：0-65535
	long	frtLeftSeatHeatLel	/* OPTIONAL */;//取值范围：0-255
	long	frtRightSeatHeatLel	/* OPTIONAL */;//取值范围：0-255
	int		airCleanerSt	/* OPTIONAL */;
	int		srsStatus;
}__attribute__((packed))  App_rmtCtrlResp_basicSt_t;

typedef struct
{
	/* remote control resp */
	long rvcReqType;
	long rvcReqStatus;
	long rvcFailureType;
	PP_rmtCtrlgpsposition_t gpsPos;
	App_rmtCtrlResp_basicSt_t basicSt;
}__attribute__((packed))  App_rmtCtrlResp_t;

typedef struct
{
	/* remote control booking resp */
	long bookingId;
	long rvcReqCode;
	long oprTime;
}__attribute__((packed))  App_rmtCtrlbookingResp_t;

typedef struct
{
	/* remote control HU booking resp */
	long	rvcReqType;
	long	huBookingTime;
	long	rvcReqHours;
	long	rvcReqMin;
	long	rvcReqEq	/* OPTIONAL */;
	uint8_t	rvcReqCycle[9]	/* OPTIONAL */;
	uint8_t rvcReqCyclelen;
	long	bookingId	/* OPTIONAL */;
}__attribute__((packed))  App_rmtCtrlHUbookingResp_t;

typedef struct
{
	App_rmtCtrlReq_t 	CtrlReq;
	App_rmtCtrlResp_t 	CtrlResp;
	App_rmtCtrlbookingResp_t 	CtrlbookingResp;
	App_rmtCtrlHUbookingResp_t 	CtrlHUbookingResp;
}__attribute__((packed))  PrvtProt_App_rmtCtrl_t;


typedef struct
{
	char rmtObj;
	PP_rmtCtrlInitObj 		Init;//初始化
	PP_rmtCtrlmainFuncObj	mainFunc;//
}PrvtProt_RmtCtrlFunc_t; /*结构体*/


#define PP_RMTCTRL_RVCSTATUSRESP 	1
#define PP_RMTCTRL_RVCBOOKINGRESP 	2
#define PP_RMTCTRL_HUBOOKINGRESP 	3
typedef struct
{
	/* tsp */
	char Resptype;//回复类型：非预约-1；2-预约
	long reqType;//请求类型
	long eventid;//事件id
	long rvcReqStatus;
	long rvcFailureType;

	long bookingId;
	long rvcReqCode;

	long	rvcReqType;
	long	huBookingTime;
	long	rvcReqHours;
	long	rvcReqMin;
	long	rvcReqEq	/* OPTIONAL */;
	uint8_t	rvcReqCycle[8]	/* OPTIONAL */;
	//long	HUbookingId	/* OPTIONAL */;

	/* 蓝牙 */

}PP_rmtCtrl_Stpara_t; /*结构体*/
/******union definitions*****/


/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_rmtCtrl_init(void);
extern int 	PP_rmtCtrl_mainfunction(void *task);
extern void PP_rmtCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);
extern int PP_rmtCtrl_StInformTsp(void *task,PP_rmtCtrl_Stpara_t *CtrlSt_para);
extern void PP_rmtCtrl_BluetoothSetCtrlReq(unsigned char obj, unsigned char cmd);
#endif 
