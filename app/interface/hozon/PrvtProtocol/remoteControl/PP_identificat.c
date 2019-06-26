/******************************************************
鏂囦欢鍚嶏細	PP_identificat.c

鎻忚堪锛�	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description锛� include the header file

*******************************************************/
#include <stdio.h>
#include "can_api.h"
#include "log.h"
#include "PPrmtCtrl_cfg.h"
#include "PP_identificat.h"

#define IDENTIFICAT_NUM 5
#define IDENTIFICAT_CAN_PORT 2

unsigned char PP_senddata[8] = { 0 };
unsigned char PP_recvdata[8] = { 0 };
static int PP_recv_can_flag = 0;
static int PP_stage = 0;
uint64_t PP_stage1_time = 0;
uint64_t PP_stage3_time = 0;
static int BDM_AuthenticationStatu = 0;
static uint64_t valid_time;
static int PP_authcnt = 0;
typedef unsigned char UINT8 ;
typedef unsigned int UINT32; 


static const UINT8 ConstSk[16]={0xA5,0x9E,0x2D,0x4B,0x49,0x18,0x0F,0x83,0x6E,0xA4,0xC5,0x48,0x55,0x15,0x5B,0xC3};
static UINT8 DataSk[16]={0x15,0x36,0xC2,0x89,0x61,0xD6,0x40,0x3F,0x9A,0xE7,0x26,0x4B,0xD9,0x96,0x7E,0x75};


static void XteaEncipher(UINT8 *DataSK, UINT8 *DataChall, UINT8 *DataResp);
extern int can_do_send(unsigned char port, CAN_SEND_MSG *msg);

/******************************************************
*鍑芥暟鍚嶏細XteaEncipher

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細鍔犲瘑鍑芥暟

*澶�  娉細
******************************************************/
static void XteaEncipher(UINT8 *DataSK, UINT8 *DataChall, UINT8 *DataResp)
{
	UINT32 v0, v1, i;
	UINT32 sum, delta;
	UINT32 v[2], k[4];
	/* Start XTEA algorithm */
	v[0] = (((UINT32)DataChall[0]) << 24) + (((UINT32)DataChall[1]) << 16) + (((UINT32)DataChall[2]) << 8) +(UINT32)DataChall[3];
	v[1] = (((UINT32)DataChall[4]) << 24) + (((UINT32)DataChall[5]) << 16) + (((UINT32)DataChall[6]) << 8) +(UINT32)DataChall[7];
	k[0] = (((UINT32)(DataSK[0]^ConstSk[0])) << 24) + (((UINT32)(DataSK[1]^ConstSk[1])) << 16) +(((UINT32)(DataSK[2]^ConstSk[2])) << 8) + ((UINT32)(DataSK[3]^ConstSk[3]));
	k[1] = (((UINT32)(DataSK[4]^ConstSk[4])) << 24) + (((UINT32)(DataSK[5]^ConstSk[5])) << 16) +(((UINT32)(DataSK[6]^ConstSk[6])) << 8) + ((UINT32)(DataSK[7]^ConstSk[7]));
	k[2] = (((UINT32)(DataSK[8]^ConstSk[8])) << 24) + (((UINT32)(DataSK[9]^ConstSk[9])) << 16) +(((UINT32)(DataSK[10]^ConstSk[10])) << 8) + ((UINT32)(DataSK[11]^ConstSk[11]));
	k[3] = (((UINT32)(DataSK[12]^ConstSk[12])) << 24) + (((UINT32)(DataSK[13]^ConstSk[13])) << 16) +(((UINT32)(DataSK[14]^ConstSk[14])) << 8) + ((UINT32)(DataSK[15]^ConstSk[15]));
	v0 = v[0];
	v1 = v[1];
	sum = 0;
	delta = 0x9E3779B9;
	for(i=0; i<32; i++)
	{
		v0 += ( ((v1 << 4)^(v1 >> 5)) + v1) ^ (sum + k[sum & 3]);
		sum += delta;
		v1 += ( ((v0 << 4)^(v0 >> 5)) + v0) ^ (sum + k[(sum >> 11) & 3]);
	}
	DataResp[0] = (UINT8)((v0&0xFF000000) >> 24);
	DataResp[1] = (UINT8)((v0&0x00FF0000) >> 16);
	DataResp[2] = (UINT8)((v0&0x0000FF00) >> 8);
	DataResp[3] = (UINT8)v0;
	DataResp[4] = (UINT8)((v1&0xFF000000) >> 24);
	DataResp[5] = (UINT8)((v1&0x00FF0000) >> 16);
	DataResp[6] = (UINT8)((v1&0x0000FF00) >> 8);
	DataResp[7] = (UINT8)v1;
}

