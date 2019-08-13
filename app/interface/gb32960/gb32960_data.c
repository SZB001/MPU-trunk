#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "gps_api.h"
#include "gb32960.h"
#include "gb32960_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "../support/protocol.h"
#include "../hozon/PrvtProtocol/PrvtProt_SigParse.h"
#include "hozon_PP_api.h"

extern int PP_identificat_rcvdata(uint8_t *dt);
gb32960_api_fault_t gb_fault;

#define GB_EXT	1//���������չ��Ϣ

#define GB_MAX_PACK_CELL    800
#define GB_MAX_PACK_TEMP    800
#define GB_MAX_FUEL_TEMP    200
#define GB_MAX_FUEL_INFO    16
#define GB_MAX_VEHI_INFO    16
#define GB_MAX_WARN_INFO    (32 + 59)//33-91Ϊ��չ��������
#define GB_MAX_MOTOR_INFO   8
#define GB_MAX_ENGIN_INFO   4
#define GB_MAX_EXTR_INFO    16
#define GB_MAX_MOTOR        4

#if GB_EXT
/* event information index */
#define GB_EVT_EPBLAMP_ON       	0x00
#define GB_EVT_EPBWRONGLAMP_ON      0x01
#define GB_EVT_REARFOGLAMP_ON       0x02
#define GB_EVT_POSLAMP_ON         	0x03
#define GB_EVT_MAINBELTLAMP_ON     	0x04
#define GB_EVT_PASSBELTLAMP_ON     	0x05
#define GB_EVT_LEFTTURNLAMP_ON     	0x06
#define GB_EVT_RIGHTTURNLAMP_ON    	0x07
#define GB_EVT_NEARLAMP_ON       	0x08//�����
#define GB_EVT_HIGHBEAMLAMP_ON      0x09
#define GB_EVT_LEFTDRVDOOR_OPEN     0x0a
#define GB_EVT_RIGHTDRVDOOR_OPEN    0x0b
#define GB_EVT_LEFTREARDRVDOOR_OPEN      	0x0c
#define GB_EVT_RIGHTREARDRVDOOR_OPEN      	0x0d
#define GB_EVT_TAILDOOR_OPEN    			0x0e
#define GB_MAX_EVENT_INFO   (GB_EVT_TAILDOOR_OPEN + 1)

/* vehi state extend information index */
#define GB_VS_DRIDOORLOCKST       	0x00//��ʻ������״̬
#define GB_VS_PASSDOORLOCKST      	0x01
#define GB_VS_LRDOORLOCKST       	0x02
#define GB_VS_RRDOORLOCKST         	0x03
#define GB_VS_REARDDOORLOCKST     	0x04//��������״̬
#define GB_VS_DRIWINDOWST     		0x05
#define GB_VS_PASSWINDOWST     		0x06
#define GB_VS_LRWINDOWST    		0x07
#define GB_VS_RRWINDOWST       		0x08//
#define GB_VS_UPWINDOWST      		0x09//�촰״̬
#define GB_VS_DRIDOORST     		0x0a
#define GB_VS_PASSDOORST		    0x0b
#define GB_VS_LRDOORST		      	0x0c
#define GB_VS_RRDOORST		      	0x0d
#define GB_VS_BACKDOORST		    0x0e//������״̬
#define GB_VS_ACST		    		0x0f
#define GB_VS_ACTEMP		    	0x10
#define GB_VS_ACMODE		    	0x11
#define GB_VS_AIRVOLUME		    	0x12
#define GB_VS_INTEMP		    	0x13
#define GB_VS_OUTTEMP		   		0x14
#define GB_VS_HLAMPST		    	0x15//˫��״̬
#define GB_VS_SLAMPST		    	0x16
#define GB_VS_NEARLAMPST		    0x17
#define GB_VS_HEADLIGHTST		    0x18
#define GB_VS_LTURNLAMPST		    0x19
#define GB_VS_RTURNLAMPST		    0x1A
#define GB_VS_BRAKELAMPST		    0x1B
#define GB_VS_ATMOSPHERELAMPST		0x1C
#define GB_VS_RFTYRETEMP		    0x1D
#define GB_VS_RFTYREPRESSURE		0x1E
#define GB_VS_LFTYRETEMP		    0x1F
#define GB_VS_LFTYREPRESSURE		0x20
#define GB_VS_RRTYRETEMP		    0x21
#define GB_VS_RRTYREPRESSURE		0x22
#define GB_VS_LRTYRETEMP		    0x23
#define GB_VS_LRTYREPRESSURE		0x24
#define GB_VS_REMAINCHRGTIME		0x25
#define GB_VS_FIXTIMECHARGEST		0x26//��ʱ���״̬
#define GB_VS_FIXSTARTCHRG_HOUR		0x27
#define GB_VS_FIXSTARTCHRG_MIN		0x28
#define GB_VS_FSCHARGEST		    0x29
#define GB_VS_CELLNETST		    	0x2A
#define GB_VS_CELLNETSIGN		    0x2B
#define GB_VS_CANST		    		0x2C
#define GB_VS_12VVOLTAGE		    0x2D//tbox
#define GB_VS_SOC		    		0x2E
#define GB_VS_ENDURANCEMILE		    0x2F
#define GB_VS_DRIVMODE		    	0x30
#define GB_VS_PARKST		    	0x31
#define GB_VS_STARTST		    	0x32
#define GB_VS_ASPEED_X				0x33
#define GB_VS_ASPEED_Y				0x34
#define GB_VS_ASPEED_Z				0x35
#define GB_VS_FLTYRERSPEED			0x36
#define GB_VS_FRTYRERSPEED			0x37
#define GB_VS_RLTYRERSPEED			0x38
#define GB_VS_RRTYRERSPEED			0x39
#define GB_VS_STEERWHEELANGLE		0x3A
#define GB_VS_TRIP					0x3B
#define GB_VS_SUBTOTALTRVLTIME		0x3C
#define GB_VS_TIPC					0x3D//˲ʱ���
#define GB_VS_TAPC					0x3E//ƽ�����
#define GB_VS_SUBPC					0x3F
#define GB_VS_ESCACTIVEST			0x40
#define GB_VS_ESCDISABLEST			0x41
#define GB_VS_TCSACTIVEST			0x42
#define GB_VS_SASCAIL				0x43
#define GB_VS_MAINDRIBELTST			0x44
#define GB_VS_PASSDRIBELTST			0x45
#define GB_VS_ELECSTOPBRAKEST		0x46
#define GB_VS_RESERVE_ONE    		0x47
#define GB_VS_RESERVE_TWO    		0x48
#define GB_VS_THREE    				0x49
#define GB_VS_FOUR    				0x4A
#define GB_MAX_VSE_INFO   (GB_VS_FOUR + 1)


/* components and parts state information index */
#define GB_CMPT_MTRTARGETTORQUE       	0x00//
#define GB_CMPT_MTRTARGETSPEED       	0x01//
#define GB_CMPT_SYSST       			0x02//
#define GB_CMPT_PWRUPST       			0x03//
#define GB_CMPT_PWRDWNST       			0x04//
#define GB_CMPT_FANGEARSTS       		0x05//
#define GB_CMPT_PTCWORKSTS       		0x06//
#define GB_CMPT_MOTCIRCWTRPUMWRKST      0x07//
#define GB_CMPT_BATCIRCWTRPUMWRKST      0x08//
#define GB_CMPT_CRASHOUTPUTST       	0x09//
#define GB_CMPT_WORKMODE       			0x0A//
#define GB_CMPT_CTRLTORQUE       		0x0B//���ʵ��Ť��
#define GB_CMPT_MTRSENSEROTA       		0x0C//
#define GB_CMPT_MAXAVAILTORQUE       	0x0D//
#define GB_CMPT_POSBATTCONTST       	0x0E//
#define GB_CMPT_NEGBATTCONTST       	0x0F//
#define GB_CMPT_PRECHARGECONTST       	0x10//
#define GB_CMPT_DCCHARGECONTST       	0x11//
#define GB_CMPT_SIGNALBATHIGHESTVOLT    0x12//
#define GB_CMPT_SIGNALBATLOWESTVOLT     0x13//
#define GB_CMPT_SIGNALDIFFPRESSURE      0x14//
#define GB_CMPT_BATHIGHESTTEMP       	0x15//
#define GB_CMPT_BATLOWESTTEMP       	0x16//
#define GB_CMPT_SIGNALDIFFTEMP       	0x17//
#define GB_CMPT_CHARGECCSIG       		0x18//
#define GB_CMPT_CHARGECPSIG       		0x19//
#define GB_CMPT_QUICHGCC       			0x1A//
#define GB_CMPT_POSFASTCHGPORTTEMP      0x1B//
#define GB_CMPT_ENGFASTCHGPORTTEMP      0x1C//
#define GB_CMPT_POSSLOWCHGPORTTEMP      0x1D//
#define GB_CMPT_NEGSLOWCHGPORTTEMP      0x1E//
#define GB_CMPT_CHARGEST     			0x1F//��份��״̬
#define GB_CMPT_PWRBATTHEATST       	0x20//
#define GB_CMPT_BATTVOLTAGE       		0x21//
#define GB_CMPT_HDSAHTOTALCPSUM      	0x22//
#define GB_CMPT_HDSAHACTIVECPSUM       	0x23//
#define GB_CMPT_12VBATTVOLT       		0x24//
#define GB_CMPT_REQOUTPUTMODE       	0x25//
#define GB_CMPT_FCCURRENTREQ       		0x26//
#define GB_CMPT_FCVOLTREQ       		0x27//
#define GB_CMPT_CHRGGUNCNCTLI       	0x28//
#define GB_CMPT_CURRENABLEPWROUTMAX     0x29//
#define GB_CMPT_CHARGEOUTVOLT       	0x2A//
#define GB_CMPT_CHARGEOUTCURR       	0x2B//
#define GB_CMPT_CHARGEINPCURR       	0x2C//
#define GB_CMPT_CHARGEINPVOLT       	0x2D//
#define GB_CMPT_CHARGEMTRWORKST       	0x2E//
#define GB_CMPT_S2ST       				0x2F//
#define GB_CMPT_ACWORKSTATE       		0x30//
#define GB_CMPT_CYCLEMODE       		0x31//
#define GB_CMPT_VENTMODE       			0x32//
#define GB_CMPT_FOOTMODE       			0x33//
#define GB_CMPT_WINDOWMODE       		0x34//
#define GB_CMPT_LHTEMP       			0x35//
#define GB_CMPT_RHTEMP       			0x36//
#define GB_CMPT_ACSTS       			0x37//
#define GB_CMPT_PTCWORKST       		0x38//
#define GB_CMPT_BLOWERSPDST       		0x39//
#define GB_CMPT_OUTSIDETEMP       		0x3A//
#define GB_CMPT_INSIDETEMP       		0x3B//
#define GB_CMPT_EACBASEST       		0x3C//
#define GB_CMPT_EACSPEEDSET       		0x3D//
#define GB_CMPT_EACACPSPEED       		0x3E//
#define GB_CMPT_EACHIGHVOLT       		0x3F//
#define GB_CMPT_PTCPWRCONS       		0x40//
#define GB_CMPT_CLNTTEMPIN       		0x41//
#define GB_CMPT_CLNTTEMPOUT       		0x42//
#define GB_CMPT_KEYAUTHEST       		0x43//
#define GB_CMPT_IDDEVICENO       		0x44//
#define GB_CMPT_VCULEARNST       		0x45//
#define GB_CMPT_RAINSENSOR       		0x46//
#define GB_CMPT_RESERVE_1       		0x47//
#define GB_CMPT_RESERVE_2       		0x48//
#define GB_CMPT_RESERVE_3       		0x49//
#define GB_CMPT_RESERVE_4       		0x4A//
#define GB_MAX_CMPT_INFO   (GB_CMPT_RESERVE_4 + 1)

#endif

/* alarm fault information index */
//�ɳ�索��װ�ù���
#define GB_AF_BATTRISEFAST       		0x00//
// ��������
#define GB_AF_FLTYRESENSORLOST       	0x01//
#define GB_AF_FLTYRELEAK       			0x02//
#define GB_AF_FLTYRELOWPRESSUREWARN     0x03//
#define GB_AF_FLTYREHIGHPRESSUREWARN    0x04//
#define GB_AF_FLTYREHIGHTEMPWARN       	0x05//
#define GB_AF_FLTYRELOWBATT       		0x06//

#define GB_AF_FRTYRESENSORLOST       	0x07//
#define GB_AF_FRTYRELEAK       			0x08//
#define GB_AF_FRTYRELOWPRESSUREWARN     0x09//
#define GB_AF_FRTYREHIGHPRESSUREWARN    0x0A//
#define GB_AF_FRTYREHIGHTEMPWARN       	0x0B//
#define GB_AF_FRTYRELOWBATT       		0x0C//

#define GB_AF_RLTYRESENSORLOST       	0x0D//
#define GB_AF_RLTYRELEAK       			0x0E//
#define GB_AF_RLTYRELOWPRESSUREWARN     0x0F//
#define GB_AF_RLTYREHIGHPRESSUREWARN    0x10//
#define GB_AF_RLTYREHIGHTEMPWARN       	0x11//
#define GB_AF_RLTYRELOWBATT       		0x12//

#define GB_AF_RRTYRESENSORLOST       	0x13//
#define GB_AF_RRTYRELEAK       			0x14//
#define GB_AF_RRTYRELOWPRESSUREWARN     0x15//
#define GB_AF_RRTYREHIGHPRESSUREWARN    0x16//
#define GB_AF_RRTYREHIGHTEMPWARN       	0x17//
#define GB_AF_RRTYRELOWBATT       		0x18//
#define GB_MAX_AF_INFO   (GB_AF_RRTYRELOWBATT + 1)

/* vehicle type */
#define GB_VEHITYPE_ELECT   0x01
#define GB_VEHITYPE_HYBIRD  0x02
#define GB_VEHITYPE_GASFUEL 0x03

/* vehicle information index */
#define GB_VINF_STATE       0x00
#define GB_VINF_CHARGE      0x01
#define GB_VINF_VEHIMODE    0x02
#define GB_VINF_SPEED       0x03
#define GB_VINF_ODO         0x04
#define GB_VINF_VOLTAGE     0x05
#define GB_VINF_CURRENT     0x06
#define GB_VINF_SOC         0x07
#define GB_VINF_DCDC        0x08
#define GB_VINF_SHIFT       0x09
#define GB_VINF_INSULAT     0x0a
#define GB_VINF_ACCPAD      0x0b
#define GB_VINF_BRKPAD      0x0c
#define GB_VINF_MAX         GB_VINF_VEHIMODE + 1

/* motor information index */
#define GB_MINF_STATE       0x00
#define GB_MINF_MCUTMP      0x01
#define GB_MINF_SPEED       0x02
#define GB_MINF_TORQUE      0x03
#define GB_MINF_MOTTMP      0x04
#define GB_MINF_VOLTAGE     0x05
#define GB_MINF_CURRENT     0x06
#define GB_MINF_MAX         GB_MINF_CURRENT + 1

