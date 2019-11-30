#ifndef __GB32960_API_H__
#define __GB32960_API_H__



#define GB32960_EXWARN 57
#define GB32960_VSWARN  (GB32960_EXWARN + 2)

#define GB32960_GLWARN 32
#define GB32960_MAXWARN (GB32960_EXWARN + GB32960_GLWARN + 2)

//存在两个告警信号出现二对一的情况
#define tempdiffWARN 					0//温度差异报警
#define battovertempWARN				1//电池高温报警
#define energystorovervolWARN			2//车载储能装置类型过压报警
#define energystorlowvolWARN			3//车载储能装置类型欠压报警
#define soclowWARN						4//soc低报警
#define battcellovervolWARN				5//单体电池过压报警
#define battcelllowvolWARN				6//单体电池欠压报警
#define socovervolWARN					7//SOC 过压报警
#define socjumpWARN						8//SOC 跳变报警
#define energystorunmatchWARN			9//可充电储能系统不匹配报警
#define battcellconsdiffWARN			10//电池单体一致性差报警
#define insulationWARN					11//绝缘报警
#define dcdctempWARN					12//DC-DC 温度报警
#define brakeWARN						13//制动系统报警
#define dcdcstatusWARN					14//DC-DC 状态报警
#define motorctrltempWARN				15//驱动电机控制器温度报警
#define highvolinterlockWARN			16//高压互锁状态报警
#define motortempWARN					17//驱动电机温度报警
#define energystoroverchrgWARN			18//车载储能装置类型过充报警
#define reserveWARN19					19//
#define reserveWARN20					20//
#define reserveWARN21					21//
#define reserveWARN22					22//
#define reserveWARN23					23//
#define reserveWARN24					24//
#define reserveWARN25					25//
#define reserveWARN26					26//
#define reserveWARN27					27//
#define reserveWARN28					28//
#define reserveWARN29					29//
#define reserveWARN30					30//
#define reserveWARN31					31//
#define batt12vlowvolWARN               32//12V 蓄电池电压过低 报警
#define epsfaultWARN                    33//EPS 故障 报警
#define epstorquesensorWARN             34//EPS 扭矩传感器信号故障报警
#define mcuIGBTVovercurrentWARN         35//MCU IGBT 驱动电路过流故障（V 相） 报警
#define mcuIGBTWovercurrentWARN         36//MCU IGBT 驱动电路过流故障（W 相） 报警
#define mcupowermoduleWARN              37//MCU 电源模块故障 报警
#define mcuIGBTUovertempWARN            38//MCU 内部 IGBT 过温（U 相） 报警
#define mcuIGBTUdrivecircuitWARN        39//MCU 内部 IGBT 驱动电路报警（U 相）报警
#define mcupossensorWARN                40//MCU 位置传感器检测回路故障报警
#define mcuhardwareUflowWARN            41//MCU 相电流硬件过流（U 相）报警
#define mcuDCbusovervolWARN             42//MCU 直流母线过压 报警
#define mcuDCbuslowvolWARN              43//MCU 直流母线欠压报警
#define tboxfaultWARN                   44//TBOX 故障报警
#define SRSmoduleWARN                   45//安全气囊模块异常报警
#define carchargeroverloadWARN          46//车载充电器过载报警
#define carchargerundervolWARN          47//车载充电器欠压报警
#define bigcreeninfonotconsWARN         48//大屏信息版本信息不一致报警
#define singlebatteryovervoltageWARN    49//单体蓄电池过压报警
#define gearsfaultWARN                  50//档位故障报警
#define gearssignfaultWARN              51//档位信号故障报警
#define battmanagesysmissingWARN        52//电池管理系统丢失故障报警
#define battheatsfastWARN               53//电池升温过快报警
#define motorctrlIGBTfaultWARN          54//电机控制器 IGBT 故障 报警
#define motorctrlinterlockWARN          55//电机控制器环路互锁报警
#define motorctrlundervloWARN           56//电机控制器欠压故障 报警
#define motorabnormalWARN               57//电机异常报警
#define pwrbattcellovervolproWARN       58//动力电池单体电压过压保护
#define pwrbattcellundervolproWARN      59//动力电池单体电压欠压保护故障报警
#define pwrbattsoclowWARN               60//动力电池电量过低报警
#define pwrbattvolimbalanceproWARN      61//动力电池电压不均衡保护故障
#define pwrbattinterlockWARN            62//动力电池环路互锁
#define pwrbattovertempproWARN          63//动力电池温度过高保护故障
#define accpedalsignalovershootWARN     64//加速踏板信号超幅错误
#define accpedalsignalfaultWARN         65//加速踏板信号故障
#define acfanunworkWARN                 66//空调风扇不工作
#define motorCANfaultWARN               67//驱动电机 CAN 通讯故障
#define BMSmissingWARN                  68//与 BMS 通讯丢失
#define MCUmissingWARN                  69//与 MCU 通讯丢失
#define abnormalvehicleheatWARN         70//整车加热工程异常
#define brakesystemfaultWARN          	71//制动系统故障
#define LRbrakelightsmalfaultWARN       72//左右刹车灯故障
#define brakefluidfaultWARN 		    73//制动液异常
#define tirepressuresystemfaultWARN     74//胎压系统故障
#define antitheftintrusionalarmWARN	    75//防盗入侵报警
#define trailerreminderWARN			    76//拖车提醒
#define electronicassistfaultWARN       77//电子助力故障
#define acunworkWARN 		            78//空调不工作报警
#define compressorfaultWARN	            79//制冷不响应原因-压缩机故障
#define electronicexpansionfaultWARN    80//制冷不响应原因-电子膨胀阀故障
#define coolhvfaultWARN	                81//制冷不响应原因-HV 故障
#define pressuresensorfaultWARN         82//制冷不响应原因-压力传感器故障
#define coolingfanfaultWARN             83//制冷不响应原因-冷却风扇故障
#define temperaturesensorfaultWARN      84//制冷不响应原因-蒸发器温度传感器故障
#define ptcfaultWARN 	                85//制热不响应原因-PTC 故障
#define hothvfaultWARN	                86//制热不响应原因-HV 故障
#define ptcpumpfaultWARN                87//制热不响应原因-PTC 水泵故障
#define watervalvefaultWARN             88//制热不响应原因-三通水阀故障

