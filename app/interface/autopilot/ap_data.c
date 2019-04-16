#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/times.h>
#include "com_app_def.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "can_api.h"
#include "cfg_api.h"
#include "shell_api.h"
#include "timer.h"
#include "tcom_api.h"
#include "nm_api.h"
#include "sock_api.h"
#include "pm_api.h"
#include "gps_api.h"
#include "ap_api.h"
#include "ap_data.h"
#include "at.h"
#include "sha.h"
#include "aes.h"
#include "dev_rw.h"
#include "../support/protocol.h"
#include "tbox_limit.h"
#include "fota_api.h"
#include <ft_uds_tbox_rule.h>

#define AP_MAX_ECU        8

#define BMS_SUPPLIER_CODE                 "M0824"
#define BMS_MODEL_CODE                    "H4794010002A1"
#define BMS_SYSTEM_NAME                   "BMS"
#define BMS_HW_VERSION                    "H4HWS004180606"
#define BMS_SW_VERSION                    "H4SWS004180606"

#define MCU_SUPPLIER_CODE                 "M0824"
#define MCU_MODEL_CODE                    "H4794010002A1"
#define MCU_SYSTEM_NAME                   "MCU"
#define MCU_HW_VERSION                    "H4HWS004180606"
#define MCU_SW_VERSION                    "H4SWS004180606"

#define VCU_SUPPLIER_CODE                 "M0824"
#define VCU_MODEL_CODE                    "H4794010002A1"
#define VCU_SYSTEM_NAME                   "VCU"
#define VCU_HW_VERSION                    "H4HWS004180606"
#define VCU_SW_VERSION                    "H4SWS004180606"

#define IC_SUPPLIER_CODE                  "M0824"
#define IC_MODEL_CODE                     "H4794010002A1"
#define IC_SYSTEM_NAME                    "IC"
#define IC_HW_VERSION                     "H4HWS004180606"
#define IC_SW_VERSION                     "H4SWS004180606"

#define TBOX_SUPPLIER_CODE                "M0824"
#define TBOX_MODEL_CODE                   "H4794010002A0"
#define TBOX_SYSTEM_NAME                  "TBOX"
#define TBOX_HW_VERSION                   "H4HWA002180814"
#define TBOX_SW_VERSION                   "H4SWA033180820"


#define Nc (4)          // state数组和扩展密钥的列数
#define Nr (10)         // 加密轮数

//static uint8_t  key[16] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00};

static char      ap_vin[18];
static char      ap_tukey[17];
static char      tsp_tukey[16];


static char      ecu_index = 0;

unsigned char tu_key_check_flag_finish = 0;
extern AP_SOCK_INFO apinfo;

static AP_REG_TBL ap_tbl;
#if 0
static int  ap_notify_changed(char *msg, int len)
{
    int i, ret;

    for (i = 0; i < ap_tbl.used_num; i++)
    {
        ret = ap_tbl.item[i].changed(msg, len);

        if (ret != 0)
        {
            log_e(LOG_AUTO, "send net changed msg failed,ret:%d,i:%u", ret, i);
            return -1;
        }
    }

    return 0;
}
#endif
int ap_pack_head(uint8_t *buff)
{
    int pos = 0;

    buff[pos++] = 0x01; //Header version
    buff[pos++] = 0x01; //flag
    buff[pos++] = 0x35; //UNSIGNED
    at_get_imei((char *)&buff[pos]);
    pos = pos + 15;
    return pos;
}