/* fuel cell information index */
#define GB_FCINF_VOLTAGE    0x00
#define GB_FCINF_CURRENT    0x01
#define GB_FCINF_RATE       0x02
#define GB_FCINF_MAXTEMP    0x03
#define GB_FCINF_MAXTEMPID  0x04
#define GB_FCINF_MAXCCTT    0x05
#define GB_FCINF_MAXCCTTID  0x06
#define GB_FCINF_MAXPRES    0x07
#define GB_FCINF_MAXPRESID  0x08
#define GB_FCINF_HVDCDC     0x09
#define GB_FCINF_MAX        GB_FCINF_HVDCDC + 1
#define GB_FCINF_TEMPTAB    0x100


/* engine information index */
#define GB_EINF_STATE       0x00
#define GB_EINF_SPEED       0x01
#define GB_EINF_FUELRATE    0x02
#define GB_EINF_MAX         GB_EINF_FUELRATE + 1

/* extremum index */
#define GB_XINF_MAXVPID     0x00
#define GB_XINF_MAXVCID     0x01
#define GB_XINF_MAXV        0x02
#define GB_XINF_MINVPID     0x03
#define GB_XINF_MINVCID     0x04
#define GB_XINF_MINV        0x05
#define GB_XINF_MAXTPID     0x06
#define GB_XINF_MAXTCID     0x07
#define GB_XINF_MAXT        0x08
#define GB_XINF_MINTPID     0x09
#define GB_XINF_MINTCID     0x0a
#define GB_XINF_MINT        0x0b
#define GB_XINF_MAX         GB_XINF_MINT + 1

/* battery information index */
#define GB_BINF_VOLTAGE     0x3fe
#define GB_BINF_CURRENT     0x3ff

/* GB32960 data type */
#define GB_DATA_VEHIINFO    0x01
#define GB_DATA_MOTORINFO   0x02
#define GB_DATA_ENGINEINFO  0x04
#define GB_DATA_LOCATION    0x05
#define GB_DATA_EXTREMA     0x06
#define GB_DATA_WARNNING    0x07
#define GB_DATA_BATTVOLT    0x08
#define GB_DATA_BATTTEMP    0x09
#define GB_DATA_FUELCELL    0x0A
#define GB_DATA_VIRTUAL     0x0B
#if GB_EXT
#define GB_DATA_EVENT     	0x0C//�¼�����
#define GB_DATA_CONPST     	0x0E//�㲿��״̬����
#define GB_DATA_VSEXT     	0x0F//����״̬��չ����

#endif
#define GB_DATA_ALARMFAULT  0x05//����-���ϴ���

/* report data type */
#define GB_RPTTYPE_REALTM   0x02
#define GB_RPTTYPE_DELAY    0x03

/* report packets parameter */
#define GB_MAX_REPORT       2000

/* battery information structure */
typedef struct
{
    int voltage;
    int current;
    int cell[GB_MAX_PACK_CELL];
    int temp[GB_MAX_PACK_TEMP];
    uint32_t   cell_cnt;
    uint32_t   temp_cnt;
}gb_batt_t;
/* motor information structure */
typedef struct
{
    int info[GB_MAX_MOTOR_INFO];
    uint8_t state_tbl[256];
}gb_motor_t;

/* vehicle information structure */
typedef struct
{
    int info[GB_MAX_VEHI_INFO];
    uint8_t state_tbl[256];
    uint8_t mode_tbl[256];
    char    shift_tbl[256];
    uint8_t charge_tbl[256];
    uint8_t dcdc_tbl[256];
    uint8_t vehi_type;
}gb_vehi_t;

#if GB_EXT
/* event information structure */
typedef struct
{
    int info[GB_MAX_EVENT_INFO];
    uint8_t oldst[GB_MAX_EVENT_INFO];
	uint8_t newst[GB_MAX_EVENT_INFO];
	uint8_t triflg;
}gb_event_t;


typedef struct
{
    uint8_t type;
    uint16_t code;
}gb_eventCode_t;

static gb_eventCode_t	gb_eventCode[GB_MAX_EVENT_INFO] =
{
	{GB_EVT_EPBLAMP_ON,0x0001},
	{GB_EVT_EPBWRONGLAMP_ON,0x0002},
	{GB_EVT_REARFOGLAMP_ON,0x0003},
	{GB_EVT_POSLAMP_ON,0x0004},
	{GB_EVT_MAINBELTLAMP_ON,0x0005},
	{GB_EVT_PASSBELTLAMP_ON,0x0006},
	{GB_EVT_LEFTTURNLAMP_ON,0x0007},
	{GB_EVT_RIGHTTURNLAMP_ON,0x0008},
	{GB_EVT_NEARLAMP_ON,0x0009},
	{GB_EVT_HIGHBEAMLAMP_ON,0x0009},
	{GB_EVT_LEFTDRVDOOR_OPEN,0x000A},
	{GB_EVT_RIGHTDRVDOOR_OPEN,0x000B},
	{GB_EVT_LEFTREARDRVDOOR_OPEN,0x000C},
	{GB_EVT_RIGHTREARDRVDOOR_OPEN,0x000D},
	{GB_EVT_TAILDOOR_OPEN,0x000E}
};


/* vehi state information structure */
typedef struct
{
    int info[GB_MAX_VSE_INFO];
    uint8_t oldst[GB_MAX_VSE_INFO];
	uint8_t newst[GB_MAX_VSE_INFO];
}gb_VehiStExt_t;

/* �㲿�� state information structure */
typedef struct
{
    int info[GB_MAX_CMPT_INFO];
    uint8_t oldst[GB_MAX_CMPT_INFO];
	uint8_t newst[GB_MAX_CMPT_INFO];
}gb_ConpState_t;

typedef struct
{
    uint16_t code;
}gb_alarmCode_t;

static gb_alarmCode_t	gb_alarmCode[GB_MAX_WARN_INFO] =
{
	{0x0000},//0~31,国标告警
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0000},
	{0x0001},//国标扩展告警
	{0x0002},
	{0x0003},
	{0x0004},
	{0x0005},
	{0x0006},
	{0x0007},
	{0x0008},
	{0x0009},
	{0x000A},
	{0x000B},
	{0x000C},
	{0x000D},
	{0x000E},
	{0x000F},
	{0x0010},
	{0x0010},
	{0x0011},
	{0x0012},
	{0x0013},
	{0x0014},
	{0x0015},
	{0x0016},//���س����Ƿѹ�澯
	//{0x0016},//���س����Ƿѹ�澯
	{0x0017},
	{0x0018},
	{0x0019},
	{0x001A},//电机异常告警
	{0x001B},//动力电池单体电压过压保护
	{0x001C},//动力电池单体电压欠压保护故障
	{0x001D},
	{0x001E},
	{0x001F},
	{0x0020},//动力电池温度过高保护故障
	{0x0021},
	{0x0022},
	{0x0023},
	{0x0024},
	{0x0025},
	{0x0026},//����쳣�澯
	{0x0027},
	{0x0028},
	{0x0029},
	{0x002A},
	{0x002B},
	{0x002C},
	{0x002D},
	{0x002E},
	{0x002F},
	{0x0030},
	{0x0031},
	{0x0032},
	{0x0033},
	{0x0034},
	{0x0035},
	{0x0036},
	{0x0037},
	{0x0038},
	{0x0039},
	{0x0026}//����쳣�澯
};

#endif

/* ����-���� information structure */
typedef struct
{
    int info[GB_MAX_AF_INFO];
    uint8_t oldst[GB_MAX_AF_INFO];
	uint8_t newst[GB_MAX_AF_INFO];
}gb_alarmFault_t;

typedef struct
{
    uint16_t code;
}gb_alarmFaultCode_t;

static gb_alarmFaultCode_t gb_alarmFaultCode[GB_MAX_AF_INFO] =
{
		{1001},

		{4001},
		{4002},
		{4003},
		{4004},
		{4005},
		{4006},
		{4007},
		{4008},
		{4009},
		{4010},
		{4011},
		{4012},
		{4013},
		{4014},
		{4015},
		{4016},
		{4017},
		{4018},
		{4019},
		{4020},
		{4021},
		{4022},
		{4023},
		{4024}
};

/* fuel cell information structure */
typedef struct
{
    int info[GB_MAX_FUEL_INFO];
    int temp[GB_MAX_FUEL_TEMP];
    int temp_cnt;
    uint8_t hvdcdc[8];
}gb_fuelcell_t;

/* engine information structure */
typedef struct
{
    int info[GB_MAX_ENGIN_INFO];
    uint8_t state_tbl[256];
}gb_engin_t;
/* GB32960 information structure */
typedef struct _gb_info_t
{
    gb_vehi_t  vehi;
    gb_motor_t motor[GB_MAX_MOTOR];
    uint32_t   motor_cnt;
    gb_fuelcell_t fuelcell;
    gb_engin_t engin;
    gb_batt_t  batt;
    int warn[4][GB_MAX_WARN_INFO];//index 3,as a relevance channel
    int extr[GB_MAX_EXTR_INFO];
    int warntrig;
    int warntest;
#if GB_EXT
	gb_event_t event;
	gb_VehiStExt_t gb_VSExt;
	gb_ConpState_t gb_ConpSt;
#endif
	gb_alarmFault_t gb_alarmFault;
    struct _gb_info_t *next;
}gb_info_t;

gb32960_api_extwarn_indextable_t	gb32960_api_extwarn_indextable[41] =
{
	{32,32},//12v蓄电池电压过低
	{33,33},//EPS 故障 
	{34,34},//EPS 扭矩传感器信号故障
	{35,35},//MCU IGBT 驱动电路过流故障（V 相）
	{36,36},//MCU IGBT 驱动电路过流故障（W 相）
	{37,37},//MCU 电源模块故障 
	{38,38},//MCU 内部 IGBT 过温（U 相）
	{39,39},//MCU 内部 IGBT 驱动电路报警（U 相）
	{40,40},//MCU 位置传感器检测回路故障
	{41,41},//MCU 相电流硬件过流（U 相） 
	{42,42},//MCU 直流母线过压 
	{43,43},//MCU 直流母线欠压 
	//{0x0,44},//msd主保险断路故障
	{44,45},//tbox故障报警
	{45,46},//安全气囊模块异常报警 
	{46,47},//车载充电器过载报警
	{47,48},//车载充电器欠压报警
	{49,49},//大屏信息版本信息不一致报警
	{50,60},//单体蓄电池过压报警 
	{51,50},//档位故障 
	{52,51},//档位信号故障 
	{53,52},//电池管理系统丢失故障 
	{54,53},//电池升温过快 
	{55,55},//电机控制器 IGBT 故障
	{56,57},//电机控制器环路互锁
	{57,58},//电机控制器欠压故障
	{58,59},//电机异常报警
	{59,61},//动力电池单体电压过压保护
	{60,62},//动力电池单体电压欠压保护故障
	{61,64},//动力电池电量过低报警
	{62,65},//动力电池电压不均衡保护故障
	{63,66},//动力电池环路互锁
	{64,70},//动力电池温度过高保护故障
	{65,73},//加速踏板信号超幅错误
	{66,74},//加速踏板信号故障
	{67,77},//空调风扇不工作
	{68,80},//驱动电机 CAN 通讯故障
	{69,91},//与 BMS 通讯丢失
	{70,92},//与 MCU 通讯丢失
	{71,96},//整车加热工程异常
	{73,101},//左右刹车灯故障
	//{74,},
	//{75,},
	//{76,},
	//{77,},
	{79,76},//空调不工作报警
};

static gb_info_t  gb_infmem[2];
static gb_info_t *gb_inf;
static gb_pack_t  gb_datamem[GB_MAX_REPORT];
static gb_pack_t  gb_errmem[(GB_MAX_PACK_CELL + 199) / 200 * 30];
static list_t     gb_free_lst;
static list_t     gb_realtm_lst;
static list_t     gb_delay_lst;
static list_t     gb_trans_lst;
static list_t    *gb_errlst_head;
static int        gb_warnflag;
static int        gb_pendflag;
static pthread_mutex_t gb_errmtx;
static pthread_mutex_t gb_datmtx;
static uint16_t gb_datintv;

#define ERR_LOCK()          pthread_mutex_lock(&gb_errmtx)
#define ERR_UNLOCK()        pthread_mutex_unlock(&gb_errmtx)
#define DAT_LOCK()          pthread_mutex_lock(&gb_datmtx)
#define DAT_UNLOCK()        pthread_mutex_unlock(&gb_datmtx)
#define GROUP_SIZE(inf)     RDUP_DIV((inf)->batt.cell_cnt, 200)


static uint8_t gb_engineSt = 2;//Ϩ��
static long    gb_totalOdoMr = 0;//�ܼ����
static long    gb_vehicleSOC = 0;//����
static long    gb_vehicleSpeed = 0;//�ٶ�
static uint8_t gb_chargeSt = 0;//
static int canact = 0;

#if GB_EXT
/* event report */
static void gb_data_eventReport(gb_info_t *gbinf,  uint32_t uptime)
{
    uint32_t len = 0;
	int i;
	uint8_t buf[1024];
	uint8_t *eventcnt_ptr;
	RTCTIME time;
	
	DAT_LOCK();
	
    can_get_time(uptime, &time);
    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
    /* data type : event information */
    buf[len++] = 0x95;//event body type

    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;
	eventcnt_ptr = &buf[len++];
	*eventcnt_ptr = 0;
	
	for(i = 0;i < GB_MAX_EVENT_INFO;i++)
	{
		if (gbinf->event.info[i])
		{
			gbinf->event.newst[i] = dbc_get_signal_from_id(gbinf->event.info[i])->value;
			if(gbinf->event.newst[i])
			{
				if(gbinf->event.oldst[i] == 0)
				{
					gbinf->event.triflg = 1;
				}
				gbinf->event.oldst[i] = gbinf->event.newst[i];
				(*eventcnt_ptr) += 1;
				buf[len++] = gb_eventCode[i].code >> 8;
				buf[len++] = gb_eventCode[i].code;
			}
			else 
			{
				gbinf->event.oldst[i] = 0;
			}
		}
	}
	
	if(gbinf->event.triflg == 1)
	{	
		gb_pack_t *rpt;
		list_t *node;
		log_i(LOG_GB32960, "event trig.");
		gbinf->event.triflg = 0;
		if ((node = list_get_first(&gb_free_lst)) == NULL)
		{
			if ((node = list_get_first(&gb_delay_lst)) == NULL &&
					(node = list_get_first(&gb_realtm_lst)) == NULL)
			{
				/* it should not be happened */
				log_e(LOG_GB32960, "BIG ERROR: no buffer to use.");

				while (1);
			}
		}

		rpt = list_entry(node, gb_pack_t, link);
		rpt->len  = len;
		for(i = 0;i < len;i++)
		{
			rpt->data[i] = buf[i];
		}
		rpt->seq  = i + 1;
		rpt->list = &gb_realtm_lst;
		rpt->type = GB_RPTTYPE_REALTM;
		list_insert_before(&gb_realtm_lst, node);
	}
	DAT_UNLOCK();
}

#endif