#define GB32960_API_FAULTNUM (watervalvefaultWARN + 1)

typedef union
{
	unsigned char warn[GB32960_API_FAULTNUM];/* */
	struct
	{
		unsigned char tempdiffwarn;//温度差异报警
		unsigned char battovertempwarn;//电池高温报警
		unsigned char energystorovervolwarn;//车载储能装置类型过压报警
		unsigned char energystorlowvolwarn;//车载储能装置类型欠压报警
		unsigned char soclowwarn;//soc低报警
		unsigned char battcellovervolwarn;//单体电池过压报警
		unsigned char battcelllowvolwarn;//单体电池欠压报警
		unsigned char socovervolwarn;//SOC 过压报警
		unsigned char socjumpwarn;//SOC 跳变报警
		unsigned char energystorunmatchwarn;//可充电储能系统不匹配报警
		unsigned char battcellconsdiffwarn;//电池单体一致性差报警
		unsigned char insulationwarn;//绝缘报警
		unsigned char dcdctempwarn;//DC-DC 温度报警
		unsigned char brakewarn;//制动系统报警
		unsigned char dcdcstatuswarn;//DC-DC 状态报警
		unsigned char motorctrltempwarn;//驱动电机控制器温度报警
		unsigned char highvolinterlockwarn;//高压互锁状态报警
		unsigned char motortempwarn;//驱动电机温度报警
		unsigned char energystoroverchrgwarn;//车载储能装置类型过充报警
		unsigned char reservewarn19;//
		unsigned char reservewarn20;//
		unsigned char reservewarn21;//
		unsigned char reservewarn22;//
		unsigned char reservewarn23;//
		unsigned char reservewarn24;//
		unsigned char reservewarn25;//
		unsigned char reservewarn26;//
		unsigned char reservewarn27;//
		unsigned char reservewarn28;//
		unsigned char reservewarn29;//
		unsigned char reservewarn30;//
		unsigned char reservewarn31;//
		unsigned char batt12vlowvolwarn;//12V 蓄电池电压过低 报警
		unsigned char epsfaultwarn;//EPS 故障 报警
		unsigned char epstorquesensorwarn;//EPS 扭矩传感器信号故障报警
		unsigned char mcuIGBTVovercurrentwarn;//MCU IGBT 驱动电路过流故障（V 相） 报警
		unsigned char mcuIGBTWovercurrentwarn;//MCU IGBT 驱动电路过流故障（W 相） 报警
		unsigned char mcupowermodulewarn;//MCU 电源模块故障 报警
		unsigned char mcuIGBTUovertempwarn;//MCU 内部 IGBT 过温（U 相） 报警
		unsigned char mcuIGBTUdrivecircuitwarn;//MCU 内部 IGBT 驱动电路报警（U 相）报警
		unsigned char mcupossensorwarn;//MCU 位置传感器检测回路故障报警
		unsigned char mcuhardwareUflowwarn;//MCU 相电流硬件过流（U 相）报警
		unsigned char mcuDCbusovervolwarn;//MCU 直流母线过压 报警
		unsigned char mcuDCbuslowvolwarn;//MCU 直流母线欠压报警
		unsigned char tboxfaultwarn;//TBOX 故障报警
		unsigned char SRSmodulewarn;//安全气囊模块异常报警
		unsigned char carchargeroverloadwarn ;//车载充电器过载报警
		unsigned char carchargerundervolwarn;//车载充电器欠压报警
		unsigned char bigcreeninfonotconswarn;//大屏信息版本信息不一致报警
		unsigned char singlebatteryovervoltage;//单体蓄电池过压报警
		unsigned char gearsfaultwarn;//档位故障报警
		unsigned char gearssignfaultwarn;//档位信号故障报警
		unsigned char battmanagesysmissingwarn;//电池管理系统丢失故障报警
		unsigned char battheatsfastwarn;//电池升温过快报警
		unsigned char motorctrlIGBTfaultwarn;//电机控制器 IGBT 故障 报警
		unsigned char motorctrlinterlockwarn;//电机控制器环路互锁报警
		unsigned char motorctrlundervlowarn;//电机控制器欠压故障 报警
		unsigned char motorabnormalwarn;//电机异常报警
		unsigned char pwrbattcellovervolprowarn;//动力电池单体电压过压保护
		unsigned char pwrbattcellundervolprowarn;//动力电池单体电压欠压保护故障报警
		unsigned char pwrbattsoclowwarn;//动力电池电量过低报警
		unsigned char pwrbattvolimbalanceprowarn;//动力电池电压不均衡保护故障
		unsigned char pwrbattinterlockwarn;//动力电池环路互锁
		unsigned char pwrbattovertempprowarn;//动力电池温度过高保护故障
		unsigned char accpedalsignalovershootwarn;//加速踏板信号超幅错误
		unsigned char accpedalsignalfaultwarn;//加速踏板信号故障
		unsigned char acfanunworkwarn;//空调风扇不工作
		unsigned char motorCANfaultwarn;//驱动电机 CAN 通讯故障
		unsigned char BMSmissingwarn;//与 BMS 通讯丢失
		unsigned char MCUmissingwarn;//与 MCU 通讯丢失
		unsigned char abnormalvehicleheatwarn;//整车加热工程异常
		unsigned char brakesystemfaultwarn;      //制动系统故障
		unsigned char LRbrakelightsmalfaultwarn;//左右刹车灯故障
		unsigned char brakefluidfaultwarn;		//制动液异常
		unsigned char tirepressuresystemfaultwarn; //胎压系统故障
		unsigned char antitheftintrusionalarm;     //防盗入侵报警
		unsigned char trailerreminder;             //拖车提醒
		unsigned char electronicassistfaultwarn;   //电子助力故障
		unsigned char acunworkwarn;           //空调不工作报警
		unsigned char compressorfaultwarn;    //制冷不响应原因-压缩机故障
		unsigned char electronicexpansionfaultwarn;//制冷不响应原因-电子膨胀阀故障
		unsigned char coolhvfaultwarn;     //制冷不响应原因-HV 故障
		unsigned char pressuresensorfaultwarn;//制冷不响应原因-压力传感器故障
		unsigned char coolingfanfaultwarn;//制冷不响应原因-冷却风扇故障
		unsigned char temperaturesensorfaultwarn; //制冷不响应原因-蒸发器温度传感器故障
		unsigned char ptcfaultwarn;     //制热不响应原因-PTC 故障
		unsigned char hothvfaultwarn;   //制热不响应原因-HV 故障
		unsigned char ptcpumpfaultwarn;   //制热不响应原因-PTC 水泵故障
		unsigned char watervalvefaultwarn; //制热不响应原因-三通水阀故障
	}type; /**/
}gb32960_api_fault_t;