int ap_pack_dispatcher(uint8_t *buff, unsigned char sid, unsigned char fid,
                       unsigned short eid, unsigned char len)
{
    static uint8_t count = 0;
    RTCTIME time;
    unsigned int temp_time = 0;

    int pos = 0;

    tm_get_abstime(&time);


    if (count == 256)
    {
        count = 0;
    }

    buff[pos++] = 0x02;
    buff[pos++] = 0x01;//type: VIN
    memcpy(&buff[pos], ap_vin, 17);
    pos = pos + 20;

    buff[pos++] = eid >> 8;   //Event id
    buff[pos++] = eid & 0xff;

    buff[pos++] = sid;   //Service id
    buff[pos++] = fid;   //Subfuction

    temp_time |= ((time.year - 2014) << 26);
    temp_time |= (time.mon << 22);
    temp_time |= (time.mday << 17);
    temp_time |= (time.hour << 12);
    temp_time |= (time.min << 6);
    temp_time |= time.sec;

    buff[pos++] = temp_time >> 24;
    buff[pos++] = temp_time >> 16;
    buff[pos++] = temp_time >> 8;
    buff[pos++] = temp_time;

    buff[pos++] = count++; //Uplink counter
    buff[pos++] = 0;       //down counter

    buff[pos++] = 0;
    buff[pos++] = 0;
    buff[pos++] = 20 + len + 20;

    return pos;
}

