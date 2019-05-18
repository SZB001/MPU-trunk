/*==================================================================================================
 * FILE: INC.H
 *==================================================================================================
 * Copyright (c) Hozon and subsidiaries
 *--------------------------------------------------------------------------------------------------
 * DESCRIPTION:
 * This function is used to link servers, Common is documents 
 
 * REVISION HISTORY:
 * Author            Date                    Comments
 * ------           ----                    ---------
 * chenL           Jan /21/2019         Initial creation
 *
 =================================================================================================*/
#ifndef _INC_H_
#define _INC_H_

SSL_CTX   *pctx;

SSL       *pssl;

int       isockfd;

extern char *hurootcertstr;
extern char *humidcertstr;
extern char *huusercertstr;
extern char *huuserkeystr;


#define CIPHERS_LIST "ALL:!eNULL:!ADH:!PSK:!IDEA:!AES256:!AES128:!SRP:!SSLv3:!GMTLSv1.1:+DHE:+ECDSA:+SHA256:+CHACHA20:+CAMELLIA256"

/********************************* define return value *********************************************/
#define CERTSETCFGOK                0
#define CERTSETCFGFAIL             -1
/*-------------------------------------------------------------------------------------------------*/
#define COMINITCTXNEW              -1
#define COMINITRTCERT              -2
#define COMINITSCDCERT             -3
#define COMINITUSERCERT            -4
#define COMINITUSERPRIKEY          -5
#define PRIKEYCERTMATCH            -6
#define COMINITNORMAL               0
/*-------------------------------------------------------------------------------------------------*/
#define CONNSOCKFAIL               -1
#define CONNBINDFAIL               -2
#define CONNLISTENFAIL             -3
#define CONNPCTXNULL               -4
#define CONNACPTFAIL               -5
#define CONNCONNECTNORMAL           0
/*-------------------------------------------------------------------------------------------------*/
#define SSLISNULL                  -1
#define SSLWRITEFAIL               -2
#define SENDDATAOK                  0
/*-------------------------------------------------------------------------------------------------*/
#define PSSLISNULL                 -1
#define SSLREADFAIL                -2
#define RECVDATAOK                  0
/*-------------------------------------------------------------------------------------------------*/
#define CLOSEOK                     0
#define CLOSEFAIL                  -1



/************************************** Functions *************************************************/
int Tsp_Test_CertchainCfg(char *RtcertPath, char *ScdCertPath, char *UsrCertPath, char *UsrKeyPath);

int Tsp_Test_Init( );

int Tsp_Test_Connect( );

int Tsp_Test_DataSend( char *reqbuf , int sendlen);

int Tsp_Test_DataRecv( char *rspbuf, int recvl);

int Tsp_Test_CloseSession( );

int Tsp_Test_Close( );

int Tsp_Test_Base64_Encode(char *in_str, int in_len, char *out_str); //编码

int Tsp_Test_Base64_Decode(char *in_str, int in_len, char *out_str); //解码




/************************************** Functions *************************************************/
int SgHzTspCertchainCfg(char *RtcertPath, char *ScdCertPath, char *UsrCertPath, char *UsrKeyPath);

int SgHzTspInit( );

int SgHzTspConnect( );

int SgHzTspDataSend( char *reqbuf , int sendlen);

int SgHzTspDataRecv( char *rspbuf, int recvl);

int SgHzTspCloseSession( );

int SgHzTspClose( );



#endif  // _INC_H__
