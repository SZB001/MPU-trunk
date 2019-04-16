#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <stdbool.h>

#define I2C_DEV                 "/dev/i2c-2"

/*****************LM49352_START***********************/
#define LM49352_SLAVE_ADDR      0x1A // with pins SA0=0, SA1=0*/

#define BASIC_SETUP_PMC_SETUP   0x00
#define BASIC_SETUP_PMC_CLOCKS  0x01

#define ANALOG_MIXER_CLASSD         0x10
#define ANALOG_MIXER_HEAD_PHONESL   0x11
#define ANALOG_MIXER_HEAD_PHONESR   0x12
#define ANALOG_MIXER_AUX_OUT        0x13
#define ANALOG_MIXER_OUTPUT_OPTIONS 0x14
#define ANALOG_MIXER_ADC            0x15
#define ANALOG_MIXER_MIC_LVL        0x16
#define ANALOG_MIXER_AUX_LVL        0x18
#define ANALOG_MIXER_MONO_LVL       0x19


#define ADC_ADC_BASIC           0x20
#define ADC_ADC_CLOCK           0x21

#define DAC_DAC_BASIC           0x30
#define DAC_DAC_CLOCK           0x31

#define DIGITAL_MIXER_IPLVL1    0x40
#define DIGITAL_MIXER_IPLVL2    0x41
#define DIGITAL_MIXER_OPPORT1   0x42
#define DIGITAL_MIXER_OPDAC     0x44

#define AUDIO_PORT1_BASIC       0x50


#define AUDIO_NULL_REG          0xFF

/*****************LM49352_END***********************/


/****************TLV320AIC3104_START***********************/
#define  TLV320AIC3104_SLAVE_ADDR  0x18 
/* reg x */
enum _audio_reg
{
    AUDIO_REG000,
    AUDIO_REG001,
    AUDIO_REG002,
    AUDIO_REG003,
    AUDIO_REG004,
    AUDIO_REG005,
    AUDIO_REG006,
    AUDIO_REG007,
    AUDIO_REG008,
    AUDIO_REG009,
    AUDIO_REG010,
    AUDIO_REG011,
    AUDIO_REG012,
    AUDIO_REG013,
    AUDIO_REG014,
    AUDIO_REG015,
    AUDIO_REG016,
    AUDIO_REG017,
    AUDIO_REG018,
    AUDIO_REG019,
    AUDIO_REG020,
    AUDIO_REG021,
    AUDIO_REG022,
    AUDIO_REG023,
    AUDIO_REG024,
    AUDIO_REG025,
    AUDIO_REG026,
    AUDIO_REG027,
    AUDIO_REG028,
    AUDIO_REG029,
    AUDIO_REG030,
    AUDIO_REG031,
    AUDIO_REG032,
    AUDIO_REG033,
    AUDIO_REG034,
    AUDIO_REG035,
    AUDIO_REG036,
    AUDIO_REG037,
    AUDIO_REG038,
    AUDIO_REG039,
    AUDIO_REG040,
    AUDIO_REG041,
    AUDIO_REG042,
    AUDIO_REG043,
    AUDIO_REG044,
    AUDIO_REG045,
    AUDIO_REG046,
    AUDIO_REG047,
    AUDIO_REG048,
    AUDIO_REG049,
    AUDIO_REG050,
    AUDIO_REG051,
    AUDIO_REG052,
    AUDIO_REG053,
    AUDIO_REG054,
    AUDIO_REG055,
    AUDIO_REG056,
    AUDIO_REG057,
    AUDIO_REG058,
    AUDIO_REG059,
    AUDIO_REG060,
    AUDIO_REG061,
    AUDIO_REG062,
    AUDIO_REG063,
    AUDIO_REG064,
    AUDIO_REG065,
    AUDIO_REG066,
    AUDIO_REG067,
    AUDIO_REG068,
    AUDIO_REG069,
    AUDIO_REG070,
    AUDIO_REG071,
    AUDIO_REG072,
    AUDIO_REG073,
    AUDIO_REG074,
    AUDIO_REG075,
    AUDIO_REG076,
    AUDIO_REG077,
    AUDIO_REG078,
    AUDIO_REG079,
    AUDIO_REG080,
    AUDIO_REG081,
    AUDIO_REG082,
    AUDIO_REG083,
    AUDIO_REG084,
    AUDIO_REG085,
    AUDIO_REG086,
    AUDIO_REG087,
    AUDIO_REG088,
    AUDIO_REG089,
    AUDIO_REG090,
    AUDIO_REG091,
    AUDIO_REG092,
    AUDIO_REG093,
    AUDIO_REG094,
    AUDIO_REG095,
    AUDIO_REG096,
    AUDIO_REG097,
    AUDIO_REG098,
    AUDIO_REG099,
    AUDIO_REG100,
    AUDIO_REG101,
    AUDIO_REG102,
    AUDIO_REG103,
    AUDIO_REG104,
    AUDIO_REG105,
    AUDIO_REG106,
    AUDIO_REG107,
    AUDIO_REG108,
    AUDIO_REG109,
    AUDIO_REG110,
    AUDIO_REG111,
    AUDIO_REG112,
    AUDIO_REG113,
    AUDIO_REG114,
    AUDIO_REG115,
    AUDIO_REG116,
    AUDIO_REG117,
    AUDIO_REG118,
    AUDIO_REG119,
    AUDIO_REG120,
    AUDIO_REG121,
    AUDIO_REG122,
    AUDIO_REG123,
    AUDIO_REG124,
    AUDIO_REG125,
    AUDIO_REG126,
    AUDIO_REG127,
};



/****************TLV320AIC3104_END***********************/

int audio_i2c_write(unsigned char reg, unsigned char *buf, unsigned short len);
int audio_i2c_read(unsigned char reg, unsigned char *buf, unsigned short len);
int register_config(unsigned char reg, unsigned char val);
int audio_open(void);
int audio_close(void);
void audio_show_all_registers(void);
int audio_route_mic_bypass(bool cut_off);
int audio_route_4G_to_ols(bool cut_off);
int audio_route_4G_to_spkout(bool cut_off);


/*LM49352*/
int lm49352_basic_setup(void);
int lm49352_route_mic_bypass(bool cut_off);
int lm49352_route_mic_to_4G(bool cut_off);
int lm49352_route_4G_to_spkout(bool cut_off);
int lm49352_route_4G_to_ols(bool cut_off);
int lm49352_route_4G_to_ils(bool cut_off);


/*TLV320AIC3104*/
void audio_setup_aic3104(void);
int aic3104_route_4G_to_micout(bool cut_off);
int aic3104_route_4G_to_spkout(bool cut_off);
int aic3104_route_4G_to_ols(bool cut_off);


#endif
