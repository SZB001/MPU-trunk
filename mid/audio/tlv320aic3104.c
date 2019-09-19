#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "i2c.h"
#include "audio.h"
#include "log.h"

void audio_setup_aic3104(void)
{
	#if 0
    /*  Page Select Register */
    register_config(AUDIO_REG000, 0x00);
    /* Codec Sample Rate Select Register */
    register_config(AUDIO_REG002, 0x55);
    /* PLL enable,Q value,P value */
    register_config(AUDIO_REG003, 0x91);
    /* PLL J value */
    register_config(AUDIO_REG004, 0xc0);
    /* Codec Data-Path,f S(ref) = 48 kHz, Left-Right-DAC data path mix of left- and right-channel */
    register_config(AUDIO_REG007, 0x1e);
    /* Audio Serial Data Interface Control Register B */
    register_config(AUDIO_REG009, 0x40);//ftc you
    /*  Audio Serial Data Interface Control Register C */
    register_config(AUDIO_REG010, 0x01);
    /* Audio Codec Overflow Flag Register,PLL R Value */
    register_config(AUDIO_REG011, 0x01);//fct81,bu ying xiang
    /* Audio Codec Digital Filter Control Register */
    register_config(AUDIO_REG012, 0x5f);
    /* Headset/Button Press Detection Register B */
    //register_config(AUDIO_REG014, 0x40);//fct wu
    /* Left-ADC PGA Gain Control Register */
    register_config(AUDIO_REG015, 0x00);
    /* Right-ADC PGA Gain Control Register */
    //register_config(0x10, 0x50);
    /* MIC1LP/LINE1LP to Left-ADC Control Register */
    register_config(AUDIO_REG019, 0x04);//fct04
     //register_config(AUDIO_REG019, 0x84);//fct04
    /* MIC1RP/LINE1RP to Left-ADC Control Register */
    //register_config(0x15, 0x0);

    /* MICBIAS Level Control */
    register_config(AUDIO_REG025, 0x40);//fct86

    /* Left-AGC Gain Register */
    register_config(AUDIO_REG032, 0x00);
    /* Right-AGC Gain Register */
    //register_config(0x21, 0x00);
    /* DAC Power and Output Driver Control Register */
    register_config(AUDIO_REG037, 0xc0);
    /* Left-DAC Digital Volume Control Register */
    register_config(AUDIO_REG043, 0x00);
    /* Right-DAC Digital Volume Control Register */
    register_config(AUDIO_REG044, 0x00);
    /* PGA_L to HPLOUT Volume Control Register */
    //register_config(AUDIO_REG046, 0x80);//direct
    /* DAC_L1 to HPLOUT Volume Control Register */
    register_config(AUDIO_REG047, 0x80);   
    /* HPLOUT Output Level Control Register */
    register_config(AUDIO_REG051, 0x9f);
    /* DAC_L1 to HPLCOM Volume Control Register */
    register_config(AUDIO_REG054, 0x80);//fct wu
    /* HPLCOM Output Level Control Register */
    register_config(AUDIO_REG058, 0x07);
    /* DAC_R1 to HPROUT Volume Control Register */
    //register_config(0x40, 0x80);
    /* HPROUT Output Level Control Register */
    //register_config(0x41, 0x0f);
    /* HPRCOM Output Level Control Register */
    //register_config(0x48, 0x07);

    /* DAC_R1 to LEFT_LOP/M Volume Control Register */
    register_config(AUDIO_REG085, 0x80);//fct wu
    /* LEFT_LOP/M Output Level Control Register */
    register_config(AUDIO_REG086, 0x0b);

    register_config(AUDIO_REG090, 0x80);
    /* DAC_R1 to RIGHT_LOP/M Volume Control Register */
    register_config(AUDIO_REG092, 0x80);
    //register_config(AUDIO_REG092, 0x8F);
    /* RIGHT_LOP/M Output Level Control Register */
    register_config(AUDIO_REG093, 0x0b);//fct 9b
    /*  Output Driver Short-Circuit Detection Status Register */
    //register_config(0x5f, 0x0c);
    /* Clock Generation Control Register,MCLK or BCLK */
    register_config(AUDIO_REG102, 0xa2);
    //register_config(AUDIO_REG102, 0x02);//fct a2
    register_config(AUDIO_REG108, 0x70);
	#endif
	int ret = 0;

    //ret |= register_config(0x1, 0x80);
    /* Codec Sample Rate Select Register */
    ret |= register_config(0x2, 0xaa);

    /* PLL Programming Register A */
    ret |= register_config(0x3, 0x91);

    /* PLL Programming Register B */
    ret |= register_config(0x4, 0xc0);

    /* Codec Data-Path Setup Register */
    ret |= register_config(0x7, 0x1e);

    /*Audio Serial Data Interface Control Register B*/
    ret |= register_config(0x9, 0x40);

    /*Audio Serial Data Interface Control Register C*/
    ret |= register_config(0xa, 0x01);

    /*Audio Codec Overflow Flag Register*/
    ret |= register_config(0xb, 0x81);

    /*Audio Codec Digital Filter Control Register*/
    ret |= register_config(0xc, 0x5f);

    /*Page 0/Register 16: Right-ADC PGA Gain Control Register*/
    ret |= register_config(0x10, 0x50);

    /* MIC1LP/LINE1LP to Left-ADC Control Register */
    ret |= register_config(0x13, 0x04);

    // ret |= register_config(0x15, 0x0);
    /* MICBIAS Control Register */
    ret |= register_config(0x19, 0x86);

    /*  Left-AGC Gain Register */
    ret |= register_config(0x20, 0x00);

    /* Right-AGC Gain Register */
    ret |= register_config(0x21, 0x00);

    /* DAC Power and Output Driver Control Register */
    ret |= register_config(0x25, 0xc0);

    /* Left-DAC Digital Volume Control Register */
    ret |= register_config(0x2b, 0x00);

    /* Right-DAC Digital Volume Control Register */
    ret |= register_config(0x2c, 0x00);

    /* Right-DAC Digital Volume Control Register (continued) */
    ret |= register_config(0x2f, 0x00);

    /* HPLOUT Output Level Control Register */
    //ret |= register_config(0x33, 0x00);
	ret |= register_config(0x33, 0x5b);

    /* 58: HPLCOM Output Level Control Register*/
    ret |= register_config(0x3a, 0x07);

    /* 64: DAC_R1 to HPROUT Volume Control Register */
    //ret |= register_config(0x40, 0x80);

    /* 65: HPROUT Output Level Control Register*/
    ret |= register_config(0x41, 0x0f);

    /* 72: HPRCOM Output Level Control Register */
    //ret |= register_config(0x48, 0x07);

    /*92: DAC_R1 to RIGHT_LOP/M Volume Control Register */
    /* ICALL��Ϊһֱ����*/
    //ret |= register_config(0x5c, 0x80);
    ret |= register_config(0x5c, 0x00);

    /* 95: Output Driver Short-Circuit Detection Status Register */
    ret |= register_config(0x5f, 0x0c);

    /*/Register 102: Clock Generation Control Register*/
    ret |= register_config(0x66, 0xa2);

     /* MIC1LP/LINE1LP to Left-ADC*/
    ret |= register_config(0x13, 0x04);
     
    /*r 89: DAC_L1 to RIGHT_LOP/M Volume Control Register*/
    ret |= register_config(0x59, 0x00); 

    /* 93: RIGHT_LOP/M Output Level Control Register (continued)*/
    ret |= register_config(0x5d, 0x5B); 

    /* add */
    /*r 15: Left-ADC PGA Gain Control Register*/
    ret |= register_config(0xf, 0x00); 
    /* r 22: MIC1RP/LINE1RP to Right-ADC Control Register */
    ret |= register_config(0x16, 0x78);
    
    /* Register 23: Reserved Registe*/
    // ret |= register_config(0x17, 0x78);
    /*/Register 24: MIC1LP/LINE1LP to Right-ADC Control Register*/
    ret |= register_config(0x18, 0x78); 

    /* /Register 82: DAC_L1 to LEFT_LOP/M Volume Control Register */
    //ret |= register_config(0x52, 0x80);
    ret |= register_config(0x52, 0x00);

    /*Register 86: LEFT_LOP/M Output Level Control Register*/
    ret |= register_config(0x56, 0x91);


     //ret |= register_config(0xf, 0x50);
     //ret |= register_config(0x7, 0x80);
     //ret |= register_config(0x13, 0x40);
	 ret |= register_config(0x13, 0x00);

     /* HPLOUT */
     ret |= register_config(0x2E, 0x80);

     //ret |= register_config(0x33, 0x19);
	  ret |= register_config(0x33, 0x9b);
     //return (ret < 0) ? -1 : 0;
     
}