int ap_pack_ecu_info_body(uint8_t *buff)
{
    int pos = 0;
    int length;
    foton_ecu_info_t ecu_i[AP_MAX_ECU];
    int i, cnt = foton_get_ecu_info(ecu_i, AP_MAX_ECU);

    if (!cnt)
    {
        shellprintf(" can't get ECU information\r\n");
    }

    for (i = 0; i < cnt; i++)
    {
        //log_i(LOG_AUTO, "name: %s", ecu_i[i].name);
        //log_i(LOG_AUTO, "partno: %s", ecu_i[i].partno);
        //log_i(LOG_AUTO, "supplier: %s", ecu_i[i].supplier);
        //log_i(LOG_AUTO, "hwver: %s", ecu_i[i].hwver);
        //log_i(LOG_AUTO, "swver: %s", ecu_i[i].swver);

        memcpy(buff + pos,  ecu_i[i].name, strlen(ecu_i[i].name));
        pos += 14;

        memcpy(buff + pos,  ecu_i[i].partno, strlen(ecu_i[i].partno));
        pos += 13;

        memcpy(buff + pos,  ecu_i[i].supplier, strlen(ecu_i[i].supplier));
        pos += 5;

        memcpy(buff + pos,  ecu_i[i].hwver, strlen(ecu_i[i].hwver));
        pos += 14;

        memcpy(buff + pos,  ecu_i[i].swver, strlen(ecu_i[i].swver));
        pos += 14;
    }

    memcpy(buff + pos,  TBOX_SYSTEM_NAME, strlen(TBOX_SYSTEM_NAME));
    pos += 14;

    memcpy(buff + pos,  TBOX_MODEL_CODE, strlen(TBOX_MODEL_CODE));
    pos += 13;

    memcpy(buff + pos,  TBOX_SUPPLIER_CODE, strlen(TBOX_SUPPLIER_CODE));
    pos += 5;

    length = 14;
    if(0 != cfg_get_para(CFG_ITEM_FT_UDS_HW, buff + pos,(unsigned int *)&length))
    {
        log_e(LOG_UDS, "get ft tsp hw error");
    }
    //memcpy(buff + pos,  TBOX_HW_VERSION, strlen(TBOX_HW_VERSION));
    pos += 14;

    get_uds_sw_ver((UDS_DID_VER*)(buff + pos));
    //memcpy(buff + pos,  TBOX_SW_VERSION, strlen(TBOX_SW_VERSION));
    pos += 14;

    log_i(LOG_AUTO, "pos = %d", pos);

    return pos;

    #if 0
    int pos = 0;

    //else if (3 == ecu_index % 5) /* VCU */
    {
        memcpy(buff,  VCU_SYSTEM_NAME, strlen(VCU_SYSTEM_NAME));
        pos += strlen(VCU_SYSTEM_NAME);

        memcpy(buff + pos,  VCU_MODEL_CODE, strlen(VCU_MODEL_CODE));
        pos += strlen(VCU_MODEL_CODE);

        memcpy(buff + pos,  VCU_SUPPLIER_CODE, strlen(VCU_SUPPLIER_CODE));
        pos += strlen(VCU_SUPPLIER_CODE);

        memcpy(buff + pos,  VCU_HW_VERSION, strlen(VCU_HW_VERSION));
        pos += strlen(VCU_HW_VERSION);

        memcpy(buff + pos,  VCU_SW_VERSION, strlen(VCU_SW_VERSION));
        pos += strlen(VCU_SW_VERSION);
    }

    //else if (1 == ecu_index % 5) /* BMS */
    {
        memcpy(buff,  BMS_SYSTEM_NAME, strlen(BMS_SYSTEM_NAME));
        pos += strlen(BMS_SYSTEM_NAME);

        memcpy(buff + pos,  BMS_MODEL_CODE, strlen(BMS_MODEL_CODE));
        pos += strlen(BMS_MODEL_CODE);

        memcpy(buff + pos,  BMS_SUPPLIER_CODE, strlen(BMS_SUPPLIER_CODE));
        pos += strlen(BMS_SUPPLIER_CODE);

        memcpy(buff + pos,  BMS_HW_VERSION, strlen(BMS_HW_VERSION));
        pos += strlen(BMS_HW_VERSION);

        memcpy(buff + pos,  BMS_SW_VERSION, strlen(BMS_SW_VERSION));
        pos += strlen(BMS_SW_VERSION);
    }

    //else if (2 == ecu_index % 5) /* MCU */
    {
        memcpy(buff,  MCU_SYSTEM_NAME, strlen(MCU_SYSTEM_NAME));
        pos += strlen(MCU_SYSTEM_NAME);

        memcpy(buff + pos,  MCU_MODEL_CODE, strlen(MCU_MODEL_CODE));
        pos += strlen(MCU_MODEL_CODE);

        memcpy(buff + pos,  MCU_SUPPLIER_CODE, strlen(MCU_SUPPLIER_CODE));
        pos += strlen(MCU_SUPPLIER_CODE);

        memcpy(buff + pos,  MCU_HW_VERSION, strlen(MCU_HW_VERSION));
        pos += strlen(MCU_HW_VERSION);

        memcpy(buff + pos,  MCU_SW_VERSION, strlen(MCU_SW_VERSION));
        pos += strlen(MCU_SW_VERSION);
    }

    //else if (4 == ecu_index % 5) /* IC */
    {
        memcpy(buff,  IC_SYSTEM_NAME, strlen(IC_SYSTEM_NAME));
        pos += strlen(IC_SYSTEM_NAME);

        memcpy(buff + pos,  IC_MODEL_CODE, strlen(IC_MODEL_CODE));
        pos += strlen(IC_MODEL_CODE);

        memcpy(buff + pos,  IC_SUPPLIER_CODE, strlen(IC_SUPPLIER_CODE));
        pos += strlen(IC_SUPPLIER_CODE);

        memcpy(buff + pos,  IC_HW_VERSION, strlen(IC_HW_VERSION));
        pos += strlen(IC_HW_VERSION);

        memcpy(buff + pos,  IC_SW_VERSION, strlen(IC_SW_VERSION));
        pos += strlen(IC_SW_VERSION);
    }

    //if (0 == ecu_index % 5) /* TBOX */
    {
        memcpy(buff,  TBOX_SYSTEM_NAME, strlen(TBOX_SYSTEM_NAME));
        pos += strlen(TBOX_SYSTEM_NAME);

        memcpy(buff + pos,  TBOX_MODEL_CODE, strlen(TBOX_MODEL_CODE));
        pos += strlen(TBOX_MODEL_CODE);

        memcpy(buff + pos,  TBOX_SUPPLIER_CODE, strlen(TBOX_SUPPLIER_CODE));
        pos += strlen(TBOX_SUPPLIER_CODE);

        memcpy(buff + pos,  TBOX_HW_VERSION, strlen(TBOX_HW_VERSION));
        pos += strlen(TBOX_HW_VERSION);

        memcpy(buff + pos,  TBOX_SW_VERSION, strlen(TBOX_SW_VERSION));
        pos += strlen(TBOX_SW_VERSION);
    }

    return pos;
    #endif
}


