/*==================================================================================================
 * FILE: MBASE.H
 *==================================================================================================
 * Copyright (c) Hozon and subsidiaries
 *--------------------------------------------------------------------------------------------------
 * DESCRIPTION:
 * Definition of the underlying common functions
 
 * REVISION HISTORY:
 * Author            Date                    Comments
 * ------           ----                    ---------
 * chenlei           Jan /21/2019         Initial creation
 *
 =================================================================================================*/
#ifndef _MBASE_H_
#define _MBASE_H_


/************************************** Functions *************************************************/
char *mbLeftTrimStr( char *pStrSrc );

char *mbRightTrimStr( char * pStrSrc );

char * mbTrimStr( char *pStrSrc );

char *GetStrItemCN(char *dest, char *src, const char *sep, unsigned int index);

unsigned char *str2hex(char *str);

void printf_buff(char *buff, int size);

#endif  // _MBASE_H_





