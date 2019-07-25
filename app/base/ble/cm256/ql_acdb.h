#ifndef __QL_ACDB_H__
#define __QL_ACDB_H__

//------------------------------------------------------------------------------
/*
* Function:     ql_codec_gain_write
* 
* Description:
*               set the codec gain module in audio playback 
*
* Parameters:
*               the codec gain value. 
* Return:        
*               set TRUE or FALSE. 
*/
//------------------------------------------------------------------------------
int ql_codec_gain_write(unsigned short gain);

//------------------------------------------------------------------------------
/*
* Function:     ql_codec_gain_read
* 
* Description:
*               read the codec gain module in audio playback 
*
* Parameters:
*               none. 
* Return:        
*               the codec gain. 
*/
//------------------------------------------------------------------------------
short ql_codec_gain_read(void);


#endif  //__QL_ACDB_H__