/******************************************************
*鍑芥暟鍚嶏細PP_identificat_mainfunction

*褰�  鍙傦細void

*杩斿洖鍊硷細int

*鎻�  杩帮細璁よ瘉涓诲嚱鏁�

*澶�  娉細
******************************************************/
int PP_identificat_mainfunction()
{
	CAN_SEND_MSG msg;
	msg.MsgID     = 0x3D2;   //
	msg.DLC       = 8;       //
	msg.isEID     = 0;       //
	msg.isRTR     = 0;
	switch(PP_stage)
	{
		case PP_stage_idle://空闲
		{
			PP_authcnt = 0;
			PP_recv_can_flag = 0;
			PP_stage = PP_stage_start;
		}
		break;
		case PP_stage_start://开始认证
		{
			if(PP_authcnt < IDENTIFICAT_NUM)
			{
				memset(msg.Data,0,8*sizeof(uint8_t));
				can_do_send(IDENTIFICAT_CAN_PORT,&msg);
				log_i(LOG_HOZON,"can_do_send success");
				PP_stage1_time = tm_get_time();
				PP_stage = PP_stage_waitrandom;
				PP_authcnt++;
			}
			else
			{
				log_e(LOG_HOZON,"BDCM in trouble!!!!!!!!!");
				PP_stage = PP_stage_idle;
				return PP_AUTH_FAIL;
			}
		}
		break;
		case PP_stage_waitrandom://等待bcdm返回随机数
		{
			if(( tm_get_time() - PP_stage1_time) <= 500)
			{
				if(PP_recv_can_flag == 1)
				{
					PP_recv_can_flag = 0;
					PP_stage = PP_stage_sendenptdata;
					PP_authcnt = 0;
				}
			}
			else  //超时
			{
				PP_stage = PP_stage_start;
			}
		}
		break;
		case PP_stage_sendenptdata://发送加密数据
		{
			if(PP_authcnt < IDENTIFICAT_NUM)
			{
				memset(msg.Data,0,8*sizeof(uint8_t));
				XteaEncipher(DataSk,PP_recvdata,msg.Data);
				can_do_send(IDENTIFICAT_CAN_PORT,&msg);
				PP_stage3_time = tm_get_time();
				PP_authcnt++;
				PP_stage = PP_stage_waitauthokst;
			}
			else
			{
				log_e(LOG_HOZON,"BDCM in trouble!!!!!!!!!");
				PP_stage = PP_stage_idle;
				return PP_AUTH_FAIL;
			}
		}
		break;
		case PP_stage_waitauthokst://等待认证返回是否ok状态
		{
			if(( tm_get_time() - PP_stage3_time ) >= PP_RMTCTRL_CFG_CANSIGWAITTIME)//延时一段时间后判断can信号状态
			{
				if(( tm_get_time() - PP_stage3_time ) <= 500)
				{
					if(PP_rmtCtrl_cfg_AuthStatus() == 1)
					{
						valid_time = tm_get_time();
						BDM_AuthenticationStatu = 1; //
						PP_stage = PP_stage_idle;
						log_o(LOG_HOZON,"TBOX and DBCM certification succeeded");
						return PP_AUTH_SUCCESS;
					}
				}
				else//超时
				{
					PP_stage = PP_stage_sendenptdata;
				}
			}
		}
		break;
		default:
		break;
	}
	
	return 0;
}

/*
 * 检查认证状态
 */
int PP_get_identificat_flag()
{
	if(((tm_get_time() - valid_time) < 270000) && (BDM_AuthenticationStatu  == 1))  //
	{
		return 1;
	}
	else
	{
		BDM_AuthenticationStatu = 0 ;
		return 0;
	}
}

int PP_identificat_rcvdata(uint8_t *dt)
{
	int i;
	for(i=0;i<8;i++)
	{
		PP_recvdata[i] = dt[i];
	}
	PP_recv_can_flag = 1;   
	return 0;
}




