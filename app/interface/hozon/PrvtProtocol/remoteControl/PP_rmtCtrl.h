/******************************************************
�ļ�����	PrvtProt_rmtCtrl.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/05/18		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_RMT_CTRL_H
#define		_PRVTPROT_RMT_CTRL_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
//���ƶ���
#define PP_RMTCTRL_UNKNOW				0xFF
//vehicle control
#define PP_RMTCTRL_DOORLOCK				0x00//������
#define PP_RMTCTRL_DOORLOCKOPEN			0x0000//��������
#define PP_RMTCTRL_DOORLOCKCLOSE		0x0001//�������ر�

#define PP_RMTCTRL_PNRSUNROOF			0x01//panoramic sunroofȫ���촰
#define PP_RMTCTRL_PNRSUNROOFOPEN		0x0100//panoramic sunroof open
#define PP_RMTCTRL_PNRSUNROOFCLOSE		0x0101//panoramic sunroof close
#define PP_RMTCTRL_PNRSUNROOFUPWARP		0x0102//panoramic sunroof upwarp
#define PP_RMTCTRL_PNRSUNROOFSTOP		0x0103//panoramic sunroof stop

#define PP_RMTCTRL_AUTODOOR				0x02//Automatic doors��Ӧʽ�綯��
#define PP_RMTCTRL_AUTODOOROPEN			0x0200//Automatic doors open
#define PP_RMTCTRL_AUTODOORCLOSE		0x0201//Automatic doors	close

//The comfort and convenience
#define PP_RMTCTRL_RMTSRCHVEHICLE		0x03//Remote search vehicle
#define PP_RMTCTRL_RMTSRCHVEHICLEOPEN	0x0300//Remote search vehicle open

#define PP_RMTCTRL_DETECTCAMERA			0x04//Driver detection camera
#define PP_RMTCTRL_CAMERAPHOTO			0x0400//����ͷ����
#define PP_RMTCTRL_RECORDVIDEO			0x0401//��Ƶ¼��

#define PP_RMTCTRL_DATARECORDER			0x05//automobile data recorder
#define PP_RMTCTRL_RECORDERPHOTO		0x0500//�г���¼������
#define PP_RMTCTRL_RECORDERVIDEO		0x0501//�г���¼��¼����Ƶ

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
#define PP_RMTCTRL_CHARGE				0x07//���
#define PP_RMTCTRL_STARTCHARGE			0x0700//��ʼ���
#define PP_RMTCTRL_STOPCHARGE			0x0701//ֹͣ���
#define PP_RMTCTRL_APPOINTCHARGE		0x0702//ԤԼ���
#define PP_RMTCTRL_CHRGCANCELAPPOINT	0x0703//ȡ��ԤԼ

#define PP_RMTCTRL_HIGHTENSIONCTRL		0x08//��ѹ�����
#define PP_RMTCTRL_POWERON				0x0800//�ϵ�
#define PP_RMTCTRL_POWEROFF				0x0801//�µ�

#define PP_RMTCTRL_ENGINECTRL			0x09//����������
#define PP_RMTCTRL_BANSTART				0x0900//
#define PP_RMTCTRL_ALOWSTART			0x0901//

/***********�꺯��***********/
typedef void (*PP_rmtCtrlInitObj)(void);//��ʼ��
typedef int (*PP_rmtCtrlmainFuncObj)(void* x);//
/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/
typedef enum
{
	RMTCTRL_DOORLOCK = 0,//������
	RMTCTRL_PANORSUNROOF,//ȫ���촰
	RMTCTRL_AUTODOOR,//�Զ���Ӧ��
	RMTCTRL_RMTSRCHVEHICLE,//
	RMTCTRL_DETECTCAMERA,
	RMTCTRL_DATARECORDER,
	RMTCTRL_AC,
	RMTCTRL_CHARGE,
	RMTCTRL_HIGHTENSIONCTRL,
	RMTCTRL_ENGINECTRL,
	RMTCTRL_OBJ_MAX
}PP_RMTCTRL_OBJ;

typedef enum
{
	CTRLDOORLOCK_IDLE = 0,//����
	CTRLDOORLOCK_OPEN_WAIT,//�������ȴ�״̬
	CTRLDOORLOCK_CLOSE_WAIT,//�������ȴ�״̬

}PP_RMTCTRL_DOORLOCK_STATE;


typedef struct
{
	uint8_t req;
	uint8_t reqType;
	uint8_t CtrlSt;
	uint64_t period;
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_rmtCtrlSt_t; /*remote control�ṹ��*/

/* application data struct */
typedef struct
{
	long rvcReqType;
	uint8_t rvcReqParams[255];
	uint8_t rvcReqParamslen;
}__attribute__((packed))  App_rmtCtrlReq_t;

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
	App_rmtCtrlReq_t CtrlReq;
	App_rmtCtrlResp_t CtrlResp;
	App_rmtCtrlbookingResp_t CtrlbookingResp;
}__attribute__((packed))  PrvtProt_App_rmtCtrl_t;


typedef struct
{
	char rmtObj;
	PP_rmtCtrlInitObj 		Init;//��ʼ��
	PP_rmtCtrlmainFuncObj	mainFunc;//
}PrvtProt_RmtCtrlFunc_t; /*�ṹ��*/
/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_rmtCtrl_init(void);
extern int PP_rmtCtrl_mainfunction(void *task);
extern void PP_rmtCtrl_SetCtrlReq(unsigned char req,uint16_t reqType);

#endif 