static int ap_do_write(int fd, unsigned char *buf, unsigned int len)
{
    int writecnt = 0;
    int ret;

    if (fd < 0)
    {
        log_e(LOG_AUTO, "dev_write invalid fd");
        return DEV_RW_INVALID_PARAMETER;
    }

    while (writecnt < len)
    {
        ret = send(fd, buf + writecnt, len - writecnt, MSG_DONTWAIT);

        if (ret > 0)
        {
            writecnt +=  ret;
        }
        else if (0 == ret)   /* timeout */
        {
            continue;        /* continue to monitor the incomging data */
        }
        else
        {
            if (EINTR == errno)  /* interrupted by signal */
            {
                continue;
            }

            log_e(LOG_AUTO, "write ret:%s", strerror(errno));
            return DEV_RW_READ_FAILED;
        }
    }

    return 0;
}

int ap_do_report_hb(int fd)
{
    int ret;
    int pos = 0;
    int out_len;
    static uint8_t data[1024];
    SHA1_CONTEXT ctx;
    static int hb_interval = 0;

    if (hb_interval < 60)
    {
        hb_interval++;
        return 0;
    }

    hb_interval = 0;

    memset(data, 0, sizeof(data));

    data[pos++] = 0x5f;
    data[pos++] = 0x8a;
    data[pos++] = 0xbb;
    data[pos++] = 0xcd;

    pos += ap_pack_head(data + pos);

    pos += ap_pack_dispatcher(data + pos, 0x60, 0x81, 0, 0);

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, &data[4], pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, (data + 4), pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    //protocol_dump(LOG_AUTO, "DATA(HB)", data, pos, 1);

    out_len = (pos - 22) + 16 - (pos - 22) % 16;
    add_pkcs_padding(&data[22], out_len, pos - 22);

    CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
    pos = out_len + 22;
    data[pos++] = 0xb2;
    data[pos++] = 0x5e;
    data[pos++] = 0x38;
    data[pos++] = 0xa2;

    protocol_dump(LOG_AUTO, "AUTO(HB)", data, pos, 1);

    ret = ap_do_write(fd, data, pos);

    if (ret != 0)
    {
        log_e(LOG_AUTO, "socket send failedl, ret:%d", ret);
    }
    else
    {
        log_i(LOG_AUTO, "send(%d) hb successful", fd);
    }

    return ret;
}