static uint32_t gb_data_save_vehi(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : vehicle information */
    buf[len++] = GB_DATA_VEHIINFO;

    /* vehicle state */
    if (gbinf->vehi.info[GB_VINF_STATE])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_STATE])->value;
        buf[len++] = gbinf->vehi.state_tbl[tmp] ? gbinf->vehi.state_tbl[tmp] : 0xff;
        gb_engineSt = gbinf->vehi.state_tbl[tmp] ? gbinf->vehi.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* charge state */
    if (gbinf->vehi.info[GB_VINF_CHARGE])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_CHARGE])->value;
        buf[len++] = gbinf->vehi.charge_tbl[tmp] ? gbinf->vehi.charge_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* vehicle type */
    //if (gbinf->vehi.info[GB_VINF_VEHIMODE])
    //{
    //    tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_VEHIMODE])->value;
    //    buf[len++] = gbinf->vehi.mode_tbl[tmp] ? gbinf->vehi.mode_tbl[tmp] : 0xff;
    //}
    //else
    {
        buf[len++] = gbinf->vehi.vehi_type;
    }

    /* vehicle speed, scale 0.1km/h */
    tmp = gbinf->vehi.info[GB_VINF_SPEED] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SPEED])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;
    gb_vehicleSpeed = tmp;

    /* odograph, scale 0.1km */
    tmp = gbinf->vehi.info[GB_VINF_ODO] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_ODO])->value * 10 : 0xffffffff;
    buf[len++] = tmp >> 24;
    buf[len++] = tmp >> 16;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;
    gb_totalOdoMr = tmp;

    /* total voltage, scale 0.1V */
    tmp = gbinf->vehi.info[GB_VINF_VOLTAGE] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_VOLTAGE])->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total curr, scale 0.1V, offset -1000A */
    tmp = gbinf->vehi.info[GB_VINF_CURRENT] ?
          (dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_CURRENT])->value + 1000) * 10: 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total SOC */
    tmp = gbinf->vehi.info[GB_VINF_SOC] ?
          dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SOC])->value : 0xff;
    buf[len++] = tmp;
    gb_vehicleSOC = tmp;

    /* DCDC state */
    if (gbinf->vehi.info[GB_VINF_DCDC])
    {
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_DCDC])->value;
        buf[len++] = gbinf->vehi.dcdc_tbl[tmp] ? gbinf->vehi.dcdc_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* shift state */
    if (gbinf->vehi.info[GB_VINF_SHIFT])
    {
        uint8_t sft;
        tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SHIFT])->value;
        sft = gbinf->vehi.shift_tbl[tmp];

        switch (sft)
        {
            case '1'...'6':
			{
                tmp = sft - '0';
			}
            break;
            case 'R':
            {
                tmp = 13;
            }
            break;
            case 'S':
            case 'D':
            {
                tmp = 14;
            }
            break;
            case 'P':
            {
                tmp = 15;
            }
            break;
            case 'N':
            {
            	tmp = 0;
            }
            break;
            default:
            {
                tmp = 0;
            }
            break;
        }
        buf[len++] = tmp;
    }
    else
    {
    	buf[len++] = 0;
    }

	/* insulation resistance, scale 1k */
	tmp = gbinf->vehi.info[GB_VINF_INSULAT] ?
		  MIN(dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_INSULAT])->value, 60000) : 0xffff;
	buf[len++] = tmp >> 8;
	buf[len++] = tmp;

	/* accelate pad value */
	tmp = gbinf->vehi.info[GB_VINF_ACCPAD] ?
		  dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_ACCPAD])->value : 0xff;
	buf[len++] = tmp;

    /* break pad value */
	if(gbinf->vehi.info[GB_VINF_BRKPAD])
	{
		tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_BRKPAD])->value;
		if(tmp >= 100)
		{
			tmp = 100;
		}
	}
	else
	{
		tmp = 0xff;
	}
	buf[len++] = tmp;

    return len;
}

static uint32_t gb_data_save_cell(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp, i, cells;
    static uint32_t start = 0;

    buf[len++] = GB_DATA_BATTVOLT;
    buf[len++] = 1;
    buf[len++] = 1;

    /* packet voltage, scale 0.1V */
    tmp = gbinf->batt.voltage ?
          dbc_get_signal_from_id(gbinf->batt.voltage)->value * 10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* packet current, scale 0.1A, offset -1000A */
    tmp = gbinf->batt.current ?
          dbc_get_signal_from_id(gbinf->batt.current)->value * 10 + 10000 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* total cell count */
    buf[len++] = gbinf->batt.cell_cnt >> 8;
    buf[len++] = gbinf->batt.cell_cnt;

    /* start cell of current frame */
    buf[len++] = (start + 1) >> 8;
    buf[len++] = (start + 1);

    /* cell count of current frame */
    cells = MIN(gbinf->batt.cell_cnt - start, 200);
    buf[len++] = cells;

    for (i = start; i < start + cells; i++)
    {
        tmp = gbinf->batt.cell[i] ?
              dbc_get_signal_from_id(gbinf->batt.cell[i])->value * 1000:0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    start = (start + cells) % gbinf->batt.cell_cnt;

    return len;
}

static uint32_t gb_data_save_temp(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i;

    buf[len++] = GB_DATA_BATTTEMP;
    buf[len++] = 1;
    buf[len++] = 1;

    /* total temp count */
    buf[len++] = gbinf->batt.temp_cnt >> 8;
    buf[len++] = gbinf->batt.temp_cnt;

    for (i = 0; i < gbinf->batt.temp_cnt; i++)
    {
        buf[len++] = gbinf->batt.temp[i]?
			(uint8_t)(dbc_get_signal_from_id(gbinf->batt.temp[i])->value + 40):0xff;
    }

    return len;
}

static uint32_t gb_data_save_motor(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmp;

    buf[len++] = GB_DATA_MOTORINFO;
    buf[len++] = gbinf->motor_cnt;

    for (i = 0; i < gbinf->motor_cnt; i++)
    {
        /* motor number */
        buf[len++] = i + 1;

        /* motor state */
        if (gbinf->motor[i].info[GB_MINF_STATE])
        {
            tmp = dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_STATE])->value;
            buf[len++] = gbinf->motor[i].state_tbl[tmp] ?
                         gbinf->motor[i].state_tbl[tmp] : 0xff;
        }
        else
        {
            buf[len++] = 0xff;
        }

        /* MCU temperature */
        tmp = gbinf->motor[i].info[GB_MINF_MCUTMP] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_MCUTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor speed, offset -20000rpm */
        tmp = gbinf->motor[i].info[GB_MINF_SPEED] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_SPEED])->value + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor torque, scale 0.1Nm, offset -2000Nm */
        tmp = gbinf->motor[i].info[GB_MINF_TORQUE] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_TORQUE])->value * 10 + 20000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor temperature */
        tmp = gbinf->motor[i].info[GB_MINF_MOTTMP] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_MOTTMP])->value + 40 : 0xff;
        buf[len++] = tmp;

        /* motor voltage, scale 0.1V*/
        tmp = gbinf->motor[i].info[GB_MINF_VOLTAGE] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_VOLTAGE])->value * 10 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;

        /* motor current, scale 0.1A, offset -1000A */
        tmp = gbinf->motor[i].info[GB_MINF_CURRENT] ?
              dbc_get_signal_from_id(gbinf->motor[i].info[GB_MINF_CURRENT])->value * 10 + 10000 : 0xffff;
        buf[len++] = tmp >> 8;
        buf[len++] = tmp;
    }

    return len;
}

static uint32_t gb_data_save_fuelcell(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp,i;

    /* data type : fuel cell information */
    buf[len++] = 0x03;

    /* fuel cell voltage value */
    tmp = gbinf->fuelcell.info[GB_FCINF_VOLTAGE] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_VOLTAGE])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell current value */
    tmp = gbinf->fuelcell.info[GB_FCINF_CURRENT] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_CURRENT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;    
    
    /* fuel cell consumption rate */
    tmp = gbinf->fuelcell.info[GB_FCINF_RATE] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_RATE])->value*100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fuel cell temperature needle number */
    buf[len++] = gbinf->fuelcell.temp_cnt >> 8;
    buf[len++] = gbinf->fuelcell.temp_cnt;

    for(i = 0;i < gbinf->fuelcell.temp_cnt; i++)
    {
        /*highest temperature of hydrogen system  */
        tmp = gbinf->fuelcell.temp[i] ? 
            (dbc_get_signal_from_id(gbinf->fuelcell.temp[i])->value + 40) : 0xff;
        buf[len++] = tmp;
    }
    
    /* highest temperature of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXTEMP] ? 
            (dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXTEMP])->value*10 + 400) : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest temperature of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXTEMPID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXTEMPID])->value : 0xff;
    buf[len++] = tmp;

    /* highest hydrogen rate  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXCCTT] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXCCTT])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* the ID of highest hydrogen rate  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXCCTTID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXCCTTID])->value : 0xff;
    buf[len++] = tmp;

    /*highest pressure of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXPRES] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXPRES])->value*10 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /*the ID of highest pressure of hydrogen system  */
    tmp = gbinf->fuelcell.info[GB_FCINF_MAXPRESID] ? 
          dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_MAXPRESID])->value : 0xff;
    buf[len++] = tmp;

    /* High voltage DCDC state */
    if (gbinf->fuelcell.info[GB_FCINF_HVDCDC])
    {
        tmp = dbc_get_signal_from_id(gbinf->fuelcell.info[GB_FCINF_HVDCDC])->value;
        buf[len++] = gbinf->fuelcell.hvdcdc[tmp] ? gbinf->fuelcell.hvdcdc[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }


    return len;
}

static uint32_t gb_data_save_extr(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, tmpv, tmpi;
    uint32_t maxvid = 0, maxtid = 0, minvid = 0, mintid = 0;
    double maxv, maxt, minv, mint;
    static uint32_t packnum = 0;

    maxvid = maxtid = minvid = mintid = 1;
    maxv = minv = gbinf->batt.cell_cnt > 0 ? dbc_get_signal_from_id(gbinf->batt.cell[0])->value : 0;
    maxt = mint = gbinf->batt.temp_cnt > 0 ? dbc_get_signal_from_id(gbinf->batt.temp[0])->value : 0;

    for (i = 0; i < gbinf->batt.cell_cnt; i++)
    {
        double value = dbc_get_signal_from_id(gbinf->batt.cell[i])->value;

        if (value > 15)
        {
            continue;
        }

        if (minv > 15 || value < minv)
        {
            minv   = value;
            minvid = i + 1;
        }

        if (maxv > 15 || value > maxv)
        {
            maxv   = value;
            maxvid = i + 1;
        }
    }

    for (i = 0; i < gbinf->batt.temp_cnt; i++)
    {
        double value = dbc_get_signal_from_id(gbinf->batt.temp[i])->value;

        if (value > 210)
        {
            continue;
        }

        if (mint > 210 || value < mint)
        {
            mint   = value;
            mintid = i + 1;
        }

        if (maxt > 210 || value > maxt)
        {
            maxt   = value;
            maxtid = i + 1;
        }
    }

    if (gbinf->extr[GB_XINF_MAXVCID])
    {
        maxvid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXVCID])->value;
    }

    if (gbinf->extr[GB_XINF_MAXV])
    {
        maxv = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXV])->value;
    }

    if (gbinf->extr[GB_XINF_MAXTCID])
    {
        maxtid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXTCID])->value;
    }

    if (gbinf->extr[GB_XINF_MAXT])
    {
        maxt = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXT])->value;
    }

    if (gbinf->extr[GB_XINF_MINVCID])
    {
        minvid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINVCID])->value;
    }

    if (gbinf->extr[GB_XINF_MINV])
    {
        minv = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINV])->value;
    }

    if (gbinf->extr[GB_XINF_MINTCID])
    {
        mintid = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINTCID])->value;
    }

    if (gbinf->extr[GB_XINF_MINT])
    {
        mint = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINT])->value;
    }

    buf[len++] = GB_DATA_EXTREMA;

    buf[len++] = 1;

    if (maxvid > (packnum + 1) * 200)
    {
        tmpi = 200;
        tmpv = 0xffff;
    }
    else if (maxvid > packnum * 200)
    {
        tmpi = maxvid - packnum * 200;
        tmpv = maxv * 1000;
    }
    else
    {
        tmpi = 0;
        tmpv = 0xffff;
    }

    buf[len++] = tmpi;
    buf[len++] = tmpv >> 8;
    buf[len++] = tmpv;

    buf[len++] = 1;

    if (minvid > (packnum + 1) * 200)
    {
        tmpi = 200;
        tmpv = 0xffff;
    }
    else if (minvid > packnum * 200)
    {
        tmpi = minvid - packnum * 200;
        tmpv = minv * 1000;
    }
    else
    {
        tmpi = 0;
        tmpv = 0xffff;
    }

    buf[len++] = tmpi;
    buf[len++] = tmpv >> 8;
    buf[len++] = tmpv;

    buf[len++] = 1;
    buf[len++] = maxtid;
    buf[len++] = maxt + 40;
    buf[len++] = 1;
    buf[len++] = mintid;
    buf[len++] = mint + 40;

    packnum = (packnum + 1) % GROUP_SIZE(gbinf);

    return len;
}

static uint32_t gb_data_save_warn(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, j, warnbit = 0, warnlvl = 0;
    uint8_t gb_warn[32] = {0};

    for (i = 0; i < 3; i++)
    {
		if(gbinf->warn[i][5])
		{
			gbinf->warn[i][2] = gbinf->warn[i][5];
		}

		if(gbinf->warn[i][6])
		{
			gbinf->warn[i][3] = gbinf->warn[i][6];
		}
    }

    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 32; j++)
        {
            if (gbinf->warn[i][j] && 
            (dbc_get_signal_from_id(gbinf->warn[i][j])->value || 
            (gbinf->warn[3][j] && 
            dbc_get_signal_from_id(gbinf->warn[3][j])->value)))
            // index 3,as a relevance channel,if the is two canid used for on warning
            {
            	gb_warn[j] = 1;
                warnbit |= 1 << j;
                warnlvl  = i + 1;
            }
        }
    }

    if (gbinf->warntest)
    {
        warnbit |= 1;
        warnlvl  = 3;
    }

    buf[len++] = GB_DATA_WARNNING;
    buf[len++] = warnlvl;
    buf[len++] = warnbit >> 24;
    buf[len++] = warnbit >> 16;
    buf[len++] = warnbit >> 8;
    buf[len++] = warnbit;

    /* battery fault */
    uint8_t *battFaultNum_ptr;
    uint32_t faultCode = 0;
    battFaultNum_ptr =  &buf[len++];
    *battFaultNum_ptr = 0;

    //uint8_t battheatsfastwarn = 0;
    //电池温升过快故障
    for (i = 0; i < 3; i++)
    {
		if (gbinf->warn[i][0x36] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x36])->value ||
		(gbinf->warn[3][0x36] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x36])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			//battheatsfastwarn = 1;
			faultCode = gb_alarmFaultCode[GB_AF_BATTRISEFAST].code;
			buf[len++] = faultCode >> 24;
			buf[len++] = faultCode >> 16;
			buf[len++] = faultCode >> 8;
			buf[len++] = faultCode;
			*battFaultNum_ptr = 1;
		}
    }

    buf[len++] = 0;     /* motor fault */
    buf[len++] = 0;     /* engin fault */

    /* other fault */
    uint8_t *otherFaultNum_ptr;
    otherFaultNum_ptr =  &buf[len++];
    *otherFaultNum_ptr = 0;
    for(i = 0; i < 4; i++)//��Ӧ4����̥
    {
    	if(gbinf->gb_alarmFault.info[GB_AF_FLTYRESENSORLOST + 6*i])
		{
			if(dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYRESENSORLOST + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYRESENSORLOST + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}

    	if(gbinf->gb_alarmFault.info[GB_AF_FLTYRELEAK + 6*i])
		{
			if(dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYRELEAK + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYRELEAK + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}

    	if(0x2 == gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWPRESSUREWARN + 6*i])
		{
			if(0x02 == dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWPRESSUREWARN + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYRELOWPRESSUREWARN + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}

    	if(0x3 == gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWPRESSUREWARN + 6*i])
		{
			if(0x03 == dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWPRESSUREWARN + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYREHIGHPRESSUREWARN + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}

    	if(gbinf->gb_alarmFault.info[GB_AF_FLTYREHIGHTEMPWARN + 6*i])
		{
			if(dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYREHIGHTEMPWARN + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYREHIGHTEMPWARN + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}

    	if(gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWBATT + 6*i])
		{
			if(dbc_get_signal_from_id(gbinf->gb_alarmFault.info[GB_AF_FLTYRELOWBATT + 6*i])->value)
			{
				faultCode = gb_alarmFaultCode[GB_AF_FLTYRELOWBATT + 6*i].code;
				buf[len++] = faultCode >> 24;
				buf[len++] = faultCode >> 16;
				buf[len++] = faultCode >> 8;
				buf[len++] = faultCode;
				*otherFaultNum_ptr += 1;
			}
		}
    }

    //故障诊断,用于外部获取故障报警状态
    for(j = 0;j < 32;j++)
    {
    	gb_fault.warn[j] = gb_warn[j];
    }

	//gb_fault.warn[battheatsfastWARN] = battheatsfastwarn;

    return len;
}

static uint32_t gb_data_save_engin(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, tmp;

    /* data type : engine information */
    buf[len++] = GB_DATA_ENGINEINFO;

    /* engine state */
    if (gbinf->engin.info[GB_EINF_STATE])
    {
        tmp = dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_STATE])->value;
        buf[len++] = gbinf->engin.state_tbl[tmp] ? gbinf->engin.state_tbl[tmp] : 0xff;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* engine speed, scale 1rpm */
    tmp = gbinf->engin.info[GB_EINF_SPEED] ?
          dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_SPEED])->value : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    /* fule rate, scale 0.01L/100km */
    tmp = gbinf->engin.info[GB_EINF_FUELRATE] ?
          dbc_get_signal_from_id(gbinf->engin.info[GB_EINF_FUELRATE])->value * 100 : 0xffff;
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    return len;
}