/* route */
int aic3104_route_4G_to_micout(bool cut_off)
{
    int ret = 0;
    
    if(cut_off)
    {
        ret |= register_config(AUDIO_REG047, 0x0); 
        //ret |= register_config(AUDIO_REG051, 0x0);  
    }
    else
    {
        ret |= register_config(AUDIO_REG047, 0x80);  
        //ret |= register_config(AUDIO_REG051, 0x19);  
    }

    return (ret < 0) ? -1 : 0;
}


int aic3104_route_4G_to_spkout(bool cut_off)
{
    int ret = 0;
    
    if(cut_off)
    {
        ret |= register_config(AUDIO_REG092, 0x0); 
        //ret |= register_config(AUDIO_REG093, 0x0);  
    }
    else
    {
        ret |= register_config(AUDIO_REG092, 0x80);  
        //ret |= register_config(AUDIO_REG093, 0x19); 
    }

    return (ret < 0) ? -1 : 0;
}

/*
4G to outside loudspeaker
*/
int aic3104_route_4G_to_ols(bool cut_off)
{
    int ret = 0;
    
    if(cut_off)
    {
        ret |= register_config(AUDIO_REG085, 0x0);  
        //ret |= register_config(AUDIO_REG086, 0x0);  
    }
    else
    {
        ret |= register_config(AUDIO_REG085, 0x80);  
        //ret |= register_config(AUDIO_REG086, 0x19); 
    }

    return (ret < 0) ? -1 : 0;
}





