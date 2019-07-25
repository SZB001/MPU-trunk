/*==================================================================================================
 * FILE: btsock.H
 *==================================================================================================
 * Copyright (c) Hozon and subsidiaries
 *--------------------------------------------------------------------------------------------------
 * DESCRIPTION:
 * This function is used to link servers, Common is documents 
 
 * REVISION HISTORY:
 * Author            Date                    Comments
 * ------           ----                    ---------
 * zhangxinyan           May/09/2019         Initial creation
 *
 =================================================================================================*/
#ifndef _BTSOCK_H_
#define _BTSOCK_H_


extern char *approotcertstr;
extern char *appmidcertstr;
extern char *appusercertstr;
extern char *appuserkeystr;
extern unsigned int pubPort;
extern char *pubIpAddr;

/********************************* define return value *********************************************/
#define CERTSETCFGFAIL             0
#define CERTSETCFGOK               1
/*-------------------------------------------------------------------------------------------------*/
#define BACKUPFAIL                  0
#define BACKUPOK                    1
#define SYMENCRYPTFAIL              0
#define SYMENCRYPTOK                1
#define FILENOTOPEN                -1
#define CERTVERIFYFAIL              0
#define CERTVERIFYOK                1
#define ROOTCERTLOADFAIL           -1
#define CONVX509FAIL               -2     
#define ROOTSTROETRUST             -3
#define INITSCDCERT                -4
#define CONVSECONCERTFAIL          -5
#define SECONSTROETRUST            -6
#define VERIFYCERTFAIL             -7
#define KEYGENFAIL                 -8

/********************************* BASE64 *********************************************/
               
#define HZBASE64 1 
//#define HZBASE64 0


/**************************************  Struct   *************************************************/
struct hz_vehicle_info_st{
    char *unique_id;
    char *tty_type;
    char *carowner_acct;
    char *impower_acct;
};
typedef struct hz_vehicle_info_st CAR_INFO;

#define CIPHERS_LIST "ALL:!eNULL:!ADH:!PSK:!IDEA:!AES256:!AES128:!SRP:!SSLv3:!GMTLSv1.1:+DHE:+ECDSA:+SHA256:+CHACHA20:+CAMELLIA256"


/************************************** Functions *************************************************/
int HzBtBackup(char *blueaddre, char *bluename, char *ccid, char *path);

int HzBtVerifyBackup(char *blueaddre, char *bluename, char *ccid, char *path);

int HzBtCertcfg(char *RtcertPath, char *ScdCertPath);

int HzBtSymEncrypt(char *raw_buf, int len, char *result_buf, int *result_len, char *userkey, int type);

int HzRequestInfo (char *cert, char *Vin, char *UserID, char *AuthorID, char *CodeInfo,  char *BackInfo, int *info_len, char *sekey,int *se_len);

int HzBtGenSymKey(char *cert, char *chiperkey, char *plainkey,int *ch_len, int *p_len);

int HzBtCertRevoke(char *cert,char *informat, char *crlfile);

int HZBase64Encode( unsigned char *dst, int *dlen, const unsigned char *src, int  slen );

int HZBase64Decode( unsigned char *dst, int *dlen, const unsigned char *src, int  slen );


#endif  // _BTSOCK_H__