/* Convert dddmm.mmmm(double) To ddd.dd+(double)*/
static uint32_t gb_data_gpsconv(double dddmm)
{
    int deg;
    double min;

    deg = dddmm / 100.0;
    min = dddmm - deg * 100;

    return (uint32_t)((deg + min / 60 + 0.5E-6) * 1000000);
}


static uint32_t gb_data_save_gps(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0;
    GPS_DATA gpsdata;

    static uint32_t  longitudeBak = 0;
    static uint32_t  latitudeBak  = 0;

    /* data type : location data */
    buf[len++] = GB_DATA_LOCATION;

    /* status bits */
    /* bit-0: 0-A,1-V */
    /* bit-1: 0-N,1-S */
    /* bit-2: 0-E,1-W */
    if (gps_get_fix_status() == 2)
    {
        gps_get_snap(&gpsdata);
        longitudeBak = gb_data_gpsconv(gpsdata.longitude);
        latitudeBak  = gb_data_gpsconv(gpsdata.latitude);
        buf[len++]   = (gpsdata.is_north ? 0 : 0x02) | (gpsdata.is_east ? 0 : 0x04);
    }
    else
    {
        buf[len++] = 0x01;
    }

    /* longitude */
    buf[len++] = longitudeBak >> 24;
    buf[len++] = longitudeBak >> 16;
    buf[len++] = longitudeBak >> 8;
    buf[len++] = longitudeBak;
    /* latitude */
    buf[len++] = latitudeBak >> 24;
    buf[len++] = latitudeBak >> 16;
    buf[len++] = latitudeBak >> 8;
    buf[len++] = latitudeBak;

    return len;
}

#if GB_EXT
static uint32_t gb_data_save_VSExt(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0;
    int i;
    int tmp = 0;

    /* data type : location data */
    buf[len++] = 0x91;//��Ϣ���ͱ�־

    /* door lock state */
    if(gbinf->gb_VSExt.info[GB_VS_DRIDOORLOCKST])
    {
        if(dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_DRIDOORLOCKST])->value)
        {
            buf[len++] = 0;//上锁
            buf[len++] = 0;//上锁
            buf[len++] = 0;//上锁
            buf[len++] = 0;//上锁
        }
        else
        {
        	 buf[len++] = 1;//解锁
        	 buf[len++] = 1;//解锁
        	 buf[len++] = 1;//解锁
        	 buf[len++] = 1;//解锁
        }
    }
    else
    {
        buf[len++] = 0xff;
        buf[len++] = 0xff;
        buf[len++] = 0xff;
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_REARDDOORLOCKST])//
    {
        if(dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_REARDDOORLOCKST])->value)
        {
            buf[len++] = 0;//
        }
        else
        {
        	 buf[len++] = 1;//
        }
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* 驾驶侧窗state */
    buf[len++] = 0xff;//驾驶侧״̬
    buf[len++] = 0xff;//副驾驶侧״̬
    buf[len++] = 0xff;//左后窗״̬
    buf[len++] = 0xff;//右后窗״̬
    tmp = PrvtProt_SignParse_sunroofSt();
    if(tmp == 0x2)
    {
    	buf[len++] = 0x0;//天窗关闭״̬
    }
    else if((tmp == 0x0) || (tmp == 0x1) || (tmp == 0x3) || (tmp == 0x4))
    {
    	buf[len++] = 0x01;//天窗开启
    }
    else if(tmp == 0x5)
    {
    	buf[len++] = 0xfe;//异常
    }
    else
    {
    	buf[len++] = 0xff;//无效
    }

    /* 驾驶侧门state */
    for(i = 0; i < 4; i++)
    {
        if(gbinf->event.info[GB_EVT_LEFTDRVDOOR_OPEN+i])
        {
            if(2 == dbc_get_signal_from_id(gbinf->event.info[GB_EVT_LEFTDRVDOOR_OPEN+i])->value)//��
            {
                buf[len++] = 1;
                gbinf->gb_VSExt.oldst[GB_VS_DRIDOORST+i]  = 1;
            }
            else if(0 == dbc_get_signal_from_id(gbinf->event.info[GB_EVT_LEFTDRVDOOR_OPEN])->value)//��
            {
            	 buf[len++] = 0;
            	 gbinf->gb_VSExt.oldst[GB_VS_DRIDOORST+i]  = 0;
            }
            else
            {
            	buf[len++] = gbinf->gb_VSExt.oldst[GB_VS_DRIDOORST+i];
            }
        }
        else
        {
            buf[len++] = 0xff;
            gbinf->gb_VSExt.oldst[GB_VS_DRIDOORST+i]  = 0xff;
        }
    }

    if(gbinf->gb_VSExt.info[GB_VS_BACKDOORST])//后备箱门状态״̬
    {
        if(dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_BACKDOORST])->value)
        {
            buf[len++] = 1;//����
        }
        else
        {
        	 buf[len++] = 0;//
        }
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* 空调信息 */
    if(gbinf->gb_VSExt.info[GB_VS_ACST])//
    {
        if(dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ACST])->value)
        {
            buf[len++] = 1;//开启
        }
        else
        {
        	 buf[len++] = 0;//
        }
    }
    else
    {
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_ACTEMP])//空调温度
    {
    	 buf[len++] = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ACTEMP])->value + 16) * 2;
    }
    else
    {
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_ACMODE])//空调模式
    {
    	tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ACMODE])->value;
    	switch(tmp)
    	{
			case 1:
			case 5:
			case 7:
			{
				 buf[len++] = 1;//制热
			}
			break;
			case 2:
			case 6:
			{
				 buf[len++] = 2;//制冷
			}
			break;
			default:
			{
				buf[len++] = 0xff;
			}
			break;
    	}
    }
    else
    {
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_AIRVOLUME])//
    {
    	buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_AIRVOLUME])->value;
    }
    else
    {
        buf[len++] = 0;
    }

    if(gbinf->gb_VSExt.info[GB_VS_INTEMP])//
    {
    	buf[len++] = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_INTEMP])->value + 55) * 2;
    }
    else
    {
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_OUTTEMP])//
    {
    	buf[len++] = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_OUTTEMP])->value + 55) * 2;
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* 车灯状态 */
    if(gbinf->gb_VSExt.info[GB_VS_HLAMPST])//˫��״̬
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_HLAMPST])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}

    if(gbinf->event.info[GB_EVT_POSLAMP_ON])//示位灯
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->event.info[GB_EVT_POSLAMP_ON])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}

    if(gbinf->event.info[GB_EVT_NEARLAMP_ON])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->event.info[GB_EVT_NEARLAMP_ON])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}
    if(gbinf->event.info[GB_EVT_HIGHBEAMLAMP_ON])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->event.info[GB_EVT_HIGHBEAMLAMP_ON])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}
    if(gbinf->event.info[GB_EVT_LEFTTURNLAMP_ON])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->event.info[GB_EVT_LEFTTURNLAMP_ON])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}
    if(gbinf->event.info[GB_EVT_RIGHTTURNLAMP_ON])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->event.info[GB_EVT_RIGHTTURNLAMP_ON])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}
    if(gbinf->gb_VSExt.info[GB_VS_BRAKELAMPST])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_BRAKELAMPST])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}
    if(gbinf->gb_VSExt.info[GB_VS_ATMOSPHERELAMPST])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ATMOSPHERELAMPST])->value;
	}
	else
	{
		buf[len++] = 0xff;
	}

    /* 车胎信息 */
    for(i=0;i<4;i++)
    {
        if(gbinf->gb_VSExt.info[GB_VS_RFTYRETEMP+2*i])//
    	{
    		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_RFTYRETEMP+2*i])->value + 50;
    	}
    	else
    	{
    		buf[len++] = 0xff;
    	}

        if(gbinf->gb_VSExt.info[GB_VS_RFTYREPRESSURE+2*i])//
    	{
        	tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_RFTYREPRESSURE+2*i])->value/100/0.0177;
        	if(tmp > 253) tmp = 253;
    		buf[len++] = tmp;
    	}
    	else
    	{
    		buf[len++] = 0xff;
    	}
    }

    /* 充电信息 */
    if(gbinf->gb_VSExt.info[GB_VS_REMAINCHRGTIME])//剩余充电时间
	{
    	tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_REMAINCHRGTIME])->value;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		buf[len++] = 0xff;
		buf[len++] = 0xff;
	}

    if(GetPP_ChargeCtrl_appointSt())
    {
		buf[len++] = 1;//定时充电状态״̬
		buf[len++] = GetPP_ChargeCtrl_appointHour();//定时充电小时
		buf[len++] = GetPP_ChargeCtrl_appointMin();//定时充电分钟
    }
    else
    {
		buf[len++] = 0;//定时充电状态״̬
		buf[len++] = 0xff;//定时充电小时
		buf[len++] = 0xff;//定时充电分钟
    }

    uint8_t chargeSt;
	if(gbinf->gb_VSExt.info[GB_VS_FSCHARGEST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_FSCHARGEST])->value;
		gb_chargeSt = tmp;//
		switch(tmp)
		{
			case 0:
			case 3:
			case 5://未充电
			{
				chargeSt = 0;
			}
			break;
			case 1:
			case 6://慢充
			{
				chargeSt = 1;
			}
			break;
			case 2://快充
			{
				chargeSt = 2;
			}
			break;
			case 4://异常
			{
				chargeSt = 0xfe;
			}
			break;
			default://无效
			{
				chargeSt = 0xff;
			}
			break;
		}

		buf[len++] = chargeSt;
	}
	else
	{
		 buf[len++] = 0xff;
	}

    /* 基本状态信息值数据 */
    buf[len++] = 0xff;//蜂窝网络状态״̬
    buf[len++] = 0xff;//蜂窝网络信号 强度
    buf[len++] = (uint8_t)canact;//CAN通讯状态״̬
    buf[len++] = 0xff;//12V 蓄电池电压
    if(gbinf->vehi.info[GB_VINF_SOC])
    {
    	tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_SOC])->value;
    	tmp = 100*tmp;
    	buf[len++] = tmp >> 8;
    	buf[len++] = tmp;
    }
    else
    {
    	 buf[len++] = 0xff;
    	 buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_ENDURANCEMILE])//续航里程
    {
    	tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ENDURANCEMILE])->value * 10;
    	buf[len++] = tmp >> 8;
    	buf[len++] = tmp;
    }
    else
    {
    	 buf[len++] = 0xff;
    	 buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_DRIVMODE])//
    {
    	buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_DRIVMODE])->value;
    }
    else
    {
    	 buf[len++] = 0xff;
    }

    uint8_t parkSt = 0;
    if(gbinf->gb_VSExt.info[GB_VS_PARKST])//驻车状态״̬
    {
    	tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_PARKST])->value;
    	switch(tmp)
    	{
    		case 0:
    		case 2://无效
    		{
    			parkSt = 0xff;
    		}
    		break;
    		case 1://关闭
    		{
    			parkSt = 0;
    		}
    		break;
    		case 4://开启
    		{
    			parkSt = 1;
    		}
    		break;
    		case 3://异常
    		{
    			parkSt = 0xfe;
    		}
    		break;
    		default:
    			parkSt = 0xff;
    		break;
    	}
    	buf[len++] = parkSt;
    }
    else
    {
    	 buf[len++] = 0xff;
    }

    if (gbinf->vehi.info[GB_VINF_STATE])//����״̬
    {
        if(dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_STATE])->value)
        {
        	buf[len++] = 1;
        }
        else
        {
        	buf[len++] = 0;
        }
    }
    else
    {
        buf[len++] = 0xff;
    }

    /* 运动状态 */
    for(i = 0;i<2;i++)
    {
        if(gbinf->gb_VSExt.info[GB_VS_ASPEED_X+i])//加速度x、y
        {
        	tmp = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ASPEED_X+i])->value + 1023) * 100;
        	buf[len++] = tmp >> 8;
        	buf[len++] = tmp;
        }
        else
        {
        	 buf[len++] = 0xff;
        	 buf[len++] = 0xff;
        }
    }

    if(gbinf->gb_VSExt.info[GB_VS_ASPEED_Z])////加速度z
    {
    	tmp = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ASPEED_Z])->value + 4095) * 100;
    	buf[len++] = tmp >> 8;
    	buf[len++] = tmp;
    }
    else
    {
    	 buf[len++] = 0xff;
    	 buf[len++] = 0xff;
    }

    for(i = 0;i<4;i++)
    {
		if(gbinf->gb_VSExt.info[GB_VS_FLTYRERSPEED +i])//����ת��
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_FLTYRERSPEED +i])->value * 10;
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
		}
    }

	if(gbinf->gb_VSExt.info[GB_VS_STEERWHEELANGLE])//
	{
		tmp = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_STEERWHEELANGLE])->value + 720) * 10;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_TRIP])//小计里程
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_TRIP])->value * 10;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_SUBTOTALTRVLTIME])//С����ʻʱ��
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_SUBTOTALTRVLTIME])->value;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

    for(i = 0;i<3;i++)
    {
		if(gbinf->gb_VSExt.info[GB_VS_TIPC+i])//˲ʱ/ƽ��/С�Ƶ��
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_TIPC+i])->value * 10;
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
		}
    }

    /* 扩展状态信息 */
	if(gbinf->gb_VSExt.info[GB_VS_ESCACTIVEST])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ESCACTIVEST])->value;
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_ESCDISABLEST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ESCDISABLEST])->value;
		if(tmp == 0)
		{
			buf[len++] = 1;
		}
		else if(tmp == 1)
		{
			buf[len++] = 0;
		}
		else
		{
			buf[len++] = 0xfe;
		}

	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_TCSACTIVEST])//TCS
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_TCSACTIVEST])->value;
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_TCSACTIVEST])//SAS
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_TCSACTIVEST])->value;
	}
	else
	{
		 buf[len++] = 0xff;
	}

	for(i = 0;i<2;i++)
	{
		if(gbinf->gb_VSExt.info[GB_VS_MAINDRIBELTST+i])//��/����ʻ��ȫ��״̬
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_MAINDRIBELTST+i])->value;
			if(tmp == 0)
			{
				buf[len++] = 1;
			}
			else if(tmp == 2)
			{
				buf[len++] = 0;
			}
			else
			{
				buf[len++] = 0xfe;
			}
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_VSExt.info[GB_VS_ELECSTOPBRAKEST])//
	{
		buf[len++] = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ELECSTOPBRAKEST])->value;
	}
	else
	{
		 buf[len++] = 0xff;
	}
	buf[len++] = 0xff;//预留1
	buf[len++] = 0xff;//预留2
	buf[len++] = 0xff;//预留3
	buf[len++] = 0xff;
	buf[len++] = 0xff;//预留4
	buf[len++] = 0xff;

    return len;
}

