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
#define HOZON_TBOX_VERSION_TEXT    "TBOX SDK 1.1.23 - 2020-05-29"


extern char *hurootcertstr;
extern char *humidcertstr;
extern char *huusercertstr;
extern char *huuserkeystr;
extern unsigned int  pubPort;
extern char         *pubIpAddr;

extern char *approotcertstr;
extern char *appmidcertstr;
extern char *appusercertstr;
extern char *appuserkeystr;

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


/**************************************  Struct   *************************************************/
struct hz_vehicle_info_st{
    char *unique_id;
    char *tty_type;
    char *carowner_acct;
    char *impower_acct;
};
typedef struct hz_vehicle_info_st CAR_INFO;

#define CIPHERS_LIST "ALL:!eNULL:!ADH:!PSK:!IDEA:!AES256:!AES128:!SRP:!SSLv3:!GMTLSv1.1:+DHE:+ECDSA:+SHA256:+CHACHA20:+CAMELLIA256"

struct hzBT_vehicle_info_st{
    char *unique_id;
    char *tty_type;
    char *carowner_acct;
    char *impower_acct;
};
typedef struct hzBT_vehicle_info_st CARBT_INFO;

#define CIPHERSBT_LIST "ALL:!eNULL:!ADH:!PSK:!IDEA:!AES256:!AES128:!SRP:!SSLv3:!GMTLSv1.1:+DHE:+ECDSA:+SHA256:+CHACHA20:+CAMELLIA256"


/************************************** public Functions *************************************************/
int HzPortAddrCft(unsigned int iPort, int mode, char *sIpaddr, char *sHostnameaddr);


/************************************** single Functions *************************************************/
int SgHzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath);

int SgHzTboxInit(char *uc_crl);

int SgHzTboxSocketFd(int *p_sockfd);

int SgHzTboxConnect(int conn_sockfd);

int SgHzTboxDataSend(char *reqbuf , int sendlen);

int SgHzTboxDataRecv(char *rspbuf, int recvl, int *recvlen);

int SgHzTboxCloseSession();

int SgHzTboxClose();


/************************************** Functions *************************************************/
int HzTboxCertchainCfg(char *RtcertPath, char *ScdCertPath, char *UsrCertPath, char *UsrKeyPath);

int HzTboxInit(char *uc_crl);

int HzTboxSocketFd(int *p_sockfd);

//int HzTboxConnect();
int HzTboxConnect(int conn_sockfd);

int HzTboxDataSend(char *reqbuf , int sendlen);

int HzTboxDataRecv(char *rspbuf, int recvl, int *recvlen);

int HzTboxClose();

int HzTboxCloseSession();


/************************************** cert Functions *************************************************/
int HzTboxGenCertCsr(char *sn_curvesname, char *ln_curvesname, CAR_INFO *carinfo, char *subject_info, char *pathname, char *filename, char *format);

int HzTboxSnSimEncInfo(const char *sSnstr, const char *sSimstr, char *sfile, char *ofile, int *cipherlen);

int HzTboxApplicationData(char *filename , char *simfile, unsigned char *phexout, int *len);

int HzTboxSnSimDecInfo(char *infile, char *kfile, char *sSnstr, char *sSimstr);


/************************************** tbox server Functions *************************************/
int HzTboxSrvInit(char *uc_crl);

int HzTboxSvrAccept();

int HzTboxSvrDataSend(char *reqbuf , int sendlen);

int HzTboxSvrDataRecv(char *rspbuf, int recvl, int *recvlen);

int HzTboxSvrClose();

int HzTboxSvrCloseSession();

int HzTboxSrvCloseCtrlState();

int HzTboxSrvListenCtrlState();


/************************************** tbox ks *************************************************/
int HzTboxKsGenCertCsr(char *contPath, char *cfgPath, char *vin, char *sLable, char *p10file);

//int HzTboxMatchCertVerify (char *ucertfile)
int HzTboxMatchCertVerify (char *ucertfile, char *ucertkey);

int HzTboxOtaSignCertVerify (char *ucertfile); 


/********************************* tbox cert update *********************************************/
/*2nd develop --author: chenL  --dayte: 20190611 */
int HzTboxCertUpdCheck(char *ucfile, char *optformat, long int sysdate, int *curstatus);

int HzTboxGetserialNumber(char *ucfile, char *informat, char *serialNum);

int HzTboxUcRevokeStatus(char *crlfile, char *seri, int *crlstatus);

int HzTboxdoSignVeri(char *certfile, const unsigned char *indata,  char *signinfo, int *signinfolen);

int HzTboxdoSign(const unsigned char *indata, const char *keyfile, char *retsignval, int *retsignlen);

int HzversionMain(char *vs);

int HzTcpLinkStateMult(char *clientIp, char *stat_value, int *iCount, char *Tcpstatus);

int HzTcpLinkStateSg(int tbox_sock_fd, int *Tcpstatus);


/********************************* tbox cert transcode *********************************************/
/* --author: chenL  --dayte: 20190715 */
/*base64 encode*/
int hz_base64_encode(unsigned char *dst, int *dlen, const unsigned char *src, int  slen);
/*base64 decode*/
int hz_base64_decode(unsigned char *dst, int *dlen, const unsigned char *src, int  slen);

/**************************************** tbox info set *********************************************/
int HzTboxSetVin(char *in_vin);
//int HzTboxSetOwner(char *in_owner);
//int HzTboxSetAuth(char *in_auth);


//test
//int HzTboxAesDecInfo( char *iFile, char *keynum);

/**/
/************************************** Functions *************************************************/
int HzBtBackup(char *blueaddre, char *bluename, char *ccid, char *path);

int HzBtVerifyBackup(char *blueaddre, char *bluename, char *ccid, char *path);

int HzBtCertcfg(char *RtcertPath, char *ScdCertPath);

int HzBtSymEncrypt(char *raw_buf, int len, char *result_buf, int *result_len, char *userkey, int type);

int HzRequestInfo(char *cert, char *crlfile, char *Vin, char *UserID,  char *AuthorID, char *CodeInfo,  char *BackInfo, int *info_len, char *sekey,int *se_len);

int HzBtGenSymKey(char *cert, char *chiperkey, char *plainkey,int *ch_len, int *p_len);



int HZBase64Encode(unsigned char *dst, int *dlen, const unsigned char *src, int  slen);

int HZBase64Decode(unsigned char *dst, int *dlen, const unsigned char *src, int  slen);

int showversion(char *version);


#endif  // _INC_H__