int ap_do_report_request_tukey(int fd)
{
    int ret;
    int pos = 0;
    int out_len;
    static uint8_t data[1024];
    SHA1_CONTEXT ctx;
   // static int hb_interval = 0;

    memset(data, 0, sizeof(data));

    data[pos++] = 0x5f;
    data[pos++] = 0x8a;
    data[pos++] = 0xbb;
    data[pos++] = 0xcd;

    pos += ap_pack_head(data + pos);

    pos += ap_pack_dispatcher(data + pos, 0x73, 0x01, 0, 0);

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, &data[4], pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, (data + 4), pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    //protocol_dump(LOG_AUTO, "DATA(HB)", data, pos, 1);

    out_len = (pos - 22) + 16 - (pos - 22) % 16;
    add_pkcs_padding(&data[22], out_len, pos - 22);

    CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
    pos = out_len + 22;
    data[pos++] = 0xb2;
    data[pos++] = 0x5e;
    data[pos++] = 0x38;
    data[pos++] = 0xa2;

    protocol_dump(LOG_AUTO, "AUTO(TuKey)", data, pos, 1);

    ret = ap_do_write(fd, data, pos);

    if (ret != 0)
    {
        log_e(LOG_AUTO, "socket send failedl, ret:%d", ret);
    }
    else
    {
        log_i(LOG_AUTO, "send(%d) TuKey successful", fd);
    }

    return ret;
}
void ap_do_report_request_tukey_qet(void)
{
	ap_do_report_request_tukey(apinfo.sockfd);
}
int ap_do_report_new_tu_key_check(int fd)
{
    int ret;
    int pos = 0,ii=0;
    int out_len;
    static uint8_t data[1024];
    SHA1_CONTEXT ctx;
   // static int ecu_interval = 0;

    memset(data, 0, sizeof(data));

    data[pos++] = 0x5f;
    data[pos++] = 0x8a;
    data[pos++] = 0xbb;
    data[pos++] = 0xcd;

    pos += ap_pack_head(data + pos);

    pos += ap_pack_dispatcher(data + pos, 0x73, 0x03, 0, 60);

    //protocol_dump(LOG_AUTO, "AUTO", data, pos, 1);

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, &data[4], pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    for(ii = 0;ii < 16;ii++)
    {
       data[pos+ii]=tsp_tukey[ii];
    }
	pos = pos + 16;
    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, (data + 4), pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    //protocol_dump(LOG_AUTO, "DATA(ECU)", data, pos, 1);

    out_len = (pos - 22) + 16 - (pos - 22) % 16;
    add_pkcs_padding(&data[22], out_len, pos - 22);

    CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
    pos = out_len + 22;
    data[pos++] = 0xb2;
    data[pos++] = 0x5e;
    data[pos++] = 0x38;
    data[pos++] = 0xa2;

    protocol_dump(LOG_AUTO, "AUTO(ECU)", data, pos, 1);

    ret = ap_do_write(fd, data, pos);

    if (ret != 0)
    {
        log_e(LOG_AUTO, "socket send failed, ret:%d", ret);
    }
    else
    {
        log_i(LOG_AUTO, "send(%d) ecu successful", fd);
    }

    return ret;



}

int ap_do_report_ecu_info(int fd)
{
    int ret;
    int pos = 0;
    int out_len;
    static uint8_t data[1024];
    SHA1_CONTEXT ctx;
    static int ecu_interval = 0;

    if (ecu_interval < 10)
    {
        ecu_interval++;
        return 0;
    }

    ecu_interval = 0;

    memset(data, 0, sizeof(data));

    data[pos++] = 0x5f;
    data[pos++] = 0x8a;
    data[pos++] = 0xbb;
    data[pos++] = 0xcd;

    pos += ap_pack_head(data + pos);

    pos += ap_pack_dispatcher(data + pos, 0x66, 0x02, 0, 60);

    //protocol_dump(LOG_AUTO, "AUTO", data, pos, 1);

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, &data[4], pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    pos += ap_pack_ecu_info_body(data + pos);

    memset(&ctx, 0, sizeof(ctx));
    sha1_init(&ctx);
    sha1_write(&ctx, (data + 4), pos - 4);
    sha1_final(&ctx);
    memcpy(&data[pos], ctx.buf, 20);
    pos = pos + 20;

    //protocol_dump(LOG_AUTO, "DATA(ECU)", data, pos, 1);

    out_len = (pos - 22) + 16 - (pos - 22) % 16;
    add_pkcs_padding(&data[22], out_len, pos - 22);

    CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
    pos = out_len + 22;
    data[pos++] = 0xb2;
    data[pos++] = 0x5e;
    data[pos++] = 0x38;
    data[pos++] = 0xa2;

    protocol_dump(LOG_AUTO, "AUTO(ECU)", data, pos, 1);

    ret = ap_do_write(fd, data, pos);

    if (ret != 0)
    {
        log_e(LOG_AUTO, "socket send failed, ret:%d", ret);
    }
    else
    {
        log_i(LOG_AUTO, "send(%d) ecu successful", fd);
    }

    return ret;
}