/* 车辆位置扩展 */
static uint32_t gb_data_save_VehiPosExt(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0;
    int tmp = 0;
	GPS_DATA gps_snap;

	gps_get_snap(&gps_snap);

    /* data type : location data */
    buf[len++] = 0x92;//��Ϣ���ͱ�־

    tmp = gps_snap.kms * 10;//�����ն˵��ٶ�
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    tmp =gps_snap.hdop * 10;//��λ����
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    tmp = gps_snap.direction;//����
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    tmp =  gps_snap.msl * 10;//�߶�
    buf[len++] = tmp >> 8;
    buf[len++] = tmp;

    return len;
}


/*�㲿��״̬����*/
static uint32_t gb_data_save_ComponentSt(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0;
    int i;
    int32_t  tmp = 0;
    uint32_t tmp_32 = 0;
    int32_t  gb_xinf_maxv = 0;
    int32_t  gb_xinf_minv = 0;
    uint8_t xinf_volvalidflg = 1;
    uint8_t  gb_xinf_maxt = 0;
    uint8_t  gb_xinf_mint = 0;
    uint8_t xinf_tempvalidflg = 1;

    /* data type : location data */
    buf[len++] = 0x93;//��Ϣ���ͱ�־

    /* ���������� */
	if(gbinf->gb_ConpSt.info[GB_CMPT_MTRTARGETTORQUE])//��ǰ���Ŀ��Ť��
	{
		tmp = (dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_MTRTARGETTORQUE])->value + 1024) * 20;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_MTRTARGETSPEED])//
	{
		tmp = (dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_MTRTARGETSPEED])->value + 16000) * 2;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_SYSST])//���µ�״̬
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_SYSST])->value;
		if((tmp>=0)&&(tmp<=5))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_PWRUPST])//״̬
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_PWRUPST])->value;
		if((tmp>=0)&&(tmp<=5))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_PWRDWNST])//״̬
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_PWRDWNST])->value;
		if((tmp>=0)&&(tmp<=6))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_FANGEARSTS])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_FANGEARSTS])->value;
		if((tmp>=0)&&(tmp<=2))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_PTCWORKSTS])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_PTCWORKSTS])->value;
		if(tmp == 0x65)
		{
			buf[len++] = 0xff;
		}
		else
		{
			buf[len++] = tmp;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_MOTCIRCWTRPUMWRKST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_MOTCIRCWTRPUMWRKST])->value;
		tmp = 10*tmp;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_BATCIRCWTRPUMWRKST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_BATCIRCWTRPUMWRKST])->value;
		tmp = 10*tmp;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_CRASHOUTPUTST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CRASHOUTPUTST])->value;
		if((tmp>=0)&&(tmp<=3))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

    /* ��������� */
	if(gbinf->gb_ConpSt.info[GB_CMPT_WORKMODE])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_WORKMODE])->value;
		if((tmp>=0)&&(tmp<=7))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_CTRLTORQUE])//���ʵ��Ť��
	{
		tmp = (dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CTRLTORQUE])->value + 1024) * 20;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_WORKMODE])//������ǰ��ת����
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_WORKMODE])->value;
		if((tmp>=0)&&(tmp<=7))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_MAXAVAILTORQUE])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_MAXAVAILTORQUE])->value * 20;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

    /* ��ع���ϵͳ */
	for(i=0;i<4;i++)//��������Ԥ��磬���̵����պ�״̬
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_POSBATTCONTST+i])//
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_POSBATTCONTST+i])->value;
			if((tmp>=0)&&(tmp<=1))
			{
				buf[len++] = tmp;
			}
			else
			{
				buf[len++] = 0xfe;
			}
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	if (gbinf->extr[GB_XINF_MAXV])
	{
		tmp = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXV])->value;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
		gb_xinf_maxv = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
		 xinf_volvalidflg = 0;
	}

	if (gbinf->extr[GB_XINF_MINV])
	{
		tmp = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINV])->value;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
		gb_xinf_minv = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
		 xinf_volvalidflg = 0;
	}

	if(1 == xinf_volvalidflg)//����ѹ��
	{
		tmp = gb_xinf_maxv - gb_xinf_minv;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if (gbinf->extr[GB_XINF_MAXT])
	{
		tmp = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MAXT])->value + 40;
		buf[len++] = tmp;
		gb_xinf_maxt = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 xinf_tempvalidflg = 0;
	}

	if (gbinf->extr[GB_XINF_MINT])
	{
		tmp = dbc_get_signal_from_id(gbinf->extr[GB_XINF_MINT])->value + 40;
		buf[len++] = tmp;
		gb_xinf_mint = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 xinf_tempvalidflg = 0;
	}

	if(1 == xinf_tempvalidflg)//�����²�
	{
		buf[len++] = gb_xinf_maxt - gb_xinf_mint;
	}
	else
	{
		 buf[len++] = 0xff;
	}

	for(i=0;i<3;i++)//���CC/CP������CC����״̬
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_CHARGECCSIG+i])//
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CHARGECCSIG+i])->value;
			if((tmp>=0)&&(tmp<=1))
			{
				buf[len++] = tmp;
			}
			else
			{
				buf[len++] = 0xfe;
			}
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	for(i=0;i<4;i++)//��䡢������¶�
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_POSFASTCHGPORTTEMP+i])//
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_POSFASTCHGPORTTEMP+i])->value + 40;
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEST])->value;
		if((tmp>=0)&&(tmp<=1))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_ACMODE])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ACMODE])->value;
		if((tmp>=0)&&(tmp<=7))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

   if(gbinf->vehi.info[GB_VINF_VOLTAGE])
   {
	   tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_VOLTAGE])->value * 20;
	   buf[len++] = tmp >> 8;
	   buf[len++] = tmp;
   }
   else
   {
	   buf[len++] = 0xff;
	   buf[len++] = 0xff;
   }

	for(i=0;i<2;i++)//��������ܡ���������
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_HDSAHTOTALCPSUM+i])//
		{
			tmp_32 = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_HDSAHTOTALCPSUM+i])->value * 100;
			buf[len++] = tmp_32 >> 24;
			buf[len++] = tmp_32 >> 16;
			buf[len++] = tmp_32 >> 8;
			buf[len++] = tmp_32;
		}
		else
		{
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
		}
	}

	if (gbinf->gb_ConpSt.info[GB_CMPT_12VBATTVOLT])
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_12VBATTVOLT])->value * 20;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_REQOUTPUTMODE])//������س������
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_REQOUTPUTMODE])->value;
		if((tmp>=0)&&(tmp<=3))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if (gbinf->gb_ConpSt.info[GB_CMPT_FCCURRENTREQ])
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_FCCURRENTREQ])->value + 400;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	if (gbinf->gb_ConpSt.info[GB_CMPT_FCVOLTREQ])
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_FCVOLTREQ])->value;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	/* ���س��� */
	if(gbinf->vehi.info[GB_VINF_CHARGE])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_VINF_CHARGE])->value;
		if((tmp>=0)&&(tmp<=2))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if (gbinf->gb_ConpSt.info[GB_CMPT_CURRENABLEPWROUTMAX])//��������������
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CURRENABLEPWROUTMAX])->value * 200;
		buf[len++] = tmp >> 8;
		buf[len++] = tmp;
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	for(i=0;i<4;i++)//������롢�����ѹ����
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEOUTVOLT])
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEOUTVOLT])->value * 20;
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_ConpSt.info[GB_VINF_CHARGE])
	{
		tmp = dbc_get_signal_from_id(gbinf->vehi.info[GB_VINF_CHARGE])->value;
		if((tmp>=0)&&(tmp<=7))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEMTRWORKST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CHARGEMTRWORKST])->value;
		if((tmp>=0)&&(tmp<=7))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	/* 空调控制器CLM */
	if(gbinf->gb_VSExt.info[GB_VS_ACST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_ACST])->value;
		if((tmp>=0)&&(tmp<=1))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_CYCLEMODE])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CYCLEMODE])->value;
		if((tmp>=0)&&(tmp<=2))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	for(i=0;i<3;i++)//吹面、吹脚、吹窗模式
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_VENTMODE+i])//
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_VENTMODE+i])->value;
			if((tmp>=0)&&(tmp<=1))
			{
				buf[len++] = tmp;
			}
			else
			{
				buf[len++] = 0xfe;
			}
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	for(i=0;i<2;i++)//主驾、副驾设定温度
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_LHTEMP+i])//
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_LHTEMP+i])->value;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_ACSTS])//AC״̬
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_ACSTS])->value;
		if((tmp>=0)&&(tmp<=1))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_PTCWORKST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_PTCWORKST])->value;
		if((tmp>=0)&&(tmp<=3))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_VSExt.info[GB_VS_AIRVOLUME])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_AIRVOLUME])->value;
		if(((tmp>=0)&&(tmp<=7)) || (tmp == 0xe))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

    if(gbinf->gb_VSExt.info[GB_VS_OUTTEMP])//
    {
    	buf[len++] = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_OUTTEMP])->value + 55) * 2;
    }
    else
    {
        buf[len++] = 0xff;
    }

    if(gbinf->gb_VSExt.info[GB_VS_INTEMP])//
    {
    	buf[len++] = (dbc_get_signal_from_id(gbinf->gb_VSExt.info[GB_VS_INTEMP])->value + 55) * 2;
    }
    else
    {
        buf[len++] = 0xff;
    }

	if(gbinf->gb_ConpSt.info[GB_CMPT_RAINSENSOR])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_RAINSENSOR])->value;
		if((tmp>=0)&&(tmp<=4))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	/* ѹ����EAC */
	if(gbinf->gb_ConpSt.info[GB_CMPT_EACBASEST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_EACBASEST])->value;
		if((tmp>=0)&&(tmp<=4))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	for(i=0;i<2;i++)//ѹ����Ŀ�ꡢʵ��ת��
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_EACSPEEDSET+i])
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_EACSPEEDSET+i])->value / 100;
			buf[len++] = tmp;
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_EACHIGHVOLT])
	{//EAC高压供电电压
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_EACHIGHVOLT])->value;
		if(tmp == 0x1ff)
		{
			buf[len++] = 0xff;
			buf[len++] = 0xff;
		}
		else
		{
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
	}
	else
	{
		buf[len++] = 0xff;
		buf[len++] = 0xff;
	}

	/* PTC */
	if(gbinf->gb_ConpSt.info[GB_CMPT_PTCPWRCONS])
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_PTCPWRCONS])->value * 10;
		if(tmp == 0xfe)
		{
			buf[len++] = 0xff;
			buf[len++] = 0xff;
		}
		else if(tmp == 0xff)
		{
			buf[len++] = 0xff;
			buf[len++] = 0xfe;
		}
		else
		{
			buf[len++] = tmp >> 8;
			buf[len++] = tmp;
		}
	}
	else
	{
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

	for(i=0;i<2;i++)//冷却液进、出口温度
	{
		if(gbinf->gb_ConpSt.info[GB_CMPT_CLNTTEMPIN+i])
		{
			tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_CLNTTEMPIN+i])->value + 40;
			if(tmp == 0xfe)
			{
				buf[len++] = 0xff;
			}
			else if(tmp == 0xff)
			{
				buf[len++] = 0xfe;
			}
			else
			{
				buf[len++] = tmp;
			}
		}
		else
		{
			 buf[len++] = 0xff;
		}
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_KEYAUTHEST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_KEYAUTHEST])->value;
		if((tmp>=0)&&(tmp<=2))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_IDDEVICENO])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_IDDEVICENO])->value;
		if((tmp>=0)&&(tmp<=4))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}

	if(gbinf->gb_ConpSt.info[GB_CMPT_VCULEARNST])//
	{
		tmp = dbc_get_signal_from_id(gbinf->gb_ConpSt.info[GB_CMPT_VCULEARNST])->value;
		if((tmp>=0)&&(tmp<=1))
		{
			buf[len++] = tmp;
		}
		else
		{
			buf[len++] = 0xfe;
		}
	}
	else
	{
		 buf[len++] = 0xff;
	}


	{//预留1
		 buf[len++] = 0xff;
	}
	{//预留2
		 buf[len++] = 0xff;
	}
	{//预留3
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}
	{//预留4
		 buf[len++] = 0xff;
		 buf[len++] = 0xff;
	}

    return len;
}

