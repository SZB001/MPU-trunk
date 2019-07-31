#ifndef __GB32960_API_H__
#define __GB32960_API_H__


#define GB32960_API_FAULTNUM 103

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
#define batt12vlowvolWARN				32//12V 蓄电池电压过低 报警
#define epsfaultWARN					33//EPS 故障 报警
#define epstorquesensorWARN				34//EPS 扭矩传感器信号故障报警
#define mcuIGBTVovercurrentWARN			35//MCU IGBT 驱动电路过流故障（V 相） 报警
#define mcuIGBTWovercurrentWARN			36//MCU IGBT 驱动电路过流故障（W 相） 报警
#define mcupowermoduleWARN				37//MCU 电源模块故障 报警
#define mcuIGBTUovertempWARN			38//MCU 内部 IGBT 过温（U 相） 报警
#define mcuIGBTUdrivecircuitWARN		39//MCU 内部 IGBT 驱动电路报警（U 相）报警
#define mcupossensorWARN				40//MCU 位置传感器检测回路故障报警
#define mcuhardwareUflowWARN			41//MCU 相电流硬件过流（U 相）报警
#define mcuDCbusovervolWARN				42//MCU 直流母线过压 报警
#define mcuDCbuslowvolWARN				43//MCU 直流母线欠压报警
#define MSDmainsafecutoffWARN			44//MSD 主保险断路故障 报警
#define tboxfaultWARN					45//TBOX 故障报警
#define SRSmoduleWARN					46//安全气囊模块异常报警
#define carchargeroverloadWARN  		47//车载充电器过载报警
#define carchargerundervolWARN			48//车载充电器欠压报警
#define bigcreeninfonotconsWARN			49//大屏信息版本信息不一致报警
#define gearsfaultWARN					50//档位故障报警
#define gearssignfaultWARN				51//档位信号故障报警
#define battmanagesysmissingWARN		52//电池管理系统丢失故障报警
#define battheatsfastWARN				53//电池升温过快报警
#define BatttempdiffWARN				54//电池温度差报警
#define motorctrlIGBTfaultWARN			55//电机控制器 IGBT 故障 报警
#define motorctrlmissingWARN			56//电机控制器丢失故障报警
#define motorctrlinterlockWARN			57//电机控制器环路互锁报警
#define motorctrlundervloWARN			58//电机控制器欠压故障 报警
#define motorabnormalWARN				59//电机异常报警
#define pwrbattcellovervolWARN			60//动力电池单体过压报警
#define pwrbattcellovervolproWARN		61//动力电池单体电压过压保护
#define pwrbattcellundervolproWARN		62//动力电池单体电压欠压保护故障报警
#define pwrbattcellundervolWARN			63//动力电池单体欠压报警
#define pwrbattsoclowWARN				64//动力电池电量过低报警
#define pwrbattvolimbalanceproWARN		65//动力电池电压不均衡保护故障
#define pwrbattinterlockWARN			66//动力电池环路互锁
#define pwrbattinsolateWARN				67//动力电池绝缘报警
#define pwrbattinsolatelowWARN			68//动力电池绝缘过低
#define pwrbattheatupfastWARN			69//动力电池升温过快
#define pwrbattovertempproWARN			70//动力电池温度过高保护故障
#define pwrbattundervolWARN				71//动力蓄电池包欠压报警
#define rearfoglampWARN					72//后雾灯故障
#define accpedalsignalovershootWARN		73//加速踏板信号超幅错误
#define accpedalsignalfaultWARN			74//加速踏板信号故障
#define insolatelowWARN					75//绝缘电阻低
#define acunworkWARN					76//空调不工作报警
#define acfanunworkWARN					77//空调风扇不工作
#define acinsolateWARN					78//空调绝缘报警
#define actempdiffWARN					79//空调制冷、制热温差报警
#define motorCANfaultWARN				80//驱动电机 CAN 通讯故障
#define motoroverheatfaultWARN			81//驱动电机过热故障
#define motorinsolateWARN				82//驱动电机绝缘报警
#define motormcuoverheatWARN			83//驱动电机控制器 MCU 过热故障
#define motorphasecurrsensorWARN		84//驱动电机相电流传感器故障
#define motorphasecurrovercurrWARN		85//驱动电机相电流过流故障
#define motorrotatingtransfaultWARN		86//驱动电机旋转变压器故障
#define motordcbusovervolfaultWARN		87//驱动电机直流母线过压故障
#define motordcundervolWARN				88//驱动电机直流欠压报警
#define motorovertempWARN				89//驱动电温度高报警
#define copperbarlooseWARN				90//铜排松动故障
#define BMSmissingWARN					91//与 BMS 通讯丢失
#define MCUmissingWARN					92//与 MCU 通讯丢失
#define prechargeresistancebreakWARN	93//预充电电阻断路故障
#define preresistancebreakWARN			94//预充电阻断路故障
#define vacpumpconsrotationfaultWARN	95//真空泵常转故障
#define abnormalvehicleheatWARN			96//整车加热工程异常
#define brakesysvacpumppresfaultWARN	97//制动系统真空泵压力故障
#define brakeassistlowvacuumfaultWARN	98//制动助力系统低真空度故障
#define brakpwrvacuumsensorfaultWARN	99//制动助力系统真空度传感器故障
#define VMSnodecommmissingWARN			100//子板 VMS 节点通讯丢失
#define LRbrakelightsmalfaultWARN		101//左右刹车灯故障
#define LRnearlampfaultWARN				102//左右近光灯故障

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
		unsigned char MSDmainsafecutoffwarn;//MSD 主保险断路故障 报警
		unsigned char tboxfaultwarn;//TBOX 故障报警
		unsigned char SRSmodulewarn;//安全气囊模块异常报警
		unsigned char carchargeroverloadwarn ;//车载充电器过载报警
		unsigned char carchargerundervolwarn;//车载充电器欠压报警
		unsigned char bigcreeninfonotconswarn;//大屏信息版本信息不一致报警
		unsigned char gearsfaultwarn;//档位故障报警
		unsigned char gearssignfaultwarn;//档位信号故障报警
		unsigned char battmanagesysmissingwarn;//电池管理系统丢失故障报警
		unsigned char battheatsfastwarn;//电池升温过快报警
		unsigned char Batttempdiffwarn;//电池温度差报警
		unsigned char motorctrlIGBTfaultwarn;//电机控制器 IGBT 故障 报警
		unsigned char motorctrlmissingwarn;//电机控制器丢失故障报警
		unsigned char motorctrlinterlockwarn;//电机控制器环路互锁报警
		unsigned char motorctrlundervlowarn;//电机控制器欠压故障 报警
		unsigned char motorabnormalwarn;//电机异常报警
		unsigned char pwrbattcellovervolwarn;//动力电池单体过压报警
		unsigned char pwrbattcellovervolprowarn;//动力电池单体电压过压保护
		unsigned char pwrbattcellundervolprowarn;//动力电池单体电压欠压保护故障报警
		unsigned char pwrbattcellundervolwarn;//动力电池单体欠压报警
		unsigned char pwrbattsoclowwarn;//动力电池电量过低报警
		unsigned char pwrbattvolimbalanceprowarn;//动力电池电压不均衡保护故障
		unsigned char pwrbattinterlockwarn;//动力电池环路互锁
		unsigned char pwrbattinsolatewarn;//动力电池绝缘报警
		unsigned char pwrbattinsolatelowwarn;//动力电池绝缘过低
		unsigned char pwrbattheatupfastwarn;//动力电池升温过快
		unsigned char pwrbattovertempprowarn;//动力电池温度过高保护故障
		unsigned char pwrbattundervolwarn;//动力蓄电池包欠压报警
		unsigned char rearfoglampwarn;//后雾灯故障
		unsigned char accpedalsignalovershootwarn;//加速踏板信号超幅错误
		unsigned char accpedalsignalfaultwarn;//加速踏板信号故障
		unsigned char insolatelowwarn;//绝缘电阻低
		unsigned char acunworkwarn;//空调不工作报警
		unsigned char acfanunworkwarn;//空调风扇不工作
		unsigned char acinsolatewarn;//空调绝缘报警
		unsigned char actempdiffwarn;//空调制冷、制热温差报警
		unsigned char motorCANfaultwarn;//驱动电机 CAN 通讯故障
		unsigned char motoroverheatfaultwarn;//驱动电机过热故障
		unsigned char motorinsolatewarn;//驱动电机绝缘报警
		unsigned char motormcuoverheatwarn;//驱动电机控制器 MCU 过热故障
		unsigned char motorphasecurrsensorwarn;//驱动电机相电流传感器故障
		unsigned char motorphasecurrovercurrwarn;//驱动电机相电流过流故障
		unsigned char motorrotatingtransfaultwarn;//驱动电机旋转变压器故障
		unsigned char motordcbusovervolfaultwarn;//驱动电机直流母线过压故障
		unsigned char motordcundervolwarn;//驱动电机直流欠压报警
		unsigned char motorovertempwarn;//驱动电温度高报警
		unsigned char copperbarloosewarn;//铜排松动故障
		unsigned char BMSmissingwarn;//与 BMS 通讯丢失
		unsigned char MCUmissingwarn;//与 MCU 通讯丢失
		unsigned char prechargeresistancebreakwarn;//预充电电阻断路故障
		unsigned char preresistancebreakwarn;//预充电阻断路故障
		unsigned char vacpumpconsrotationfaultwarn;//真空泵常转故障
		unsigned char abnormalvehicleheatwarn;//整车加热工程异常
		unsigned char brakesysvacpumppresfaultwarn;//制动系统真空泵压力故障
		unsigned char brakeassistlowvacuumfaultwarn;//制动助力系统低真空度故障
		unsigned char brakpwrvacuumsensorfaultwarn;//制动助力系统真空度传感器故障
		unsigned char VMSnodecommmissingwarn;//子板 VMS 节点通讯丢失
		unsigned char LRbrakelightsmalfaultwarn;//左右刹车灯故障
		unsigned char LRnearlampfaultwarn;//左右近光灯故障
	}type; /**/
}gb32960_api_fault_t;

extern gb32960_api_fault_t gb_fault;

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
extern uint8_t gb_data_chargeGunCnctSt(void);
extern uint8_t gb_data_BlowerGears(void);
extern uint8_t gb_data_outTemp(void);
extern uint8_t gb_data_InnerTemp(void);
extern uint8_t gb_data_CanbusActiveSt(void);
extern uint8_t gb_data_CrashOutputSt(void);
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
extern uint8_t gb_data_chargestauus(void);
extern int gb32960_networkSt(void);
extern long gb_data_ResidualOdometer(void);
extern long gb_data_ACChargeRemainTime(void);
#endif
