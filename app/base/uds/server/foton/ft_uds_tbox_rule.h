#ifndef __FT_UDS_TBOX_RULE__
#define __FT_UDS_TBOX_RULE__

//e.g "H4HWS004180606"
#pragma pack(1)
/*SW [0][1]:MPU big version; [2]:MCU big version*/
/*
typedef struct UDS_DID_VER
{
    char item[2];
    char hswid[2];
    char stage;
    char ver[3];        
    char data[6];    
}UDS_DID_VER;
*/
typedef struct UDS_DID_VER
{
    char item[2];
    char hswid;
    char stage;
    char bver[2];
    char firmver[2];
    char mpuver[3];
    char mcuver[3];
}UDS_DID_VER;
#pragma pack(0) 

#pragma pack(1)
typedef struct DBC_NAME_STR
{
    char custom_name[32];       /*厂商*/
    char car_name[10];              /*车辆型号名称*/
    unsigned int type;         /*车辆型号编码*/
    unsigned int batnum;        /*电池单体数目*/
    unsigned int tnum;          /*温度检测点数目*/
    unsigned int power;         /*供电电压*/
    unsigned int baud;          /*CAN2波特率*/
    char ver[10];           /*版本*/
    char others[10];
}DBC_NAME_STR;
#pragma pack(0)
 
//char *get_uds_sw_ver(UDS_DID_VER* sw, char *data, uint8_t* hex2bcd);
char *get_uds_sw_ver(UDS_DID_VER *sw);

uint8_t *get_date_yymmdd(char *buf, uint8_t mode, uint8_t* hex2bcd);

int dbc_set_by_type(unsigned char type, unsigned char mode);

#endif