/* 告警扩展数据 */
static uint32_t gb_data_save_warnExt(gb_info_t *gbinf, uint8_t *buf)
{
    uint32_t len = 0, i, j;
    uint8_t* warnlvl_ptr;
    uint8_t* warnnum_ptr;
    uint16_t warn_code = 0;
    uint8_t ac_warnflag=0;
    uint8_t warnvalue = 0;
    uint8_t Abnormalheat_warnflag = 0;
	uint8_t vs_warn[71] = {0};
    /* data type : warn extend information */
    buf[len++] = 0x94;

    warnlvl_ptr = &buf[len++];//��߱����ȼ�
    warnnum_ptr = &buf[len++];//������
    *warnlvl_ptr = 0;
    *warnnum_ptr = 0;
    for (i = 0; i < 3; i++)
    {
    	if(gbinf->warn[i][10])
    	{//动力电池电压不均衡保护故障
    		gbinf->warn[i][62] = gbinf->warn[i][10];
    	}
    }

    for (i = 0; i < 3; i++)
    {
        for (j = 32; j < GB_MAX_WARN_INFO; j++)
        {
            if (gbinf->warn[i][j] &&
            (dbc_get_signal_from_id(gbinf->warn[i][j])->value ||
            (gbinf->warn[3][j] &&
            dbc_get_signal_from_id(gbinf->warn[3][j])->value)))
            // index 3,as a relevance channel,if the is two canid used for on warning
            {
            	warnvalue = dbc_get_signal_from_id(gbinf->warn[i][j])->value;
            	if(j == 0x50)//制冷不工作原因-压缩机故障
            	{
            		switch(warnvalue)
            		{
            			case 0x01://压缩机故障
            			{
            				warn_code = 48;
            			}
            			break;
            			case 0x02://电子膨胀阀故障
            			{
            				warn_code = 49;
            			}
            			break;
            			case 0x03://HV 故障
            			{
            				warn_code = 50;
            			}
            			break;
            			case 0x04://压力传感器故障
            			{
            				warn_code = 51;
            			}
            			break;
            			case 0x05://冷却风扇故障
            			{
            				warn_code = 52;
            			}
            			break;
            			case 0x06://蒸发器温度传感器故障
            			{
            				warn_code = 53;
            			}
            			break;
            			default:
            			break;
            		}
    	        	buf[len++] = warn_code >> 8;
    	        	buf[len++] = warn_code;
    	        	(*warnnum_ptr)++;
    	        	(*warnlvl_ptr)  = i + 1;
    	        	ac_warnflag=1;
            	}
            	else if(j == 0x47)//制热不响应原因
            	{
            		switch(warnvalue)
            		{
            			case 0x01://PTC故障
            			{
            				warn_code = 54;
            				Abnormalheat_warnflag = 1;
            			}
            			break;
            			case 0x02://HV故障
            			{
            				warn_code = 55;
            				Abnormalheat_warnflag = 1;
            			}
            			break;
            			case 0x03://PTC水泵故障
            			{
            				warn_code = 56;
            				Abnormalheat_warnflag = 1;
            			}
            			break;
            			case 0x04://三通水阀故障
            			{
            				warn_code = 57;
            				Abnormalheat_warnflag = 1;
            			}
            			break;
            			default:
            			break;
            		}
    	        	buf[len++] = warn_code >> 8;
    	        	buf[len++] = warn_code;
    	        	(*warnnum_ptr)++;
    	        	(*warnlvl_ptr)  = i + 1;
    	        	ac_warnflag=1;
            	}
            	else
            	{
                	buf[len++] = gb_alarmCode[j].code >> 8;
                	buf[len++] = gb_alarmCode[j].code;
                	(*warnnum_ptr)++;
                	(*warnlvl_ptr)  = i + 1;
            	}
            }
			if (gbinf->warn[i][j] &&
            	(dbc_get_signal_from_id(gbinf->warn[i][j])->value ||
            	(gbinf->warn[3][j] &&
            	dbc_get_signal_from_id(gbinf->warn[3][j])->value)))
			{
				vs_warn[j-32] = 1;
			}
        }

        //MCU 内部 IGBT 过温（U 相） ࣩ
		if (gbinf->warn[i][0x0f] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x0f])->value ||
		(gbinf->warn[3][0x0f] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x0f])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 7;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //单体蓄电池过压报警
		if (gbinf->warn[i][0x05] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x05])->value ||
		(gbinf->warn[3][0x05] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x05])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 18;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //档位信号故障
		if (gbinf->warn[i][0x33] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x33])->value ||
		(gbinf->warn[3][0x33] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x33])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 20;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //电机异常报警
		if (gbinf->warn[i][0x0b] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x0b])->value ||
		(gbinf->warn[3][0x0b] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x0b])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 26;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //电机异常报警
		if (gbinf->warn[i][0x0f] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x0f])->value ||
		(gbinf->warn[3][0x0f] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x0f])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 26;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //动力电池单体电压过压保护
		if (gbinf->warn[i][0x05] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x05])->value ||
		(gbinf->warn[3][0x05] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x05])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 27;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //动力电池单体电压欠压保护故障
		if (gbinf->warn[i][0x06] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x06])->value ||
		(gbinf->warn[3][0x06] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x06])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 28;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

        //动力电池温度过高保护故障
		if (gbinf->warn[i][0x01] &&
		(dbc_get_signal_from_id(gbinf->warn[i][0x01])->value ||
		(gbinf->warn[3][0x01] &&
		dbc_get_signal_from_id(gbinf->warn[3][0x01])->value)))
		// index 3,as a relevance channel,if the is two canid used for on warning
		{
			warn_code = 32;
        	buf[len++] = warn_code >> 8;
        	buf[len++] = warn_code;
        	(*warnnum_ptr)++;
        	(*warnlvl_ptr)  = i + 1;
		}

    }

    if(ac_warnflag)//空调不工作
    {
		warn_code = 47;
    	buf[len++] = warn_code >> 8;
    	buf[len++] = warn_code;
    	(*warnnum_ptr)++;
    	//(*warnlvl_ptr)  = i + 1;
    }

    if(Abnormalheat_warnflag)//整车加热异常
    {
    	warn_code = 39;
		buf[len++] = warn_code >> 8;
		buf[len++] = warn_code;
		(*warnnum_ptr)++;
    	//(*warnlvl_ptr)  = i + 1;
    }

    PP_rmtDiag_NodeFault_t NodeFault;
    getPPrmtDiagCfg_NodeFault(&NodeFault);
    //TBOX 故障报警
    if(NodeFault.tboxFault)
    {
		warn_code = 13;
    	buf[len++] = warn_code >> 8;
    	buf[len++] = warn_code;
    	(*warnnum_ptr)++;
    	(*warnlvl_ptr)  = 0 + 1;
    }

    //与BMS通讯丢失
    if(NodeFault.BMSMiss)
    {
		warn_code = 37;
    	buf[len++] = warn_code >> 8;
    	buf[len++] = warn_code;
    	(*warnnum_ptr)++;
    	(*warnlvl_ptr)  = 2 + 1;
    }

    //与MCU通讯丢失
    if(NodeFault.MCUMiss)
    {
		warn_code = 38;
    	buf[len++] = warn_code >> 8;
    	buf[len++] = warn_code;
    	(*warnnum_ptr)++;
    	//(*warnlvl_ptr)  = 0 + 1;
    }

    //拖车提醒
    if(0)
    {
		warn_code = 45;
    	buf[len++] = warn_code >> 8;
    	buf[len++] = warn_code;
    	(*warnnum_ptr)++;
    	//(*warnlvl_ptr)  = i + 1;
    }
	//故障诊断,用于外部获取故障报警状态
    for(j = 0;j < 41;j++)
    {
    	//gb_fault.warn[32] = 1;	
    	gb_fault.warn[gb32960_api_extwarn_indextable[j].vsindex] = vs_warn[gb32960_api_extwarn_indextable[j].gbindex - 32];
    }
    return len;
}

#endif