static void str_to_hex(uint8_t *src, int len, uint8_t *dst)
{
    int h1 = 0, h2 = 0;
    int i;

    for (i = 0; i < len; i=i+2)
    {

      if(src[i]>='0' && src[i]<='9') h1=src[i]-'0';
      else if(src[i]>='A' && src[i]<='Z') h1=src[i]-'A'+0x0A;
      else if(src[i]>='a' && src[i]<='z') h1=src[i]-'a'+0x0a; 
        
	 if(src[i+ 1]>='0' && src[i+1]<='9') h2=src[i+1]-'0';
     else if(src[ i+1]>='A' && src[i+1]<='Z') h2=src[ i+1]-'A'+0x0A;
	 else if(src[i+1]>='a' && src[i+1]<='z') h2=src[i+1]-'a'+0x0a; 

      dst[i/2] = h1 * 16 + h2;
    }
}

void ap_msg_proc(AP_SOCK_INFO* info, unsigned char *msg, unsigned int len)
{
    unsigned char sid, fid;
    static uint8_t data[1024];
    int pos = 0;
    int out_len, ret;
    unsigned short eid;
    SHA1_CONTEXT ctx;

    InvCipherString(&msg[22], len - 22 - 4,(unsigned char*)tsp_tukey);

    eid = (msg[44] << 8) | msg[45];
    sid = msg[46];
    fid = msg[47];

    if (len <= 77 + 24)
    {
        log_e(LOG_AUTO, "invalid len:%u", len);
        return;
    }

    if (0x16 == sid && 0x01 == fid)   /* update request */
    {
        char buf[256];
        uint8_t md5[32];
        uint8_t hex_md5[16];
        uint8_t mode;
        int body_len = msg[54] * 65536 + msg[55] * 256 + msg[56] - 40;
        uint8_t *body = msg + 77;

        //log_i(LOG_AUTO, "recv update request");

        //protocol_dump(LOG_AUTO, "req msg data:", &msg[77+2], len - 77 -2 - 24, 0);

        //ap_notify_changed((char *)&msg[77], body_len);
        //log_i(LOG_AUTO, "get update url: %.*s", body_len - 32, body + 32);

        if (body_len - 32 >= 256)
        {
            log_e(LOG_AUTO, "url is too long");
        }
        else
        {
            memcpy(md5, body, 32);
           // printf("wxxwang platform_md5=%s",md5);
            mode = body[32];
            strncpy(buf, (char *)(body + 32 + 1), body_len - 32 -1);
            buf[body_len - 32 - 1] = 0;
            str_to_hex(md5,32,hex_md5);
            protocol_dump(LOG_AUTO, "md5:", hex_md5, 16, 0);
            log_i(LOG_AUTO, "update mode: %d", mode);
            log_i(LOG_AUTO, "update url: %s", buf);
            fota_silent(mode);//不使用静默模式
            fota_new_request(buf, hex_md5, 0);
        }

        memset(data, 0, sizeof(data));

        data[pos++] = 0x5f;
        data[pos++] = 0x8a;
        data[pos++] = 0xbb;
        data[pos++] = 0xcd;

        pos += ap_pack_head(data + pos);
        pos += ap_pack_dispatcher(data + pos, 0x66, 0x01, eid, 0);

        memset(&ctx, 0, sizeof(ctx));
        sha1_init(&ctx);
        sha1_write(&ctx, &data[4], pos - 4);
        sha1_final(&ctx);
        memcpy(&data[pos], ctx.buf, 20);
        pos = pos + 20;

        memset(&ctx, 0, sizeof(ctx));
        sha1_init(&ctx);
        sha1_write(&ctx, (data + 4), pos - 4);
        sha1_final(&ctx);
        memcpy(&data[pos], ctx.buf, 20);
        pos = pos + 20;

        out_len = (pos - 22) + 16 - (pos - 22) % 16;
        add_pkcs_padding(&data[22], out_len, pos - 22);

        CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
        pos = out_len + 22;
        data[pos++] = 0xb2;
        data[pos++] = 0x5e;
        data[pos++] = 0x38;
        data[pos++] = 0xa2;

        protocol_dump(LOG_AUTO, "update answer", data, pos, 1);

        ret = ap_do_write(info->sockfd, data, pos);

        if (ret != 0)
        {
            log_e(LOG_AUTO, "socket send failed, ret:%d", ret);
        }
        else
        {
            log_i(LOG_AUTO, "send(%d) successful", info->sockfd);
        }
    }
    else if (0x16 == sid && 0x02 == fid)   /* ack for ecu info report*/
    {
        info->waitcnt++;
        log_i(LOG_AUTO, "recv the ack of ecu info report");
        ecu_index++;
    }
    else if (0x23 == sid)
    {
      //  int body_len = msg[54] * 65536 + msg[55] * 256 + msg[56] - 40;
        uint8_t *body = msg + 77;
       // uint8_t tu_key[16];
        unsigned int length = 0;
        if (0x01 == fid)
        {
             log_i(LOG_AUTO, "recv the ack of request key(0x01) report");
        }
        else if (0x02 == fid)
        {
            memcpy(tsp_tukey, body, 16); 
            memcpy(ap_tukey, tsp_tukey, 16); 
            
            length = 17;
            ap_tukey[16] = 0;
            cfg_set_para(CFG_ITEM_FTTSP_TUKEY, (void *)ap_tukey, length);
           // printf("wang %x %x %x %x \r\n",tsp_tukey[0],tsp_tukey[1],tsp_tukey[2],tsp_tukey[3]);

            memset(data, 0, sizeof(data));

            data[pos++] = 0x5f;
            data[pos++] = 0x8a;
            data[pos++] = 0xbb;
            data[pos++] = 0xcd;

            pos += ap_pack_head(data + pos);
            pos += ap_pack_dispatcher(data + pos, 0x73, 0x02, eid, 0);

            memset(&ctx, 0, sizeof(ctx));
            sha1_init(&ctx);
            sha1_write(&ctx, &data[4], pos - 4);
            sha1_final(&ctx);
            memcpy(&data[pos], ctx.buf, 20);
            pos = pos + 20;

            memset(&ctx, 0, sizeof(ctx));
            sha1_init(&ctx);
            sha1_write(&ctx, (data + 4), pos - 4);
            sha1_final(&ctx);
            memcpy(&data[pos], ctx.buf, 20);
            pos = pos + 20;

            out_len = (pos - 22) + 16 - (pos - 22) % 16;
            add_pkcs_padding(&data[22], out_len, pos - 22);

            CipherString(&data[22], out_len,(unsigned char*)tsp_tukey);
            pos = out_len + 22;
            data[pos++] = 0xb2;
            data[pos++] = 0x5e;
            data[pos++] = 0x38;
            data[pos++] = 0xa2;

            protocol_dump(LOG_AUTO, "hu_key answer", data, pos, 1);

            ret = ap_do_write(info->sockfd, data, pos);

            if (ret != 0)
            {
                log_e(LOG_AUTO, "socket send failed, ret:%d", ret);
            }
            else
            {
                log_i(LOG_AUTO, "send(%d) successful", info->sockfd);
                ap_do_report_new_tu_key_check(apinfo.sockfd);
            }
          
        }
        else if (0x03 == fid)
        {
             tu_key_check_flag_finish = 1;
             log_i(LOG_AUTO, "recv the ack of request key(0x03) report");
        }
    }
}

