#ifndef _QL_ADC_H_
#define _QL_ADC_H_

#define ADC0 "mpp4_vadc"
#define ADC1 "mpp6_vadc"

/* battery level */
#define CBC  "vbat_sns"

/*****************************************************
*
*	function: ql_adc_show(char *adc)
*	
*	para	: ADC0  or ADC1 or CBC
*	
*	return	: adc value(mv) when success
*		  -1 when error
*
*****************************************************/
int ql_adc_show(char *adc);

#endif