static uint32_t gb_data_save_all(gb_info_t *gbinf, uint8_t *buf, uint32_t uptime)
{
    uint32_t len = 0;
    RTCTIME time;

    can_get_time(uptime, &time);

    buf[len++] = time.year - 2000;
    buf[len++] = time.mon;
    buf[len++] = time.mday;
    buf[len++] = time.hour;
    buf[len++] = time.min;
    buf[len++] = time.sec;

    len += gb_data_save_vehi(gbinf, buf + len);

    if (gbinf->vehi.vehi_type == GB_VEHITYPE_ELECT ||
        gbinf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
    {   
        uint8_t fuelcell;
        uint32_t paralen = sizeof(fuelcell);
        len += gb_data_save_motor(gbinf, buf + len);        
        if(0 == cfg_get_para(CFG_ITEM_FUELCELL, &fuelcell, &paralen) && fuelcell)
        {
            len += gb_data_save_fuelcell(gbinf, buf + len);
        }
        len += gb_data_save_cell(gbinf, buf + len);
        len += gb_data_save_temp(gbinf, buf + len);
        len += gb_data_save_extr(gbinf, buf + len);
    }

    if (gbinf->vehi.vehi_type == GB_VEHITYPE_GASFUEL ||
        gbinf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
    {
        len += gb_data_save_engin(gbinf, buf + len);
    }

    len += gb_data_save_gps(gbinf, buf + len);
    len += gb_data_save_warn(gbinf, buf + len);
#if GB_EXT
    len += gb_data_save_VSExt(gbinf, buf + len);
    len += gb_data_save_VehiPosExt(gbinf, buf + len);
    len += gb_data_save_ComponentSt(gbinf, buf + len);
    len += gb_data_save_warnExt(gbinf, buf + len);

#endif
    return len;
}

static void gb_data_save_realtm(gb_info_t *gbinf, uint32_t uptime)
{
    int i;

    DAT_LOCK();

    for (i = 0; i < GROUP_SIZE(gbinf); i++)
    {
        gb_pack_t *rpt;
        list_t *node;

        if ((node = list_get_first(&gb_free_lst)) == NULL)
        {
            if ((node = list_get_first(&gb_delay_lst)) == NULL &&
                (node = list_get_first(&gb_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_GB32960, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        rpt = list_entry(node, gb_pack_t, link);
        rpt->len  = gb_data_save_all(gbinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        rpt->list = &gb_realtm_lst;
        rpt->type = GB_RPTTYPE_REALTM;
        list_insert_before(&gb_realtm_lst, node);
    }

    DAT_UNLOCK();
}

static void gb_data_save_error(gb_info_t *gbinf, uint32_t uptime)
{
    int i;

    ERR_LOCK();

    for (i = 0; i < GROUP_SIZE(gbinf); i++)
    {
        gb_pack_t *rpt = list_entry(gb_errlst_head, gb_pack_t, link);

        rpt->len  = gb_data_save_all(gbinf, rpt->data, uptime);
        rpt->seq  = i + 1;
        gb_errlst_head = gb_errlst_head->next;
    }

    ERR_UNLOCK();
}

static void gb_data_flush_error(void)
{
    list_t tmplst;

    list_init(&tmplst);
    ERR_LOCK();

    if (gb_errlst_head == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    while (list_entry(gb_errlst_head->prev, gb_pack_t, link)->len)
    {
        list_t *node;
        gb_pack_t *rpt, *err;

        DAT_LOCK();

        if ((node = list_get_first(&gb_free_lst)) == NULL)
        {
            log_e(LOG_GB32960, "report buffer is full, discard the oldest one");

            if ((node = list_get_first(&gb_delay_lst)) == NULL &&
                (node = list_get_first(&gb_realtm_lst)) == NULL)
            {
                /* it should not be happened */
                log_e(LOG_GB32960, "BIG ERROR: no buffer to use.");

                while (1);
            }
        }

        DAT_UNLOCK();

        gb_errlst_head = gb_errlst_head->prev;
        err = list_entry(gb_errlst_head, gb_pack_t, link);
        rpt = list_entry(node, gb_pack_t, link);
        memcpy(rpt, err, sizeof(gb_pack_t));
        err->len  = 0;
        rpt->list = &gb_delay_lst;
        rpt->type = GB_RPTTYPE_DELAY;
        list_insert_after(&tmplst, node);
    }

    ERR_UNLOCK();

    if (!list_empty(&tmplst))
    {
        DAT_LOCK();
        /* append error data to delay list */
        tmplst.next->prev = gb_delay_lst.prev;
        gb_delay_lst.prev->next = tmplst.next;
        tmplst.prev->next = &gb_delay_lst;
        gb_delay_lst.prev = tmplst.prev;
        DAT_UNLOCK();
    }
}

void gb_data_flush_sending(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_last(&gb_trans_lst)) != NULL)
    {
        gb_pack_t *pack = list_entry(node, gb_pack_t, link);

        list_insert_after(pack->list, &pack->link);
    }

    DAT_UNLOCK();
}

void gb_data_flush_realtm(void)
{
    list_t *node;

    DAT_LOCK();

    while ((node = list_get_first(&gb_realtm_lst)) != NULL)
    {
        gb_pack_t *pack = list_entry(node, gb_pack_t, link);
        pack->list = &gb_delay_lst;
        pack->type = GB_RPTTYPE_DELAY;
        list_insert_before(&gb_delay_lst, &pack->link);
    }

    DAT_UNLOCK();
}

void gb_data_clear_report(void)
{
    int i;

    DAT_LOCK();
    list_init(&gb_realtm_lst);
    list_init(&gb_delay_lst);
    list_init(&gb_trans_lst);
    list_init(&gb_free_lst);

    for (i = 0; i < GB_MAX_REPORT; i++)
    {
        list_insert_before(&gb_free_lst, &gb_datamem[i].link);
    }

    DAT_UNLOCK();
}

void gb_data_clear_error(void)
{
    list_t *node;

    ERR_LOCK();

    if ((node = gb_errlst_head) == NULL)
    {
        ERR_UNLOCK();
        return;
    }

    do
    {
        list_entry(node, gb_pack_t, link)->len = 0;
        node = node->next;
    }
    while (node != gb_errlst_head);

    ERR_UNLOCK();
}

static void gb_data_parse_define(gb_info_t *gbinf, const char *str)
{
    assert(str != NULL);

    /* vehicle type */
    {
        uint32_t vtype;

        if (1 == sscanf(str, "BA_ \"IN_VEHITYPE\" %2x", &vtype) &&
            (vtype == GB_VEHITYPE_ELECT || vtype == GB_VEHITYPE_GASFUEL || GB_VEHITYPE_HYBIRD))
        {
            gbinf->vehi.vehi_type = vtype;
            return;
        }
    }

    /* engine state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_ENGINESTATE_%2x\" %2x", &index, &state))
        {
            gbinf->engin.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHISTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.state_tbl[index] = state;
            return;
        }
    }

    /* vehicle mode */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_VEHIMODE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.mode_tbl[index] = state;
            return;
        }
    }

    /* charge state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_CHGSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.charge_tbl[index] = state;
            return;
        }
    }

    /* DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_DCDCSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->vehi.dcdc_tbl[index] = state;
            return;
        }
    }

    /* High voltage DC-DC state */
    {
        uint32_t state, index;

        if (2 == sscanf(str, "BA_ \"IN_HVDCDCSTATE_%2x\" %2x", &index, &state))
        {
            gbinf->fuelcell.hvdcdc[index] = state;
            return;
        }
    }

    /* shift state */
    {
        char shift;
        uint32_t index;

        if (2 == sscanf(str, "BA_ \"IN_SHIFT_%2x\" %c", &index, &shift))
        {
            gbinf->vehi.shift_tbl[index] = shift;
            return;
        }
    }

    /* motor state */
    {
        uint32_t state, index, motor;

        if (3 == sscanf(str, "BA_ \"IN_MOTSTATE%u_%2x\" %2x", &motor, &index, &state) &&
            motor < GB_MAX_MOTOR)
        {
            gbinf->motor[motor].state_tbl[index] = state;
            return;
        }
    }
}

static int gb_data_parse_surfix(gb_info_t *gbinf, int sigid, const char *sfx)
{
    uint32_t gbtype, gbindex;

    assert(sigid > 0 && sfx != NULL);

    if (2 != sscanf(sfx, "G%1x%3x", &gbtype, &gbindex))
    {
        return 0;
    }

    switch (gbtype)
    {
        case GB_DATA_VEHIINFO:
            if (gbindex >= GB_MAX_VEHI_INFO)
            {
                log_e(LOG_GB32960, "vehicle info over %d! ", gbindex);
                break;
            }

            gbinf->vehi.info[gbindex] = sigid;
            break;

        case GB_DATA_MOTORINFO:
            if ((gbindex >> 8) >= GB_MAX_MOTOR)
            {
                log_e(LOG_GB32960, "motor ID over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= GB_MAX_MOTOR_INFO)
            {
                log_e(LOG_GB32960, "motor info over %d! ", gbindex & 0xff);
                break;
            }

            gbinf->motor[gbindex >> 8].info[gbindex & 0xff] = sigid;

            if ((gbindex >> 8) + 1 > gbinf->motor_cnt)
            {
                gbinf->motor_cnt = (gbindex >> 8) + 1;
            }

            break;
            
        case GB_DATA_FUELCELL:
            if(gbindex >= GB_FCINF_TEMPTAB)
            {
                gbinf->fuelcell.temp_cnt++;
                gbinf->fuelcell.temp[gbindex-GB_FCINF_TEMPTAB] = sigid;
            }
            else
            {
                if ((gbindex & 0xf) >= GB_FCINF_MAX)
                {
                    log_e(LOG_GB32960, "fuel cell info over %d! ", gbindex & 0xf);
                    break;
                }
                gbinf->fuelcell.info[gbindex & 0xf] = sigid;
            }
            break;
            
        case GB_DATA_ENGINEINFO:
            if (gbindex >= GB_MAX_ENGIN_INFO)
            {
                log_e(LOG_GB32960, "engine info over %d! ", gbindex);
                break;
            }

            gbinf->engin.info[gbindex] = sigid;
            break;

        case GB_DATA_EXTREMA:
            if (gbindex >= GB_MAX_EXTR_INFO)
            {
                log_e(LOG_GB32960, "extrema info over %d! ", gbindex);
                break;
            }

            gbinf->extr[gbindex] = sigid;
            break;

        case GB_DATA_WARNNING:
            if ((gbindex >> 8) >= 4)
            {
                log_e(LOG_GB32960, "warnning level over %d! ", gbindex >> 8);
                break;
            }

            if ((gbindex & 0xff) >= GB_MAX_WARN_INFO)
            {
                log_e(LOG_GB32960, "warnning number over %d! ", gbindex & 0xff);
                break;
            }

            gbinf->warn[gbindex >> 8][gbindex & 0xff] = sigid;
            break;

        case GB_DATA_BATTVOLT:
            if ((gbindex >> 10) >= 1)
            {
                log_e(LOG_GB32960, "battery number over %u! ", gbindex >> 10);
            }
            else if ((gbindex & 0x3ff) == 0x3fe)
            {
                gbinf->batt.voltage = sigid;
            }
            else if ((gbindex & 0x3ff) == 0x3ff)
            {
                gbinf->batt.current = sigid;
            }
            else if ((gbindex & 0x3ff) >= GB_MAX_PACK_CELL)
            {
                log_e(LOG_GB32960, "battery cells count over %u! ", gbindex & 0x3ff);
            }
            else if (gbinf->batt.cell[gbindex & 0x3ff] == 0)
            {
                gbinf->batt.cell[gbindex & 0x3ff] = sigid;
                gbinf->batt.cell_cnt++;
            }

            break;

        case GB_DATA_BATTTEMP:
            if ((gbindex >> 8) >= 1)
            {
                log_e(LOG_GB32960, "battery number over %d! ", gbindex >> 8);
            }
            else if ((gbindex & 0xff) >= GB_MAX_PACK_TEMP)
            {
                log_e(LOG_GB32960, "battery temperature count over %d! ", gbindex & 0xff);
            }
            else if (gbinf->batt.temp[gbindex & 0xff] == 0)
            {
                gbinf->batt.temp[gbindex & 0xff] = sigid;
                gbinf->batt.temp_cnt++;
            }

            break;

        case GB_DATA_VIRTUAL:
            log_i(LOG_GB32960, "get virtual channe %s", sfx);
            break;
#if GB_EXT
		case GB_DATA_EVENT:
		{
			if (gbindex >= GB_MAX_EVENT_INFO)
            {
                log_e(LOG_GB32960, "event info over %d! ", gbindex);
                break;
            }

            gbinf->event.info[gbindex] = sigid;
		}
		break;
		case GB_DATA_VSEXT:
		{
			if (gbindex >= GB_MAX_VSE_INFO)
            {
                log_e(LOG_GB32960, "VS ext info over %d! ", gbindex);
                break;
            }

            gbinf->gb_VSExt.info[gbindex] = sigid;
		}
		break;
		case GB_DATA_CONPST:
		{
			if (gbindex >= GB_MAX_VSE_INFO)
            {
                log_e(LOG_GB32960, "conp st info over %d! ", gbindex);
                break;
            }

            gbinf->gb_ConpSt.info[gbindex] = sigid;
		}
		break;
#endif
		case GB_DATA_ALARMFAULT:
		{
			if (gbindex >= GB_MAX_AF_INFO)
            {
                log_e(LOG_GB32960, "alarm fault info over %d! ", gbindex);
                break;
            }

            gbinf->gb_alarmFault.info[gbindex] = sigid;
		}
		break;
        default:
            log_o(LOG_GB32960, "unkonwn type %s%x", sfx ,gbtype);
       break;
    }

    return 5;
}


static int gb_data_dbc_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    static gb_info_t *gb_rld = NULL;
    int ret = 0;

    switch (event)
    {
        case DBC_EVENT_RELOAD:
            {
                gb_info_t *next;

                gb_rld = gb_inf == NULL ? gb_infmem : gb_inf->next;
                next = gb_rld->next;
                memset(gb_rld, 0, sizeof(gb_info_t));
                gb_rld->next = next;
                gb_rld->vehi.vehi_type = GB_VEHITYPE_ELECT;
                break;
            }

        case DBC_EVENT_FINISHED:
            if (gb_rld && arg1 == 0)
            {
                int i;

				for (i = 0; i < gb_rld->batt.cell_cnt && gb_rld->batt.cell[i]; i++);

				if (i < gb_rld->batt.cell_cnt)
				{
					log_e(LOG_GB32960, "battery cell defined in dbc is not incorrect");
					break;
				}

				for (i = 0; i < gb_rld->batt.temp_cnt && gb_rld->batt.temp[i]; i++);

				if (i < gb_rld->batt.temp_cnt)
				{
					log_e(LOG_GB32960, "temperature defined in dbc is not incorrect");
					break;
				}

                gb_inf = gb_rld;

                for (i = 0; i < GB_MAX_WARN_INFO; i++)
                {
                    if (gb_inf->warn[2][i] != 0)
                    {
                        dbc_set_signal_flag(gb_inf->warn[2][i], gb_warnflag);
                    }
                }

                ERR_LOCK();

                if (GROUP_SIZE(gb_inf) > 0)
                {
                    gb_errlst_head = &gb_errmem[0].link;
                    list_init(gb_errlst_head);

                    for (i = 1; i < GROUP_SIZE(gb_inf) * 30; i++)
                    {
                        gb_errmem[i].len  = 0;
                        gb_errmem[i].type = GB_RPTTYPE_DELAY;
                        gb_errmem[i].list = &gb_delay_lst;
                        list_insert_before(gb_errlst_head, &gb_errmem[i].link);
                    }
                }
                else
                {
                    gb_errlst_head = NULL;
                }

                ERR_UNLOCK();
                gb_data_clear_report();
            }

            gb_rld = NULL;
            break;

        case DBC_EVENT_DEFINE:
            if (gb_rld)
            {
                gb_data_parse_define(gb_rld, (const char *)arg1);
            }

            break;

        case DBC_EVENT_SURFIX:
            if (gb_rld)
            {
                ret = gb_data_parse_surfix(gb_rld, (int)arg1, (const char *)arg2);

                ret = PrvtProt_data_parse_surfix((int)arg1, (const char *)arg2);
            }

            break;

        case DBC_EVENT_UPDATE:
            if (gb_inf &&
                dbc_test_signal_flag((int)arg1, gb_warnflag, 0) &&
                dbc_get_signal_lastval((int)arg1) == 0 &&
                dbc_get_signal_value((int)arg1) != 0)
            {
                log_e(LOG_GB32960, "warnning triggered");
                gb_inf->warntrig = 1;
            }
        break;
        default:
            break;
    }

    return ret;
}

static void gb_data_periodic(gb_info_t *gbinf, int intv, uint32_t uptime)
{
    int period;
    static int ticks = 0, times = 0, triger = 0;

    if (gbinf->warntrig)
    {
        RTCTIME time;

        gbinf->warntrig = 0;
        times = 30 + 1;

        if (gb_pendflag)
        {
            gb_data_flush_realtm();
            gb_data_flush_error();
        }
        else
        {
            triger = 1;
        }

        can_get_time(uptime, &time);
        log_e(LOG_GB32960, "level 3 error start: %u, %02d/%02d/%02d, %02d:%02d:%02d",
              uptime, time.year, time.mon, time.mday, time.hour, time.min, time.sec);
    }

    ticks++;

    if (times == 0)
    {
        gb_data_save_error(gbinf, uptime);
        period = intv;
    }
    else if (--times == 0)
    {
        period = intv;
    }
    else
    {
        period = 1;
    }

    if (ticks >= period)
    {
        ticks = 0;
        gb_data_save_realtm(gbinf, uptime);

        if (triger)
        {
            triger = 0;
            gb_data_flush_error();
        }
    }
}

static int gb_data_can_cb(uint32_t event, uint32_t arg1, uint32_t arg2)
{
    //static int canact = 0;

    switch (event)
    {
        case CAN_EVENT_ACTIVE:
            canact = 1;
            break;

        case CAN_EVENT_SLEEP:
        case CAN_EVENT_TIMEOUT:
            canact = 0;
            gb_data_clear_error();
            break;

        case CAN_EVENT_DATAIN:
		{
			static int counter = 0;
			CAN_MSG *msg = (CAN_MSG *)arg1;

			while (canact && gb_inf && arg2--)
			{
				if (msg->type == 'T' && ++counter == 100)
				{
					counter = 0;
					gb_data_periodic(gb_inf, gb_datintv, msg->uptime);
				}
				if(msg->MsgID == 0x3D1)
				{
					PP_identificat_rcvdata(msg->Data);
					int i;
					for(i=0;i<8;i++)
					{
						log_o(LOG_GB32960,"data[%d] = %d",i,msg->Data[i]);
					}
				}
				msg++;
			}
#if GB_EXT
			if(gb_inf)
			{
				//log_i(LOG_GB32960, "event check report");
				gb_data_eventReport(gb_inf,msg->uptime);
			}
#endif
		}
		break;
        default:
        break;
    }

    return 0;
}


#include "gb32960_disp.h"

static int gb_shell_dumpgb(int argc, const char **argv)
{
    dbc_lock();

    if (gb_inf == NULL)
    {
        shellprintf(" dbc file is not loaded\r\n");
    }
    else
    {
        shellprintf(" [������Ϣ]\r\n");
        gb_disp_vinf(&gb_inf->vehi);

        if (gb_inf->vehi.vehi_type == GB_VEHITYPE_ELECT ||
            gb_inf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
        {
            int i;

            for (i = 0; i < gb_inf->motor_cnt; i++)
            {
                shellprintf(" [�����Ϣ-%d]\r\n", i + 1);
                gb_disp_minf(&gb_inf->motor[i]);
            }
            
            shellprintf(" [ȼ�ϵ����Ϣ]\r\n");
            gb_disp_finf(&gb_inf->fuelcell);

            shellprintf(" [�����Ϣ]\r\n");
            gb_disp_binf(&gb_inf->batt);
            shellprintf(" [��ֵ��Ϣ]  (���ȫ��δ���壬�����ն˼���)\r\n");
            gb_disp_xinf(gb_inf->extr);
        }

        if (gb_inf->vehi.vehi_type == GB_VEHITYPE_GASFUEL ||
            gb_inf->vehi.vehi_type == GB_VEHITYPE_HYBIRD)
        {
            shellprintf(" [��������Ϣ]\r\n");
            gb_disp_einf(&gb_inf->engin);
        }

        shellprintf(" [������Ϣ-1��]\r\n");
        gb_disp_winf(gb_inf->warn[0]);
        shellprintf(" [������Ϣ-2��]\r\n");
        gb_disp_winf(gb_inf->warn[1]);
        shellprintf(" [������Ϣ-3��]\r\n");
        gb_disp_winf(gb_inf->warn[2]);
    }

    dbc_unlock();
    return 0;
}


static int gb_shell_setintv(int argc, const const char **argv)
{
    uint16_t intv;

    if (argc != 1 || sscanf(argv[0], "%hu", &intv) != 1)
    {
        shellprintf(" usage: gbsetintv <interval seconds>\r\n");
        return -1;
    }

    if (intv == 0)
    {
        shellprintf(" error: data interval can't be 0\r\n");
        return -1;
    }

    if (gb_set_datintv(intv))
    {
        shellprintf(" error: call GB32960 API failed\r\n");
        return -2;
    }

    return 0;
}


static int gb_shell_testwarning(int argc, const const char **argv)
{
    int on;

    if (argc != 1 || sscanf(argv[0], "%d", &on) != 1)
    {
        shellprintf(" usage: gbtestwarn <0/1>\r\n");
        return -1;
    }

    dbc_lock();

    if (gb_inf)
    {
        if (!gb_inf->warntest && on)
        {
            gb_inf->warntrig = 1;
        }

        gb_inf->warntest = on;
    }

    dbc_unlock();
    return 0;
}

int gb_data_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            gb_infmem[0].next = gb_infmem + 1;
            gb_infmem[1].next = gb_infmem;
            gb_inf = NULL;
            gb_errlst_head = NULL;
            gb_datintv = 10;
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                uint32_t cfglen;

                gb_warnflag = dbc_request_flag();
                ret |= dbc_register_callback(gb_data_dbc_cb);
                ret |= can_register_callback(gb_data_can_cb);
                ret |= shell_cmd_register_ex("gbdump", "dumpgb", gb_shell_dumpgb,
                                             "show GB32960 signals information");
                ret |= shell_cmd_register_ex("gbsetintv", "setintvd", gb_shell_setintv,
                                             "set GB32960 report period");
                ret |= shell_cmd_register_ex("gbtestwarn", "errtrig", gb_shell_testwarning,
                                             "test GB32960 warnning");
                ret |= pthread_mutex_init(&gb_errmtx, NULL);
                ret |= pthread_mutex_init(&gb_datmtx, NULL);

                cfglen = sizeof(gb_datintv);
                ret |= cfg_get_para(CFG_ITEM_GB32960_INTERVAL, &gb_datintv, &cfglen);
                break;
            }
    }

    return ret;
}

void gb_data_put_back(gb_pack_t *pack)
{
    DAT_LOCK();
    list_insert_after(pack->list, &pack->link);
    DAT_UNLOCK();
}

void gb_data_put_send(gb_pack_t *pack)
{
    DAT_LOCK();
    list_insert_before(&gb_trans_lst, &pack->link);
    DAT_UNLOCK();
}

void gb_data_ack_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&gb_trans_lst)) != NULL)
    {
        list_insert_before(&gb_free_lst, node);
    }

    DAT_UNLOCK();
}

static int gb_emerg;

