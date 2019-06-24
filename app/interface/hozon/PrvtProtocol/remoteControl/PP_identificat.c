/******************************************************
鏂囦欢鍚嶏細	PP_identificat.c

鎻忚堪锛�	浼佷笟绉佹湁鍗忚锛堟禉姹熷悎浼楋級
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description锛� include the header file

*******************************************************/

#include<stdio.h>
#include "PP_identificat.h"
#include "can_api.h"
#include "log.h"

#define IDENTIFICAT_NUM 5
#define IDENTIFICAT_CAN_PORT 1

unsigned char PP_senddata[8] = { 0 };
unsigned char PP_recvdata[8] = { 0 };
static int PP_recv_can_flag = 0;
static int PP_stage = 0;
uint64_t PP_stage1_time = 0;
uint64_t PP_stage3_time = 0;
static int BDM_AuthenticationStatu = 0;
static uint64_t valid_time;
static int cnt = 0;
typedef unsigned char UINT8 ;
typedef unsigned int UINT32; 


static const UINT8 ConstSk[16]={0xA5,0x9E,0x2D,0x4B,0x49,0x18,0x0F,0x83,0x6E,0xA4,0xC5,0x48,0x55,0x15,0x5B,0xC3};
static UINT8 DataSk[16]={0x15,0x36,0xC2,0x89,0x61,0xD6,0x40,0x3F,0x9A,0xE7,0x26,0x4B,0xD9,0x96,0x7E,0x75};


int PP_get_identificat_flag(void);

int PP_identificat_mainfunction(void);

void XteaEncipher(UINT8 *DataSK, UINT8 *DataChall, UINT8 *DataResp);
extern int can_do_send(unsigned char port, CAN_SEND_MSG *msg);

extern int PrvtProtcfg_AuthenticationStatu(void);

/******************************************************
*鍑芥暟鍚嶏細XteaEncipher

*褰�  鍙傦細void

*杩斿洖鍊硷細void

*鎻�  杩帮細鍔犲瘑鍑芥暟

*澶�  娉細
******************************************************/

void XteaEncipher(UINT8 *DataSK, UINT8 *DataChall, UINT8 *DataResp)
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
	msg.MsgID     = 0x3D2;   //娑堟伅ID
	msg.DLC       = 8;       //鏁版嵁闀垮害
	msg.isEID     = 0;       //鏍囧噯甯�
	msg.isRTR     = 0;
	switch(PP_stage)
	{
		case PP_stage1:    //tbox璁よ瘉stage1锛屼富鍔ㄥ彂閫�8涓瓧鑺�00
			{
				memset(msg.Data,0,8*sizeof(uint8_t));
				can_do_send(IDENTIFICAT_CAN_PORT,&msg);
				//log_o(LOG_HOZON,"can_do_send success");
				PP_stage1_time = tm_get_time();
				PP_stage = PP_stage3;
				PP_recv_can_flag = 0;
				break;
			}
		case PP_stage3:  //tbox璁よ瘉stage3,灏嗘敹鍒扮殑闅忔満鏁板姞瀵嗗彂鍑�,濡傛灉娌℃湁鏀跺埌鍥炲埌璁よ瘉stage1
			{
				if(( tm_get_time() - PP_stage1_time) <= 500) 
				{	
					if(PP_recv_can_flag == 1)
					{
						memset(msg.Data,0,8*sizeof(uint8_t));
						XteaEncipher(DataSk,PP_recvdata,msg.Data);
						can_do_send(IDENTIFICAT_CAN_PORT,&msg);
						PP_stage = PP_stage5;
						cnt = 0;
						PP_stage3_time = tm_get_time();
					}					
				}
				else  //濡傛灉娌℃湁鏀跺埌闅忔満鏁帮紝閲嶅彂8涓瓧鑺�00锛岀疮璁″彂5娆�
				{
					
					PP_stage = PP_stage1;
					cnt++;
					if(cnt == IDENTIFICAT_NUM)
					{  
						cnt = 0;
						//log_o(LOG_HOZON,"BDCM in trouble!!!!!!!!!");
					}
				}

				break;
			}
		case PP_stage5:
			{	//tbox璁よ瘉stage5锛屾敹鍒拌璇佺粨鏋�  ,浠庡浗鏍囦腑鑾峰彇淇″彿
				if(( tm_get_time() - PP_stage3_time ) <= 500)
				{
					if(PrvtProtcfg_AuthenticationStatu() == 1 )
					{
						valid_time = tm_get_time();
						BDM_AuthenticationStatu = 1; //re璁よ瘉鎴愬姛鏍囧織
						cnt = 0;
						PP_stage = PP_stage1;
						log_o(LOG_HOZON,"TBOX and DBCM certification succeeded");
					}
				}
				else
				{
					XteaEncipher(DataSk,PP_recvdata,msg.Data);
					can_do_send(IDENTIFICAT_CAN_PORT,&msg);
					PP_stage3_time = tm_get_time();
					cnt++;
					if(cnt == IDENTIFICAT_NUM)
					{
						BDM_AuthenticationStatu = 0; //璁よ瘉澶辫触
						valid_time = 0;
						cnt = 0;
						PP_stage = PP_stage1;
					}
				}
				break;
			}
		default: break;
	}
	
	return 0;
	 	
}




int PP_get_identificat_flag()
{
	//灏嗚璇佹椂闂存湁鏁堟湡璁句负4鍒�30绉�
	if(((tm_get_time() - valid_time) < 270000) && (BDM_AuthenticationStatu  == 1) && (PrvtProtcfg_AuthenticationStatu() == 1))  //鑾峰彇can淇″彿
	{
		return 1;	//璁よ瘉鏈夋晥
	}
	else
	{
		BDM_AuthenticationStatu = 0 ;//璁よ瘉澶辫触
		return 0;
	}

}

int PP_identificat_rcvdata(uint8_t *dt)  //鍦╟an瑙ｆ瀽閲岄潰璋冪敤
{
	int i;
	for(i=0;i<8;i++)
	{
		PP_recvdata[i] = dt[i];
	}
	PP_recv_can_flag = 1;   
	return 0;
}