int ap_recv_proc(AP_SOCK_INFO* info, unsigned char *msg, unsigned int *len)
{
    int i = 0, j, start_index = -1, end_index = -1;

    while (i < *len)
    {
        if (-1 == start_index)
        {
            if ((0x5f == msg[i]) && (0x8a == msg[i + 1]) && (0xbb == msg[i + 2]) && (0xcd == msg[i + 3]))
            {
                start_index = i;
                i = i + 4;
            }
            else
            {
                i++;
            }
        }
        else
        {
            if ((0xb2 == msg[i]) && (0x5e == msg[i + 1]) && (0x38 == msg[i + 2]) && (0xa2 == msg[i + 3]))
            {
                end_index = i;
                break;
            }
            else
            {
                i++;
            }
        }
    }

    /* one msg is found */
    if (start_index >= 0 && end_index >= 0)
    {
        ap_msg_proc(info, msg + start_index, end_index - start_index + 1);
    }

    /* one msg is found */
    if (end_index >= 0)
    {
        for (j = end_index + 1; j < *len; j++)
        {
            msg[j - end_index] = msg[j];
        }

        *len = *len - end_index - 1;
    }
    else if (start_index >= 0)
    {
        for (j = start_index + 1; j < *len; j++)
        {
            msg[j - start_index] = msg[j];
        }

        *len = *len - start_index - 1;
    }
    else
    {
        *len = 0;
    }

    return 0;
}

