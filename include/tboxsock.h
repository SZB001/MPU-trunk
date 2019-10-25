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
#ifndef _INC_H_
#define _INC_H_

#define HOZON_TBOX_VERSION         0
#define HOZON_TBOX_VERSION_TEXT    "TBOX SDK 1.1.8 - 2019-09-23"


extern char *hurootcertstr;
extern char *humidcertstr;
extern char *huusercertstr;
extern char *huuserkeystr;
extern unsigned int  pubPort;
extern char         *pubIpAddr;



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
int HzPortAddrCft(unsigned int iPort, int mode, char *sIpaddr, char *sHostnameaddr);


/************************************** single Functions *************************************************/
int SgHzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath);

int SgHzTboxInit(char *uc_crl);

int SgHzTboxConnect( );

int SgHzTboxDataSend( char *reqbuf , int sendlen );

int SgHzTboxDataRecv( char *rspbuf, int recvl, int *recvlen );

int SgHzTboxCloseSession( );

int SgHzTboxClose( );


/************************************** Functions *************************************************/
int HzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath, char *UsrCertPath, char *UsrKeyPath);

int HzTboxInit(char *uc_crl);

int HzTboxConnect( );

int HzTboxDataSend(char *reqbuf , int sendlen);

int HzTboxDataRecv(char *rspbuf, int recvl, int *recvlen);

int HzTboxClose( );

int HzTboxCloseSession( );


/************************************** cert Functions *************************************************/
int HzTboxGenCertCsr(char *sn_curvesname, char *ln_curvesname, CAR_INFO *carinfo, char *subject_info, char *pathname, char *filename, char *format);

int HzTboxSnSimEncInfo(const char *sSnstr, const char *sSimstr, char *sfile, char *ofile, int *cipherlen);

int HzTboxApplicationData (char *filename , char *simfile, unsigned char *phexout, int *len);


/************************************** tbox server Functions *************************************/
int HzTboxSrvInit(char *uc_crl);

int HzTboxSvrAccept ( );

int HzTboxSvrDataSend ( char *reqbuf , int sendlen );

int HzTboxSvrDataRecv( char *rspbuf, int recvl, int *recvlen );

int HzTboxSvrClose ( );

int HzTboxSvrCloseSession ( );


/************************************** tbox ks *************************************************/
int HzTboxKsGenCertCsr(char *contPath, char *cfgPath, char *vin, char *sLable, char *p10file);

int HzTboxMatchCertVerify (char *ucertfile);


/********************************* tbox cert update *********************************************/
/*2nd develop --author: chenL  --dayte: 20190611 */
int HzTboxCertUpdCheck(char *ucfile, char *optformat, long int sysdate, int *curstatus);

int HzTboxGetserialNumber(char *ucfile, char *informat, char *serialNum);

int HzTboxUcRevokeStatus(char *crlfile, char *seri, int *crlstatus);

int HzTboxdoSignVeri(char *certfile, const unsigned char *indata,  char *signinfo, int *signinfolen);

int HzTboxdoSign(const unsigned char *indata, const char *keyfile, char *retsignval, int *retsignlen);

int HzversionMain(char *vs);



/********************************* tbox cert transcode *********************************************/
/* --author: chenL  --dayte: 20190715 */
/*base64 encode*/
int hz_base64_encode( unsigned char *dst, int *dlen, const unsigned char *src, int  slen );
/*base64 decode*/
int hz_base64_decode( unsigned char *dst, int *dlen, const unsigned char *src, int  slen );

/**************************************** tbox info set *********************************************/
int HzTboxSetVin(char *in_vin);
//int HzTboxSetOwner(char *in_owner);
//int HzTboxSetAuth(char *in_auth);


//test
//int HzTboxAesDecInfo( char *iFile, char *keynum);

#endif  // _INC_H__





