#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "i2c.h"
#include "audio.h"
#include "log.h"

static unsigned char audio_dev_addr = TLV320AIC3104_SLAVE_ADDR;

#define delay() do{volatile unsigned int x; for(x=0;x<100;x++);}while(0)

/* i2c read/write function */
static int aud_i2c_fd = -1;
int audio_i2c_write(unsigned char reg, unsigned char *buf, unsigned short len)
{
    return Ql_I2C_Write(aud_i2c_fd, audio_dev_addr, reg, buf, len);
}

int audio_i2c_read(unsigned char reg, unsigned char *buf, unsigned short len)
{
    return Ql_I2C_Read(aud_i2c_fd, audio_dev_addr, reg, buf, len);
}

int register_config(unsigned char reg, unsigned char val)
{
    return audio_i2c_write(reg, &val, 1);
}

static void audio_auto_ident(void)
{
    unsigned char rcvData[1];
    int rt = 0;
    unsigned int i;

    for (i = 0; i < 3; i++)
    {
        log_o(LOG_MID, "check AIC3104 %d times", i + 1);
        audio_dev_addr = TLV320AIC3104_SLAVE_ADDR;
        rcvData[0] = 0xff;
        rt = audio_i2c_read(0x00, rcvData, 1);
        if (rt >= 0)
        {
            log_o(LOG_MID, "audio type AIC3104");
            return;
        }
        delay();
        log_o(LOG_MID, "check LM49352 %d times", i + 1);
        audio_dev_addr = LM49352_SLAVE_ADDR;
        rcvData[0] = 0xff;
        rt = audio_i2c_read(0x00, rcvData, 1);
        if (rt >= 0)
        {
            log_o(LOG_MID, "audio type LM49352");
            return;
        }
        delay();
    }

    log_o(LOG_MID, "audio type unknow, default AIC3104");
    audio_dev_addr = TLV320AIC3104_SLAVE_ADDR;
}


/* audio open */
int audio_open(void)
{
    int ret = 0;
    aud_i2c_fd = open(I2C_DEV, O_RDWR);

    if (aud_i2c_fd < 0)
    {
        log_e(LOG_MID, "Fail to open audio");
        return -1;
    }

    /* Automatic identification of audio chip */
    audio_auto_ident();

    if (audio_dev_addr == LM49352_SLAVE_ADDR)
    {
        ret |= lm49352_basic_setup();
        
        // default
        ret |= lm49352_route_mic_to_4G(false);
        ret |= lm49352_route_4G_to_spkout(false);
        ret |= lm49352_route_mic_bypass(false);
        if (ret < 0)
        {
            log_e(LOG_MID, "Fail to init audio");
            return -1;
        }
        log_o(LOG_MID, "open and init audio OK");
        return 0;
    }
    else
    {
        audio_setup_aic3104();
		//register_config(AUDIO_REG002,0xaa);
		//register_config(AUDIO_REG016,0x50);
		//register_config(AUDIO_REG019,0x40);
		//register_config(AUDIO_REG025,0x86);
		//register_config(AUDIO_REG036,0xd0);
		register_config(AUDIO_REG046,0x80);
		register_config(AUDIO_REG047,0x00);
		
        return 0;
    }     
}


int audio_route_mic_bypass(bool cut_off)
{
    int ret = 0;
    
    if (audio_dev_addr == LM49352_SLAVE_ADDR)
    {
        ret |= lm49352_route_mic_bypass(cut_off);
    }
    else
    {
       ret |= aic3104_route_4G_to_micout(cut_off);
    }

    return (ret < 0) ? -1 : 0;
}
int audio_route_4G_to_ols(bool cut_off)
{
    int ret = 0;
    
    if (audio_dev_addr == LM49352_SLAVE_ADDR)
    {
        ret |= lm49352_route_4G_to_ols(cut_off);
    }
    else
    {
        ret |= aic3104_route_4G_to_ols(cut_off);
    }

    return (ret < 0) ? -1 : 0;
}
int audio_route_4G_to_spkout(bool cut_off)
{
    int ret = 0;
    
    if (audio_dev_addr == LM49352_SLAVE_ADDR)
    {
        ret |= lm49352_route_4G_to_spkout(cut_off);
    }
    else
    {
        ret |= aic3104_route_4G_to_spkout(cut_off);
    }

    return (ret < 0) ? -1 : 0;
}


/* audio close */
int audio_close(void)
{
    int fd = aud_i2c_fd;
    aud_i2c_fd = -1;

    return close(fd);
}

/* audio show all registers */
void audio_show_all_registers(void)
{
    unsigned short i;
    unsigned char val;

    for (i = 0; i <= 0xff; i++)
    {
        val = 0xff;

        if (audio_i2c_read(i, &val, 1) < 0)
        {
            log_e(LOG_MID, "read reg error");
            continue;
        }

        log_o(LOG_MID, "reg=0x%02x(%d) val=0x%02x", i,i, val);
        delay();
    }
}
int audio_basic_ECall(void)
{
     int ret = 0;

     ret |= register_config(0x13, 0x04);
     
     /* 46: PGA_L to HPLOUT Volume Control Register */
     ret |= register_config(0x2E, 0x00);

     /* HPLOUT Output Level Control Register */
     ret |= register_config(0x33, 0x00);

      /* /Register 82: DAC_L1 to LEFT_LOP/M Volume Control Register */
     ret |= register_config(0x52, 0x80);

     /*92: DAC_R1 to RIGHT_LOP/M Volume Control Register */
     ret |= register_config(0x5c, 0x00);
     /* ICALL��Ϊһֱ����*/
     //ret |= register_config(0x5c, 0x80);
     
     ret |= register_config(0x56, 0x9B);
         

     return (ret < 0) ? -1 : 0;
}
int audio_basic_ICall(void)
{
     int ret = 0;

     ret |= register_config(0x13, 0x04);

      /* 46: PGA_L to HPLOUT Volume Control Register */
     ret |= register_config(0x2E, 0x00);

     /* HPLOUT Output Level Control Register */
     ret |= register_config(0x33, 0x00);

      /* /Register 82: DAC_L1 to LEFT_LOP/M Volume Control Register */
     ret |= register_config(0x52, 0x00);

     /*92: DAC_R1 to RIGHT_LOP/M Volume Control Register */
     ret |= register_config(0x5c, 0x80);
         

     return (ret < 0) ? -1 : 0;
}