static int ap_shell_setvin(int argc, const char **argv)
{
    if (argc != 1)
    {
        shellprintf(" usage: gbsetaddr <server url> <server port>\r\n");
        return -1;
    }

    if (strlen(argv[0]) != 17)
    {
        shellprintf(" error: url length is not 17 charactores\r\n");
        return -1;
    }

    cfg_set_para(CFG_ITEM_FTTSP_VIN, (unsigned char *)argv[0], 18);

    shellprintf(" note: reboot to take effect...........\r\n");

    return 0;
}


static int ap_shell_setaddr(int argc, const char **argv)
{
    uint16_t port;

    if (argc != 2)
    {
        shellprintf(" usage: gbsetaddr <server url> <server port>\r\n");
        return -1;
    }

    if (strlen(argv[0]) > 63)
    {
        shellprintf(" error: url length can't over 63 charactores\r\n");
        return -1;
    }

    if (sscanf(argv[1], "%hu", &port) != 1)
    {
        shellprintf(" error: \"%s\" is a invalid port\r\n", argv[1]);
        return -1;
    }

    cfg_set_para(CFG_ITEM_FTTSP_URL, (unsigned char *)argv[0], 256);
    cfg_set_para(CFG_ITEM_FTTSP_PORT, (unsigned char *)&port, sizeof(port));

    shellprintf(" note: reboot to take effect...........\r\n");

    return 0;
}


int ap_update_evt_reg(ap_update_notify callback)
{
    /* the paramerter is invalid */
    if (NULL == callback)
    {
        log_e(LOG_AUTO, "callback is NULL");
        return -1;
    }

    if (ap_tbl.used_num >= AP_MAX_REG_TBL)
    {
        log_e(LOG_AUTO, "ap register table overflow");
        return -1;
    }

    ap_tbl.item[ap_tbl.used_num].changed = callback;
    ap_tbl.used_num++;

    return 0;
}


int ap_data_init(INIT_PHASE phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            memset(&ap_tbl, 0 , sizeof(ap_tbl));
            break;

        case INIT_PHASE_RESTORE:
            break;

        case INIT_PHASE_OUTSIDE:
            {
                unsigned int len;
                len = sizeof(ap_vin);
                ret |= cfg_get_para(CFG_ITEM_FTTSP_VIN, ap_vin, &len);  

                len = sizeof(ap_tukey);
                ret |= cfg_get_para(CFG_ITEM_FTTSP_TUKEY, ap_tukey, &len);
                memcpy(tsp_tukey,ap_tukey,16);
                printf("tukey %x %x %x %x\r\n",tsp_tukey[0],tsp_tukey[1],tsp_tukey[2],tsp_tukey[3]);
                printf("      %x %x %x %x\r\n",tsp_tukey[4],tsp_tukey[5],tsp_tukey[6],tsp_tukey[7]);
                printf("      %x %x %x %x\r\n",tsp_tukey[8],tsp_tukey[9],tsp_tukey[10],tsp_tukey[11]);
                printf("      %x %x %x %x\r\n",tsp_tukey[12],tsp_tukey[13],tsp_tukey[14],tsp_tukey[15]);
                
                ret |= shell_cmd_register("fttspsetvin", ap_shell_setvin, "set foton tsp vin");
                ret |= shell_cmd_register("fttspsetaddr", ap_shell_setaddr, "set foton tsp server address");            
                break;
            }
    }

    return ret;
}

