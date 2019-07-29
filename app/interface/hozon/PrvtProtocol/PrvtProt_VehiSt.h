/******************************************************
�ļ�����	PrvtProt_VehiSt.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_VEHI_ST_H
#define		_PRVTPROT_VEHI_ST_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_VS_LOW_TEMP		16
#define PP_VS_HIGH_TEMP		32

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	PP_VS_NOREQ = 0,//
	PP_VS_BASICSTATUS,//
	PP_VS_EXTSTATUS//
} PP_VS_REQTYPE;//��ѯ����״̬����������

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* ����:box to tsp */
	uint8_t resp;/* ��Ӧ:box to tsp */
	uint8_t retrans;/* retransmission */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_VSSt_t; /*�ṹ��*/

/* application data struct */
typedef struct
{
	long vehStatusReqType;
}PrvtProt_VSReq_t;

typedef struct
{
	int  gpsSt;//gps״̬ 0-��Ч��1-��Ч
	long gpsTimestamp;//gpsʱ���
	long latitude;//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long longitude;//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long altitude;//�߶ȣ�m��
	long heading;//��ͷ����Ƕȣ�0Ϊ��������
	long gpsSpeed;//�ٶ� x 10����λkm/h
	long hdop;//ˮƽ�������� x 10
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
	long	accTemp	/* OPTIONAL */;//ȡֵ��Χ��18-36
	long	accMode	/* OPTIONAL */;//ȡֵ��Χ��0-3
	long	accBlowVolume	/* OPTIONAL */;//ȡֵ��Χ��0-7
	long	 innerTemp;//ȡֵ��Χ��0-125
	long	 outTemp;//ȡֵ��Χ��0-125
	int	 sideLightStatus;
	int	 dippedBeamStatus;
	int	 mainBeamStatus;
	int	 hazardLightStus;
	long	frtRightTyrePre	/* OPTIONAL */;//ȡֵ��Χ��0-45
	long	frtRightTyreTemp	/* OPTIONAL */;//ȡֵ��Χ��0-168
	long	frontLeftTyrePre	/* OPTIONAL */;//ȡֵ��Χ��0-45
	long	frontLeftTyreTemp	/* OPTIONAL */;//ȡֵ��Χ��0-168
	long	rearRightTyrePre	/* OPTIONAL */;//ȡֵ��Χ��0-45
	long	rearRightTyreTemp	/* OPTIONAL */;//ȡֵ��Χ��0-165
	long	rearLeftTyrePre	/* OPTIONAL */;//ȡֵ��Χ��0-45
	long	rearLeftTyreTemp	/* OPTIONAL */;//ȡֵ��Χ��0-165
	long	 batterySOCExact;//ȡֵ��Χ��0-10000
	long	chargeRemainTim	/* OPTIONAL */;//ȡֵ��Χ��0-65535
	long	 availableOdomtr;//ȡֵ��Χ��0-65535
	long	engineRunningTime	/* OPTIONAL */;//ȡֵ��Χ��0-65535
	int	 	bookingChargeSt;
	long	bookingChargeHour	/* OPTIONAL */;//ȡֵ��Χ��0-23
	long	bookingChargeMin	/* OPTIONAL */;//ȡֵ��Χ��0-59
	long	chargeMode	/* OPTIONAL */;//ȡֵ��Χ��0-255
	long	chargeStatus	/* OPTIONAL */;//ȡֵ��Χ��0-255
	long	powerMode	/* OPTIONAL */;//ȡֵ��Χ��0-255
	long	 speed;//ȡֵ��Χ��0-2500
	long	 totalOdometer;//ȡֵ��Χ��0-1000000
	long	 batteryVoltage;//ȡֵ��Χ��0-10000
	long	 batteryCurrent;//ȡֵ��Χ��0-10000
	long	 batterySOCPrc;//ȡֵ��Χ��0-100
	int	 dcStatus;
	long	 gearPosition;//ȡֵ��Χ��0-255
	long	 insulationRstance;//ȡֵ��Χ��0-60000
	long	 acceleratePedalprc;//ȡֵ��Χ��0-100
	long	 deceleratePedalprc;//ȡֵ��Χ��0-100
	int	 canBusActive;
	int	 bonnetStatus;
	int	 lockStatus;
	int	 gsmStatus;
	long	wheelTyreMotrSt	/* OPTIONAL */;//ȡֵ��Χ��0-255
	long	 vehicleAlarmSt;//ȡֵ��Χ��0-255
	long	 currentJourneyID;//ȡֵ��Χ��0-2147483647
	long	 journeyOdom;//ȡֵ��Χ��0-65535
	long	frtLeftSeatHeatLel	/* OPTIONAL */;//ȡֵ��Χ��0-255
	long	frtRightSeatHeatLel	/* OPTIONAL */;//ȡֵ��Χ��0-255
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
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_VS_init(void);
extern int PP_VS_mainfunction(void *task);
extern void PP_VS_SetVSReq(unsigned char req);

#endif 
