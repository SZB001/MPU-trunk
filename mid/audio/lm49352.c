#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "i2c.h"
#include "audio.h"
#include "log.h"

int lm49352_basic_setup(void)
{
    int ret = 0;

    /* basic setup */
    // enable audio functions
    ret |= register_config(BASIC_SETUP_PMC_SETUP, 0xff);
    // use internal clock 300kHZ
    ret |= register_config(BASIC_SETUP_PMC_CLOCKS, 0x01);

    /* ADC and DAC clock setup */
    // clock setup for ADC (osr=128)
    ret |= register_config(ADC_ADC_BASIC, (0x1 << 4) | 0x02);
    ret |= register_config(ADC_ADC_CLOCK, 0);
    // clock setup for DAC (osr=128)
    ret |= register_config(DAC_DAC_BASIC, (0x1 << 4) | 0x01);
    ret |= register_config(DAC_DAC_CLOCK, 0);

    // adc effects
    ret |= register_config(0x70, 0xff);
    // dac effects
    ret |= register_config(0x71, 0xff);

    return (ret < 0) ? -1 : 0;
}


/*
          ---------------->HPL
          |
MONO in(single-ended)----->ADC--------->AUD1 tx

AUD1 rx------------------->DAC--------->AUX out
                            |
                            |---------->HPR
                            |
                            ----------->LS
*/
/* single link */
static int audio_sl_opt_func
(bool cut_off, unsigned char link_reg, unsigned char link_val, unsigned char lvl_reg,
 unsigned char lvl_val)
{
    int ret = 0;
    unsigned char val = 0xff;

    ret |= audio_i2c_read(link_reg, &val, 1);
    val = cut_off ? (val & (~link_val)) : (val | link_val);
    ret |= register_config(link_reg, val);

    ret |= cut_off ? 0 : register_config(lvl_reg, lvl_val);

    return (ret < 0) ? -1 : 0;
}

static int audio_sl_hpl_from_monoin(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             ANALOG_MIXER_HEAD_PHONESL,
                             0x01 << 4,
                             ANALOG_MIXER_MONO_LVL,
                             0x61); // single-ended
}

static int audio_sl_adc_from_monoin(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             ANALOG_MIXER_ADC,
                             0x01 << 5,
                             ANALOG_MIXER_MONO_LVL,
                             0x61); // single-ended
}

static int audio_sl_aud1tx_from_adc(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             DIGITAL_MIXER_OPPORT1,
                             0x01 | (0x01 << 2),
                             AUDIO_PORT1_BASIC,
                             0x01 | (0x01 << 1) | (0x01 << 2) | (0x01 << 5));
}

static int audio_sl_dac_from_aud1rx(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             DIGITAL_MIXER_OPDAC,
                             0x01 | (0x01 << 3),
                             AUDIO_PORT1_BASIC,
                             0x01 | (0x01 << 1) | (0x01 << 2) | (0x01 << 5));
}

static int audio_sl_auxout_from_dac(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             ANALOG_MIXER_AUX_OUT,
                             0x03,
                             ANALOG_MIXER_OUTPUT_OPTIONS,
                             0x0);
}

static int audio_sl_hpr_from_dac(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             ANALOG_MIXER_HEAD_PHONESR,
                             0x03,
                             ANALOG_MIXER_OUTPUT_OPTIONS,
                             0x0);
}

static int audio_sl_ls_from_dac(bool cut_off)
{
    return audio_sl_opt_func(cut_off,
                             ANALOG_MIXER_CLASSD,
                             0x03,
                             ANALOG_MIXER_OUTPUT_OPTIONS,
                             0x0);
}

/* route */
int lm49352_route_mic_bypass(bool cut_off)
{
    int ret = 0;

    /* monoin->hpl */
    ret |= audio_sl_hpl_from_monoin(cut_off);

    return (ret < 0) ? -1 : 0;
}

int lm49352_route_mic_to_4G(bool cut_off)
{
    int ret = 0;

    /* monoin->adc l->audio1 l/r tx */
    ret |= audio_sl_adc_from_monoin(false);
    ret |= audio_sl_aud1tx_from_adc(cut_off);

    return (ret < 0) ? -1 : 0;
}

int lm49352_route_4G_to_spkout(bool cut_off)
{
    int ret = 0;

    /* audio1 l/r rx->dac l/r->auxout */
    ret |= audio_sl_dac_from_aud1rx(false);
    ret |= audio_sl_auxout_from_dac(cut_off);

    return (ret < 0) ? -1 : 0;
}

/*
4G to outside loudspeaker
*/
int lm49352_route_4G_to_ols(bool cut_off)
{
    int ret = 0;

    /* audio1 l/r rx->dac l/r->hpr */
    ret |= audio_sl_dac_from_aud1rx(false);
    ret |= audio_sl_hpr_from_dac(cut_off);

    return (ret < 0) ? -1 : 0;
}

/*
4G to inside loudspeaker
*/
int lm49352_route_4G_to_ils(bool cut_off)
{
    int ret = 0;

    /* audio1 l/r rx->dac l/r->ls */
    ret |= audio_sl_dac_from_aud1rx(false);
    ret |= audio_sl_ls_from_dac(cut_off);

    return (ret < 0) ? -1 : 0;
}