extern gb32960_api_fault_t gb_fault;

typedef struct
{
	uint8_t  gbindex;//
	uint8_t  vsindex;//
}gb32960_api_extwarn_indextable_t;


extern int gb_set_addr(const char *url, uint16_t port);
extern int gb_set_vin(const char *vin);
extern int gb_set_datintv(uint16_t period);
extern int gb_set_regintv(uint16_t period);
extern int gb_set_timeout(uint16_t timeout);
extern int gb_init(INIT_PHASE phase);
extern int gb_run(void);


extern int gb32960_getNetworkSt(void);
extern void gb32960_getURL(void* ipaddr);
extern int gb32960_getAllowSleepSt(void);
extern int gb32960_getsuspendSt(void);
extern unsigned char gb32960_PowerOffSt(void);

extern uint8_t gb_data_vehicleState(void);
extern long gb_data_vehicleSOC(void);
extern long gb_data_vehicleOdograph(void);
extern long gb_data_vehicleSpeed(void);
extern uint8_t gb_data_doorlockSt(void);
extern uint8_t gb_data_reardoorSt(void);
extern int gb_data_LHTemp(void);
extern uint8_t gb_data_chargeSt(void);
extern uint8_t gb_data_reardoorlockSt(void);
extern uint8_t gb_data_ACMode(void);
extern uint8_t gb_data_ACOnOffSt(void);
extern uint8_t gb_data_chargeOnOffSt(void);
extern uint8_t gb_data_BlowerGears(void);
extern uint8_t gb_data_outTemp(void);
extern uint8_t gb_data_InnerTemp(void);
extern uint8_t gb_data_CanbusActiveSt(void);
extern uint8_t gb_data_CrashOutputSt(void);
extern unsigned char gb32960_vinValidity(void);
extern void gb32960_getvin(char* vin);
extern uint8_t gb_data_ACTemperature(void);
extern uint8_t gb_data_TwinFlashLampSt(void);
extern uint8_t gb_data_PostionLampSt(void);
extern uint8_t gb_data_NearLampSt(void);
extern uint8_t gb_data_HighbeamLampSt(void);
extern uint8_t gb_data_frontRightTyrePre(void);
extern uint8_t gb_data_frontLeftTyrePre(void);
extern uint8_t gb_data_rearRightTyrePre(void);
extern uint8_t gb_data_rearLeftTyrePre(void);
extern uint8_t gb_data_frontRightTyreTemp(void);
extern uint8_t gb_data_frontLeftTyreTemp(void);
extern uint8_t gb_data_rearRightTyreTemp(void);
extern uint8_t gb_data_rearLeftTyreTemp(void);
extern uint8_t gb_data_gearPosition(void);
extern uint16_t  gb_data_insulationResistance(void);
extern uint8_t gb_data_acceleratePedalPrc(void);
extern uint8_t gb_data_deceleratePedalPrc(void);
extern uint16_t gb_data_batteryVoltage(void);
extern uint16_t gb_data_batteryCurrent(void);
extern uint8_t gb_data_powermode(void);
extern uint8_t gb_data_chargestatus(void);
extern int gb32960_networkSt(void);
extern long gb_data_ResidualOdometer(void);
extern long gb_data_ACChargeRemainTime(void);
extern int gb32960_gbLogoutSt(void);
extern uint8_t gb_data_dcdcstatus(void);
extern uint16_t gb_data_trip(void);
extern char getgb_data_warnSt(void);
extern long getgb_data_bdmsystemfailure(void);
extern long getgb_data_backDoorAjarSt(void);
extern long getgb_data_LFDoorOpenSt(void);
extern long getgb_data_RFDoorOpenSt(void);
extern long getgb_data_LRDoorOpenSt(void);
extern long getgb_data_RRDoorOpenSt(void);
extern long getgb_data_CLMLHTemp(void);
#endif
