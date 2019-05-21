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
 * chenlei           Jan /21/2019         Initial creation
 *
 =================================================================================================*/
#ifndef _TBOXSOCK_H_
#define _TBOXSOCK_H_

SSL_CTX   *pctx;

SSL       *pssl;

int       isockfd;

extern char *hurootcertstr;
extern char *humidcertstr;
extern char *huusercertstr;
extern char *huuserkeystr;
extern unsigned int pubPort;
extern char *pubIpAddr;

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
#define CONNINETATON               -2
#define CONNCONNECTSRV             -3
#define CONNPCTXNULL               -4
#define CONNSTLCONNECTFAIL         -5
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



/**************************************  Struct   *************************************************/
struct hz_vehicle_info_st{
    char *unique_id;
    char *tty_type;
    char *carowner_acct;
    char *impower_acct;
};
typedef struct hz_vehicle_info_st CAR_INFO;

#define CIPHERS_LIST "ALL:!eNULL:!ADH:!PSK:!IDEA:!AES256:!AES128:!SRP:!SSLv3:!GMTLSv1.1:+DHE:+ECDSA:+SHA256:+CHACHA20:+CAMELLIA256"

/************************************** public Functions *************************************************/
int HzPortAddrCft(unsigned int iPort, char *sIpaddr);



/************************************** Functions *************************************************/
int HzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath, char *UsrCertPath, char *UsrKeyPath);

int HzTboxInit( );

int HzTboxConnect( );

int HzTboxDataSend(char *reqbuf , int sendlen);

int HzTboxDataRecv(char *rspbuf, int recvl);

int HzTboxClose( );

int HzTboxCloseSession( );

int HzTboxGenCertCsr(char *sn_curvesname, char *ln_curvesname, CAR_INFO *carinfo, char *subject_info, char *filename, char *format);

char *HzTboxGetVinAndCheck(char *g_vin_out);

char *HzTboxSnSimEncInfo(int iCount , const char *sSnstr, const char *sSimstr, char *sfile, int *cipherlen);

unsigned char*HzTboxApplicationData (char *filename , char *siminfo, unsigned char *phexout);


/************************************** single Functions *************************************************/
int SgHzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath);

int SgHzTboxInit( );

int SgHzTboxConnect( );

int SgHzTboxDataSend( char *reqbuf , int sendlen);

int SgHzTboxDataRecv( char *rspbuf, int recvl);

int SgHzTboxCloseSession( );

int SgHzTboxClose( );



#endif  // _INC_H__





