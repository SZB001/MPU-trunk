/******************************************************
文件名：	PrvtProt_VehiSt.h

描述：	企业私有协议（浙江合众）	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_VEHI_ST_H
#define		_PRVTPROT_VEHI_ST_H
/*******************************************************
description： include the header file
*******************************************************/

/*******************************************************
description： macro definitions
*******************************************************/
/**********宏开关定义*********/

/**********宏常量定义*********/


/*******************************************************
description： struct definitions
*******************************************************/

/*******************************************************
description： typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_VS_NOREQ = 0,//
	PP_VS_BASICSTATUS,//
	PP_VS_EXTSTATUS//
} PP_VS_REQTYPE;//查询车辆状态的请求类型

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* 请求:box to tsp */
	uint8_t resp;/* 响应:box to tsp */
	uint8_t retrans;/* retransmission */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_VSSt_t; /*结构体*/

/* application data struct */
typedef struct
{
	long vehStatusReqType;
}PrvtProt_VSReq_t;

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
}__attribute__((packed)) PP_APP_VSgpspos_t;

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
	long	 batterySOCExact;//取值范围：0-10000
	long	chargeRemainTim	/* OPTIONAL */;//取值范围：0-65535
	long	 availableOdomtr;//取值范围：0-65535
	long	engineRunningTime	/* OPTIONAL */;//取值范围：0-65535
	int	 	bookingChargeSt;
	long	bookingChargeHour	/* OPTIONAL */;//取值范围：0-23
	long	bookingChargeMin	/* OPTIONAL */;//取值范围：0-59
	long	chargeMode	/* OPTIONAL */;//取值范围：0-255
	long	chargeStatus	/* OPTIONAL */;//取值范围：0-255
	long	powerMode	/* OPTIONAL */;//取值范围：0-255
	long	 speed;//取值范围：0-2500
	long	 totalOdometer;//取值范围：0-1000000
	long	 batteryVoltage;//取值范围：0-10000
	long	 batteryCurrent;//取值范围：0-10000
	long	 batterySOCPrc;//取值范围：0-100
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
}__attribute__((packed))  PP_App_VS_basicSt_t;

typedef struct
{
	long alertSize;
	uint8_t alertIds[256];
	//uint8_t alertIdslen;
	uint8_t validFlg;
}__attribute__((packed)) PP_App_VS_ExtSt_t;

typedef struct
{
	long statusTime;
	PP_APP_VSgpspos_t	gpsPos;
	PP_App_VS_basicSt_t	basicSt;
	PP_App_VS_ExtSt_t 	ExtSt;
}__attribute__((packed)) PrvtProt_VSResp_t;

typedef struct
{
	PrvtProt_VSReq_t 	VSReq;
	PrvtProt_VSResp_t	VSResp;
}__attribute__((packed)) PrvtProt_App_VS_t;

/******union definitions*****/

/*******************************************************
description： variable External declaration
*******************************************************/

/*******************************************************
description： function External declaration
*******************************************************/
extern void PP_VS_init(void);
extern int PP_VS_mainfunction(void *task);
extern void PP_VS_SetVSReq(unsigned char req);

#endif 