void gb_data_emergence(int set)
{
    DAT_LOCK();
    gb_emerg = set;
    DAT_UNLOCK();
}

gb_pack_t *gb_data_get_pack(void)
{
    list_t *node;

    DAT_LOCK();

    if ((node = list_get_first(&gb_realtm_lst)) == NULL)
    {
        node = gb_emerg ? list_get_last(&gb_delay_lst) : list_get_first(&gb_delay_lst);
    }

    DAT_UNLOCK();

    return node == NULL ? NULL : list_entry(node, gb_pack_t, link);;
}

int gb_data_nosending(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&gb_trans_lst);
    DAT_UNLOCK();
    return ret;
}

int gb_data_noreport(void)
{
    int ret;

    DAT_LOCK();
    ret = list_empty(&gb_realtm_lst) && list_empty(&gb_delay_lst);
    DAT_UNLOCK();
    return ret;
}

void gb_data_set_intv(uint16_t intv)
{
    gb_datintv = intv;
}

int gb_data_get_intv(void)
{
    return gb_datintv;
}

void gb_data_set_pendflag(int flag)
{
    gb_pendflag = flag;
}

/*
 	车辆状态״̬
*/
uint8_t gb_data_vehicleState(void)
{
	uint8_t st = 0;
    if(gb_inf && gb_inf->vehi.info[GB_VINF_STATE])
    {
    	st = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_STATE])->value;
    }

	return st;
}

/*
 	电量
*/
long gb_data_vehicleSOC(void)
{
	long soc = 0;
	if(gb_inf && gb_inf->vehi.info[GB_VINF_SOC])
	{
		soc = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_SOC])->value;
	}

	return  soc;
}

/*
 	 车辆里程
*/
long gb_data_vehicleOdograph(void)
{
	long totalodoMr = 0;

	if(gb_inf && gb_inf->vehi.info[GB_VINF_ODO])
	{
		totalodoMr = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_ODO])->value * 10;
	}

    return totalodoMr;
}

/*
 	 车速
*/
long gb_data_vehicleSpeed(void)
{
	long Vspeed = 0;

	if(gb_inf && gb_inf->vehi.info[GB_VINF_SPEED])
	{
		Vspeed = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_SPEED])->value * 10;
	}

    return Vspeed;
}

/*
 	 门锁状态״̬
*/
uint8_t gb_data_doorlockSt(void)
{
	uint8_t st = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_DRIDOORLOCKST])
	{
		st = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_DRIDOORLOCKST])->value;
	}
	return st;
}

/*
 	 后备箱状态״̬
*/
uint8_t gb_data_reardoorSt(void)
{
	uint8_t st = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_BACKDOORST])
	{
		st = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_BACKDOORST])->value;
	}
	return st;
}


/*
 	 空调开启/关闭状态״̬
*/
uint8_t gb_data_ACOnOffSt(void)
{
	uint8_t st = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_ACST])
	{
		st = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_ACST])->value;
	}
	return st;
}

/*
 	 主驾设定温度
*/
int gb_data_LHTemp(void)
{
	int st = 0;
	if(gb_inf && gb_inf->gb_ConpSt.info[GB_CMPT_LHTEMP])
	{
		st = dbc_get_signal_from_id(gb_inf->gb_ConpSt.info[GB_CMPT_LHTEMP])->value;
	}
	return st;
}

/*
 	充电状态״̬
*/
uint8_t gb_data_chargeSt(void)
{
	//uint8_t st = 0;
	//if(gb_inf->gb_VSExt.info[GB_VS_FSCHARGEST])
	//{
	//	st = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_FSCHARGEST])->value;
	//}

	return gb_chargeSt;
}

/*
 	后备箱门锁状态״̬
*/
uint8_t gb_data_reardoorlockSt(void)
{
	uint8_t st = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_REARDDOORLOCKST])//��������
	{
		if(dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_REARDDOORLOCKST])->value)
		{
			st = 0;//����
		}
		else
		{
			st = 1;//����
		}
	}

	return st;
}

/*
 	空调模式
*/
uint8_t gb_data_ACMode(void)
{
	uint8_t acmode = 3;//�Զ�
	int32_t tmp;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_ACMODE])//�յ�ģʽ
	{
		tmp = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_ACMODE])->value;
		switch(tmp)
		{
			case 1:
			case 5:
			case 7:
			{
				acmode = 1;//����
			}
			break;
			case 2:
			case 6:
			{
				acmode = 2;//����
			}
			break;
			default:
			break;
		}
	}

	return acmode;
}

/*
 	充电开启/关闭状态״̬
*/
uint8_t gb_data_chargeOnOffSt(void)
{
	uint8_t chargeSt = 0;

	if(gb_inf && gb_inf->gb_ConpSt.info[GB_CMPT_CHARGEST])
	{
		chargeSt = dbc_get_signal_from_id(gb_inf->gb_ConpSt.info[GB_CMPT_CHARGEST])->value;
	}

	return chargeSt;
}

/*
 	充电枪连接状态״̬
*/
uint8_t gb_data_chargeGunCnctSt(void)
{
	uint8_t chargeGunCnctSt = 0;
	if (gb_inf && gb_inf->vehi.info[GB_VINF_CHARGE])
	{
		chargeGunCnctSt = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_CHARGE])->value;
	}

	return chargeGunCnctSt;
}

/*
 	鼓风机档位״̬
*/
uint8_t gb_data_BlowerGears(void)
{
	uint8_t tmp;
	uint8_t fangears = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_AIRVOLUME])//
	{
		tmp = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_AIRVOLUME])->value;
		if(((tmp>=0)&&(tmp<=7)) || (tmp == 0xe))
		{
			fangears = tmp;
		}
		else
		{
			fangears = 0;
		}
	}

	return fangears;
}

/*
 	室外温度״̬
*/
uint8_t gb_data_outTemp(void)
{
	uint8_t temp = 150;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_OUTTEMP])//
	{
		temp = (dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_OUTTEMP])->value + 55) * 2;
	}

	return temp;
}

/*
 	室内温度״̬
*/
uint8_t gb_data_InnerTemp(void)
{
	uint8_t temp = 150;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_INTEMP])//
	{
		temp = (dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_INTEMP])->value + 55) * 2;
	}

	return temp;
}

/*
 	can bus active状态״̬
*/
uint8_t gb_data_CanbusActiveSt(void)
{
	return (uint8_t)canact;
}

/*
 	安全气囊状态״̬
*/
uint8_t gb_data_CrashOutputSt(void)
{
	uint8_t tmp;
	uint8_t crashSt = 0;
	if(gb_inf && gb_inf->gb_ConpSt.info[GB_CMPT_CRASHOUTPUTST])//
	{
		tmp = dbc_get_signal_from_id(gb_inf->gb_ConpSt.info[GB_CMPT_CRASHOUTPUTST])->value;
		if((tmp>=0)&&(tmp<=3))
		{
			crashSt = tmp;
		}
	}

	return crashSt;
}

/*
 	空调的温度״̬
*/
uint8_t gb_data_ACTemperature(void)
{
	uint8_t actemp = 20;//默认20度
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_ACTEMP])//空调温度
	{
		actemp = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_ACTEMP])->value;
	}

	return actemp;
}

/*
 	双闪状态״̬
*/
uint8_t gb_data_TwinFlashLampSt(void)
{
	uint8_t hlampSt = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_HLAMPST])//״̬
	{
		hlampSt = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_HLAMPST])->value;
	}

	return hlampSt;
}

/*
 * 示位灯状态
 */
uint8_t gb_data_PostionLampSt(void)
{
	uint8_t posLampSt = 0;
	if(gb_inf && gb_inf->event.info[GB_EVT_POSLAMP_ON])
	{
		posLampSt = dbc_get_signal_from_id(gb_inf->event.info[GB_EVT_POSLAMP_ON])->value;
	}

	return posLampSt;
}

/*
 * 近光灯状态
 */
uint8_t gb_data_NearLampSt(void)
{
	uint8_t lampSt = 0;
	if(gb_inf && gb_inf->event.info[GB_EVT_NEARLAMP_ON])//
	{
		lampSt = dbc_get_signal_from_id(gb_inf->event.info[GB_EVT_NEARLAMP_ON])->value;
	}
	return lampSt;
}

/*
 * 远光灯状态
 */
uint8_t gb_data_HighbeamLampSt(void)
{
	uint8_t lampSt = 0;
	if(gb_inf && gb_inf->event.info[GB_EVT_HIGHBEAMLAMP_ON])//
	{
		lampSt = dbc_get_signal_from_id(gb_inf->event.info[GB_EVT_HIGHBEAMLAMP_ON])->value;
	}
	return lampSt;
}

/*
 * 右前车胎压强
 */
uint8_t gb_data_frontRightTyrePre(void)
{
	uint8_t tyrePre = 0x0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_RFTYREPRESSURE])//
	{
		tyrePre = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_RFTYREPRESSURE])->value/10;
	}

	return tyrePre;
}

/*
 * 左前车胎压强
 */
uint8_t gb_data_frontLeftTyrePre(void)
{
	uint8_t tyrePre = 0x0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_LFTYREPRESSURE])//
	{
		tyrePre = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_LFTYREPRESSURE])->value/10;
	}

	return tyrePre;
}

/*
 * 右后车胎压强
 */
uint8_t gb_data_rearRightTyrePre(void)
{
	uint8_t tyrePre = 0x0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_RRTYREPRESSURE])//
	{
		tyrePre = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_RRTYREPRESSURE])->value/10;
	}

	return tyrePre;
}

/*
 * 左后车胎压强
 */
uint8_t gb_data_rearLeftTyrePre(void)
{
	uint8_t tyrePre = 0x0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_LRTYREPRESSURE])//
	{
		tyrePre = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_LRTYREPRESSURE])->value/10;
	}

	return tyrePre;
}

/*
 * 右前车胎温度
 */
uint8_t gb_data_frontRightTyreTemp(void)
{
	uint8_t temperature = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_RFTYRETEMP])//
	{
		temperature = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_RFTYRETEMP])->value + 50;
	}
	return temperature;
}

/*
 * 左前车胎温度
 */
uint8_t gb_data_frontLeftTyreTemp(void)
{
	uint8_t temperature = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_LFTYRETEMP])//
	{
		temperature = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_LFTYRETEMP])->value + 50;
	}
	return temperature;
}

/*
 * 右后车胎温度
 */
uint8_t gb_data_rearRightTyreTemp(void)
{
	uint8_t temperature = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_RRTYRETEMP])//
	{
		temperature = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_RRTYRETEMP])->value + 50;
	}
	return temperature;
}

/*
 * 左后车胎温度
 */
uint8_t gb_data_rearLeftTyreTemp(void)
{
	uint8_t temperature = 0;
	if(gb_inf && gb_inf->gb_VSExt.info[GB_VS_LRTYRETEMP])//
	{
		temperature = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_LRTYRETEMP])->value + 50;
	}
	return temperature;
}

uint8_t gb_data_gearPosition(void)
{
	uint8_t gearpos = 0;
	if (gb_inf  && gb_inf->vehi.info[GB_VINF_SHIFT])
	{
		uint8_t temp;
		gearpos = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_SHIFT])->value;
		temp = gb_inf->vehi.shift_tbl[gearpos];

		switch (temp)
		{
			case '1'...'6':
			{
				gearpos = temp - '0';
			}
			break;
			case 'R':
			{
				gearpos = 13;
			}
			break;
			case 'S':
			case 'D':
			{
				gearpos = 14;
			}
			break;
			case 'P':
			{
				gearpos = 15;
			}
			break;
			case 'N':
			{
			 gearpos = 0;
			}
			break;
			default:
			{
				gearpos = 0;
			}
			break;
		}
	}
	return gearpos;
}

/*
 绝缘电阻
*/
uint16_t  gb_data_insulationResistance(void)
{
	uint16_t temp = 0;
	if (gb_inf  && gb_inf->vehi.info[GB_VINF_INSULAT])
	{
		temp = MIN(dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_INSULAT])->value, 60000);
	}
	return temp;
}

/*
 加速踏板行程值有效范围
*/
uint8_t gb_data_acceleratePedalPrc(void)
{
	uint8_t temp  = 0;
	if (gb_inf  && gb_inf->vehi.info[GB_VINF_ACCPAD])
	{
		temp = gb_inf->vehi.info[GB_VINF_ACCPAD] ?
		dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_ACCPAD])->value : 0xff;
	}
	return temp;
}

/*
 制动踏板行程值有效范围
*/
uint8_t gb_data_deceleratePedalPrc(void)
{
	uint8_t temp = 0;
	if(gb_inf && gb_inf->vehi.info[GB_VINF_BRKPAD])
	{
		temp = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_BRKPAD])->value;
		if(temp >= 100)
		{
			temp = 100;
		}
	}

	return temp;
}

/*
 总电压
*/
uint16_t gb_data_batteryVoltage(void)
{
	/* total voltage, scale 0.1V */
	 uint16_t temp = 0;
	 if (gb_inf && gb_inf->vehi.info[GB_VINF_VOLTAGE])
	 {
		 temp = gb_inf->vehi.info[GB_VINF_VOLTAGE] ?
		dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_VOLTAGE])->value * 10 : 0xffff;
	 }
    return temp;
}

/*
 总电流
*/
uint16_t gb_data_batteryCurrent(void)
{
	uint16_t temp = 0;
	/* total curr, scale 0.1V, offset -1000A */
	if (gb_inf && gb_inf->vehi.info[GB_VINF_CURRENT])
	{
		temp = gb_inf->vehi.info[GB_VINF_CURRENT] ?
				(dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_CURRENT])->value + 1000) * 10: 0xffff;
	}
	return temp;
}

/*
 动力模式
*/
uint8_t gb_data_powermode(void)
{
	uint8_t temp;
	uint8_t mode = 0;
	if (gb_inf && gb_inf->vehi.info[GB_VINF_VEHIMODE])
	{
		temp = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_VEHIMODE])->value;
		 mode = gb_inf->vehi.mode_tbl[temp] ? gb_inf->vehi.mode_tbl[temp] : 0xff;
	}
	return mode;
}

/*
 充电状态
*/
uint8_t gb_data_chargestauus(void)
{
	uint8_t temp;
	uint8_t status = 0;
	if (gb_inf  && gb_inf->vehi.info[GB_VINF_CHARGE])
	{
		temp = dbc_get_signal_from_id(gb_inf->vehi.info[GB_VINF_CHARGE])->value;
		status = gb_inf->vehi.charge_tbl[temp] ? gb_inf->vehi.charge_tbl[temp] : 0xff;
	}
	return status;
}

/*
 *续航里程
 */
long gb_data_ResidualOdometer(void)
{
	long odometer = 0;
	if(gb_inf  && gb_inf->gb_VSExt.info[GB_VS_ENDURANCEMILE])//续航里程
	{
		odometer = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_ENDURANCEMILE])->value * 10;
	}

	return odometer;
}

/*
 *剩余充电时间
 */
long gb_data_ACChargeRemainTime(void)
{
	long acremaintime = 0;
	if(gb_inf  && gb_inf->gb_VSExt.info[GB_VS_REMAINCHRGTIME])//
	{
		acremaintime = dbc_get_signal_from_id(gb_inf->gb_VSExt.info[GB_VS_REMAINCHRGTIME])->value;
	}

	return acremaintime;
}
