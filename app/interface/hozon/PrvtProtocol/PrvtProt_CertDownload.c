/******************************************************
�ļ�����	PrvtProt_CertDownload.c

������	��ҵ˽��Э�飨�㽭���ڣ�	
Data			Vasion			author
2018/1/10		V1.0			liujian
*******************************************************/

/*******************************************************
description�� include the header file
*******************************************************/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include  <errno.h>
#include <sys/times.h>
#include <sys/time.h>
#include "timer.h"
#include <sys/prctl.h>
#include "dir.h"
#include <sys/types.h>
#include <sysexits.h>	/* for EX_* exit codes */
#include <assert.h>	/* for assert(3) */
#include "constr_TYPE.h"
#include "asn_codecs.h"
#include "asn_application.h"
#include "asn_internal.h"	/* for _ASN_DEFAULT_STACK_MAX */
#include "Bodyinfo.h"
#include "per_encoder.h"
#include "per_decoder.h"
#include "file.h"
#include "init.h"
#include "log.h"
#include "list.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
#include "cfg_api.h"
#include "hozon_SP_api.h"
#include "hozon_PP_api.h"
#include "shell_api.h"
#include "gb32960_api.h"
#include "PrvtProt_shell.h"
#include "PrvtProt_queue.h"
#include "PrvtProt_cfg.h"
#include "PrvtProt.h"
#include "tboxsock.h"
#include "PrvtProt_remoteConfig.h"
#include "PrvtProt_CertDownload.h"

/*******************************************************
description�� global variable definitions
*******************************************************/

/*******************************************************
description�� static variable definitions
*******************************************************/
typedef struct
{
	PrvtProt_pack_Header_t	Header;
}__attribute__((packed))  PP_CertDL_pack_t; /**/

typedef struct
{
	PP_CertDL_pack_t 			pack;
	PP_CertDownloadSt_t	 		state;
	PP_CertificateDownload_t	para;
	uint16_t 			Cnt;
	uint8_t				BDLink;
}__attribute__((packed))  PP_CertDL_t;

typedef struct
{
	PP_CertificateEnReq_t		para;
	uint8_t enSt;
	uint8_t	CertEnCnt;
}PP_CertificateEn_t;

typedef struct
{
	PP_CertRevoListReq_t		para;
	uint8_t	checkRevoFlag;
	uint8_t RLSt;
}PP_CertRevoList_t;

typedef struct
{
	uint8_t	checkStFlag;
	uint8_t CertUpdataReq;
	uint8_t CertUpdataSt;
	uint8_t updataSt;
}PP_checkCertSt_t;

typedef struct
{
	PP_CertUpdataReq_t			para;
	uint8_t	allowupdata;
	uint8_t	updatedFlag;
	uint8_t	Cnt;
}PP_CertUpdata_t;

static PP_CertDL_t 				PP_CertDL;
static PP_CertDownloadPara_t	PP_CertDownloadPara;//需要掉电保存的参数
static char	PP_CertDL_SN[19];
static char	PP_CertDL_ICCID[21];
static PP_CertificateEn_t		PP_CertEn;
static PP_CertRevoList_t		PP_CertRevoList;
static PP_checkCertSt_t			PP_checkCertSt;
static PP_CertUpdata_t			PP_CertUpdata;
static PrvtProt_TxInform_t CertDL_TxInform[PP_CERT_DL_TXINFORMNODE];

#define ERR_BASE64_BUFFER_TOO_SMALL               0x0010
#define ERR_BASE64_INVALID_CHARACTER              0x0012

static const unsigned char base64_enc_map[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};

static const unsigned char base64_dec_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
    54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
    5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
    29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
    39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 127, 127, 127, 127, 127
};

/*******************************************************
description�� function declaration
*******************************************************/
/*Global function declaration*/

/*Static function declaration*/
static int PP_CertDL_do_checksock(PrvtProt_task_t *task);
static int PP_CertDL_do_rcvMsg(PrvtProt_task_t *task);
static void PP_CertDL_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len);
static int PP_CertDL_do_wait(PrvtProt_task_t *task);
static int PP_CertDL_do_checkCertificate(PrvtProt_task_t *task);

static void PP_CertDL_send_cb(void * para);
static int 	PP_CertDL_CertDLReq(PrvtProt_task_t *task,PP_CertificateDownload_t *CertificateDownload);
static int  PP_CertDL_checkCipherCsr(void);
static int  MatchCertVerify(void);
static int 	PP_CertDL_base64_decode( unsigned char *dst, unsigned int *dlen, \
                     const unsigned char *src, int  slen );
static int PP_CertDL_getIdleNode(void);
static int PP_CertDL_do_EnableCertificate(PrvtProt_task_t *task);
static int PP_CertDL_CertEnReq(PrvtProt_task_t *task,PP_CertificateEn_t *CertificateEn);
static int PP_CertDL_getCertSn(void);
static int PP_CertDL_do_checkRevocationList(PrvtProt_task_t *task);
static int PP_CertDL_RevoListRenewReq(PrvtProt_task_t *task,PP_CertRevoList_t *CertRevoList);
static int PP_CertDL_do_checkCertStatus(void);
static int PP_CertDL_CertRenewReq(PrvtProt_task_t *task,PP_CertUpdata_t *CertUpdata);
static int PP_CertDL_checkRevoRenewCert(PrvtProt_task_t *task);
static int PP_CertDL_do_CertDownload(PrvtProt_task_t *task);
static int PP_CertDL_do_CertRenew(PrvtProt_task_t *task);
/******************************************************
description�� function code
******************************************************/
/******************************************************
*��������PP_CertDownload_init

*��  �Σ�void

*����ֵ��void

*��  ������ʼ��

*��  ע��
******************************************************/
void PP_CertDownload_init(void)
{
	//int res;
	memset(&PP_CertEn,0 , sizeof(PP_CertificateEn_t));
	memset(&PP_CertDL,0 , sizeof(PP_CertDL_t));
	memcpy(PP_CertDL.pack.Header.sign,"**",2);
	PP_CertDL.pack.Header.commtype.Byte = 0xe1;
	PP_CertDL.pack.Header.ver.Byte = 0x30;
	PP_CertDL.pack.Header.opera = 0x05;
	PP_CertDL.state.dlSt = PP_CERTDL_IDLE;

	PP_CertDownloadPara.eventid = 0;

	PP_CertDL.state.certDLTestflag = 0;

	unsigned int len = 1;
	cfg_get_para(CFG_ITEM_HOZON_TSP_CERT_EN,&PP_CertDL.state.CertEnflag,&len);
	cfg_get_para(CFG_ITEM_HOZON_TSP_CERT_VALID,&PP_CertDL.state.CertValid,&len);
#if 0
	FILE *fp;
	if((fp=fopen(PP_CERTDL_CERTPATH, "r")) != NULL)//检查证书文件是否存在
	{
		PP_CertDL.state.CertValid = 1;
		fclose(fp);
	}
#endif
	//PP_checkCertSt.checkStFlag = 1;
}

/******************************************************
*��������PP_CertDownload_mainfunction

*��  �Σ�void

*����ֵ��void

*��  ������������

*��  ע��
******************************************************/
int PP_CertDownload_mainfunction(void *task)
{
	int res;
	PrvtProt_task_t *task_ptr = (PrvtProt_task_t*)task;

	res = 		PP_CertDL_do_checksock(task_ptr) 			||	\
				PP_CertDL_do_rcvMsg(task_ptr) 				||	\
				PP_CertDL_do_wait(task_ptr) 				||	\
				PP_CertDL_do_checkCertificate(task_ptr);

	return res;
}

/******************************************************
*��������PP_CertDL_do_checksock

*��  �Σ�void

*����ֵ��void

*��  �������socket����

*��  ע��
******************************************************/
static int PP_CertDL_do_checksock(PrvtProt_task_t *task)
{
	if((1 == sockproxy_sgsocketState()) || \
			(1 == sockproxy_socketState()))//socket open
	{

		return 0;
	}
	return -1;
}

/******************************************************
*��������PP_CertDL_do_rcvMsg

*��  �Σ�void

*����ֵ��void

*��  �����������ݺ���

*��  ע��
******************************************************/
static int PP_CertDL_do_rcvMsg(PrvtProt_task_t *task)
{	
	int rlen = 0;
	PrvtProt_pack_t rcv_pack;
	memset(&rcv_pack,0 , sizeof(PrvtProt_pack_t));
	if ((rlen = RdPP_queue(PP_CERT_DL,rcv_pack.Header.sign,sizeof(PrvtProt_pack_t))) <= 0)
    {
		return 0;
	}
	
	log_i(LOG_HOZON, "receive cert download message\n");
	//protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
	if((rcv_pack.Header.sign[0] != 0x2A) || (rcv_pack.Header.sign[1] != 0x2A) || \
			(rlen <= 18))//
	{
		return 0;
	}
	
	if(rlen > (18 + PP_MSG_DATA_LEN))//
	{
		return 0;
	}
	PP_CertDL_RxMsgHandle(task,&rcv_pack,rlen);

	return 0;
}

/******************************************************
*��������PP_CertDL_RxMsgHandle

*��  �Σ�void

*����ֵ��void

*��  �����������ݴ���

*��  ע��
******************************************************/
static void PP_CertDL_RxMsgHandle(PrvtProt_task_t *task,PrvtProt_pack_t* rxPack,int len)
{

	if(PP_OPERATETYPE_CERTDL != rxPack->Header.opera)
	{
		log_e(LOG_HOZON, "unknow package");
		return;
	}

	if(rxPack->msgdata[0] == PP_CERTDL_MID_RESP)//mid == 2,cert download response
	{
		PP_CertDownloadPara.eventid = 0;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID] << 24;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+1] << 16;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+2] << 8;
		PP_CertDownloadPara.eventid |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_EVTID+3];

		log_i(LOG_HOZON, "PP_CertDownloadPara.eventid = %d\n",PP_CertDownloadPara.eventid);

		if(rxPack->msgdata[PP_CERTDL_RESP_RESULT] == 1)//成功  && tbox证书
		{
			if(rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE] == 1)//成功  && tbox证书 result=1时，不传错误类型
			{
				PP_CertDL.para.CertDLResp.certLength = 0;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN+0] << 24;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN+1] << 16;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN+2] << 8;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN+3];

				log_i(LOG_HOZON, "PP_CertDL.para.CertDLResp.certLength = %d\n",PP_CertDL.para.CertDLResp.certLength);
				log_i(LOG_HOZON, "certContent = %s\n",&rxPack->msgdata[PP_CERTDL_RESP_CERTCONTENT]);


				//保存证书内容
				FILE *updataF = fopen(PP_CERTDL_CERTPATH_UPDATE,"w");
				if(updataF != NULL)
				{
					log_i(LOG_HOZON,"save and verify authbook\n");
					unsigned char ucCertData[2048] = {0};     // CA机构颁发的证书
			        unsigned int uiCertDataLen = sizeof(ucCertData);
			        PP_CertDL_base64_decode(ucCertData, &uiCertDataLen,\
			        		&rxPack->msgdata[PP_CERTDL_RESP_CERTCONTENT],PP_CertDL.para.CertDLResp.certLength);

					fwrite(ucCertData,uiCertDataLen,1,updataF);
					fclose(updataF);
					PP_CertDownloadPara.tboxid = (unsigned int)rxPack->Header.tboxid;
					PP_CertDL.state.dlsuccess = PP_CERTDL_SUCCESS;
				}
				else
				{
					log_e(LOG_HOZON,"open userAuth.cer error\n");
					PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
				}
			}
			else
			{
				log_e(LOG_HOZON, "rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE] = %d\n",rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE]);
				PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
			}
		}
		else
		{
			log_e(LOG_HOZON, "rxPack->msgdata[PP_CERTDL_RESP_RESULT] = %d\n",rxPack->msgdata[PP_CERTDL_RESP_RESULT]);
			PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
		}
	}
	else if(rxPack->msgdata[0] == PP_CERTDL_MID_REVOLISTRESP)
	{//吊销证书列表响应
		log_i(LOG_HOZON,"revocation list response\n");
		if(rxPack->msgdata[5] == 1)//tbox证书
		{
			if(rxPack->msgdata[6] == 1)//成功, result=1时，不传错误类型
			{
				PP_CertRevoList.RLSt = 1;
				uint16_t	cretlistlen = 0;
				cretlistlen = rxPack->msgdata[7];
				cretlistlen = (cretlistlen << 8) + rxPack->msgdata[8];

				FILE *pfid = fopen("/usrdata/pem/tbox.crl","w");
				fwrite(&rxPack->msgdata[9],cretlistlen,1,pfid);
				fclose(pfid);

				uint16_t	certListSignLength  = 0;
				certListSignLength = rxPack->msgdata[9 + cretlistlen];
				certListSignLength = (certListSignLength << 8) + rxPack->msgdata[9 + cretlistlen + 1];
				FILE *pfid1 = fopen("/usrdata/pem/tboxsign.crl","w");
				fwrite(&rxPack->msgdata[9 + cretlistlen + 2],certListSignLength,1,pfid1);
				fclose(pfid1);
			}
			else
			{
				log_e(LOG_HOZON, "result rxPack->msgdata[6] = %d\n",rxPack->msgdata[6]);
				PP_CertRevoList.RLSt = 2;
			}
		}
		else
		{
			log_e(LOG_HOZON, "Cert type rxPack->msgdata[5] = %d\n",rxPack->msgdata[5]);
			PP_CertRevoList.RLSt = 2;
		}
	}
	else if(rxPack->msgdata[0] == PP_CERTDL_MID_UDREQRESP)
	{
		if(rxPack->msgdata[5] && rxPack->msgdata[6])
		{
			PP_CertUpdata.allowupdata = 1;
		}
		else
		{
			log_e(LOG_HOZON, "do not allow updata certificate\n");
			log_e(LOG_HOZON, "Cert type rxPack->msgdata[5] = %d\n",rxPack->msgdata[5]);
			log_e(LOG_HOZON, "Cert type rxPack->msgdata[6] = %d\n",rxPack->msgdata[6]);
			PP_CertUpdata.allowupdata = 2;
		}
	}
	else
	{}
}

/******************************************************
*��������PP_CertDL_do_wait

*��  �Σ�void

*����ֵ��void

*��  ��������Ƿ����¼��ȴ�Ӧ��

*��  ע��
******************************************************/
static int PP_CertDL_do_wait(PrvtProt_task_t *task)
{

	return 0;
}

/******************************************************
*��������PP_CertDL_do_checkCertificate

*��  �Σ�

*����ֵ��

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_checkCertificate(PrvtProt_task_t *task)
{
	#define	PP_CERTDL_TIMES		1
	#define	PP_CERTUPDATE_TIMES		1

	if(1 == sockproxy_sgsocketState())
	{

		switch(PP_CertDL.state.checkSt)
		{
			case PP_CHECK_CERT_IDLE:
			{
				PP_CertDL.state.verifyFlag = 0;
				PP_CertDL.state.certAvailableFlag = 0;
				if(1 == PP_CertDL.state.CertValid)
				{//检查证书可用性
					if(PP_CertUpdata.Cnt < PP_CERTUPDATE_TIMES)
					{
						PP_CertUpdata.Cnt++;
						PP_CertDL.state.checkSt = PP_CHECK_CERT_AVAILABILITY;
					}
				}
				else
				{//下载证书
					if((PP_CertDL.Cnt < PP_CERTDL_TIMES) || \
							(PP_CertDL.state.certDLTestflag))
					{
						PP_CertDL.Cnt++;
						PP_CertDL.state.certDLTestflag = 0;
						PP_CertDL.state.dlSt = PP_CERTDL_IDLE;
						PP_CertDL.state.checkSt = PP_CHECK_CERT_DL;
					}
				}
			}
			break;
			case PP_CHECK_CERT_AVAILABILITY:
			{
				uint8_t certAvailableSt;
				certAvailableSt = PP_CertDL_do_checkCertStatus();
				if(1 == certAvailableSt)//检查吊销和过期
				{
					log_i(LOG_HOZON, "certificate need renew\n");
					PP_CertDL.state.checkSt = PP_CHECK_CERT_RENEWCERT;
				}
				else if(0 == certAvailableSt)
				{
					log_i(LOG_HOZON, "certificate available\n");
					PP_CertDL.state.certAvailableFlag = 1;
					PP_CertDL.state.checkSt = PP_CHECK_CERT_END;
				}
				else
				{
					PP_CertDL.state.checkSt = PP_CHECK_CERT_END;
				}
			}
			break;
			case PP_CHECK_CERT_RENEWCERT:
			{
				uint8_t renewSt;
				renewSt = PP_CertDL_do_CertRenew(task);
				if(1 == renewSt)//更新完成
				{
					PP_CertDL.state.checkSt = PP_CHECK_CERT_VERIFYCERT;
				}
				else if(renewSt < 0)//更新失败
				{
					PP_CertDL.state.checkSt = PP_CHECK_CERT_END;
				}
				else
				{}
			}
			break;
			case PP_CHECK_CERT_DL:
			{
				int dlSt;
				dlSt = PP_CertDL_do_CertDownload(task);
				if(1 == dlSt)
				{//下载完成,验证证书
					log_i(LOG_HOZON, "auth download success\n");
					PP_CertDL.state.checkSt = PP_CHECK_CERT_VERIFYCERT;
				}
				else if(dlSt < 0)
				{//下载失败
					log_i(LOG_HOZON, "auth download fail\n");
					PP_CertDL.state.checkSt = PP_CHECK_CERT_END;
				}
				else
				{}
			}
			break;
			case PP_CHECK_CERT_VERIFYCERT:
			{
				if(0 == MatchCertVerify())//验证证书
				{
					log_i(LOG_HOZON,"verify userAuth.cer success\n");
					PP_checkCertSt.CertUpdataSt = 0;
					PrvtPro_SettboxId(PP_CertDownloadPara.tboxid);
					PP_CertDL.state.CertValid = 1;
					(void)cfg_set_para(CFG_ITEM_HOZON_TSP_CERT_VALID,&PP_CertDL.state.CertValid,1);
					PP_CertDL.state.CertEnflag = 0;
					(void)cfg_set_para(CFG_ITEM_HOZON_TSP_CERT_EN,&PP_CertDL.state.CertEnflag,1);
					PP_CertEn.CertEnCnt = 0;
					PP_CertDL.state.verifyFlag = 1;
				}
				else
				{
					log_e(LOG_HOZON,"verify userAuth.cer fail\n");
				}
				PP_CertDL.state.checkSt = PP_CHECK_CERT_END;
			}
			break;
			case PP_CHECK_CERT_END:
			{
				PP_CertDL.state.checkSt = PP_CHECK_CERT_IDLE;
			}
			break;
			default:
			break;
		}

		if(((1 == PP_CertDL.state.CertValid) && (PP_CertDL.state.verifyFlag == 1)) || \
				(1 == PP_CertDL.state.certAvailableFlag))

		{
			PP_CertDL.state.verifyFlag = 0;
			PP_CertDL.state.certAvailableFlag = 0;
			sockproxy_socketclose();
		}
	}
	else if(1 == sockproxy_socketState())//双向链路
	{
		PP_CertDL_do_EnableCertificate(task);
		PP_CertDL_do_checkRevocationList(task);
	}
	else
	{}

	return 0;
}


/******************************************************
*��������PP_CertDL_do_CertDownload

*��  �Σ�

*����ֵ��

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_CertDownload(PrvtProt_task_t *task)
{

	switch(PP_CertDL.state.dlSt)
	{
		case PP_CERTDL_IDLE:
		{
			log_i(LOG_HOZON, "cdrtificate download request\n");
			PP_CertDL.state.dlsuccess = PP_CERTDL_INITVAL;
			PP_CertDL.state.waittime = tm_get_time();
			PP_CertDL.state.dlSt = PP_CERTDL_CHECK_CIPHER_CSR;
		}
		break;
		case PP_CERTDL_CHECK_CIPHER_CSR:
		{
			if((tm_get_time() - PP_CertDL.state.waittime) <= 15000)
			{
				if(PP_rmtCfg_getIccid((uint8_t*)PP_CertDL_ICCID))
				{
					if(PrvtProt_tboxsnValidity())
					{
						PrvtProt_gettboxsn(PP_CertDL_SN);
						if(PP_CertDL_checkCipherCsr() == 0)
						{
							PP_CertDL.state.dlSt = PP_CERTDL_DLREQ;
						}
						else
						{
							log_e(LOG_HOZON, "check cipher fail\n");
							PP_CertDL.state.dlSt = PP_CERTDL_END;
						}
					}
					else
					{
						log_e(LOG_HOZON, "tbox sn invalid\n");
						PP_CertDL.state.dlSt = PP_CERTDL_END;
					}
				}
			}
			else
			{
				log_e(LOG_HOZON, "iccid read timeout\n");
				PP_CertDL.state.dlSt = PP_CERTDL_END;
			}
		}
		break;
		case PP_CERTDL_DLREQ:
		{
			PP_CertDL.para.CertDLReq.mid = PP_CERTDL_MID_REQ;
			PP_CertDL.para.CertDLReq.eventid = PP_CertDownloadPara.eventid;
			PP_CertDL.para.CertDLReq.cerType = 1;//tbox
			//生成消息列表,VIN &&密文信息 &&密钥编号&& 证书申请文件内容 CSR，其中密文信息中包含 TBOXSN、IMISI、随机数信息
			unsigned char gcsroutdata[4096] = {0};
			int 	 gcoutlen = 0;
			int 	 iRet = 0;
			/*读取CSR文件内容*/
			iRet = HzTboxApplicationData(PP_CERTDL_TWOCERTCSRPATH , \
					"/usrdata/pki/sn_sim_encinfo.txt", gcsroutdata, &gcoutlen);
			if(iRet == 3630)
			{
				if(gb32960_vinValidity())
				{
					char vin[18] = {0};
					gb32960_getvin(vin);
					PP_CertDL.para.CertDLReq.infoListLength = 19 + gcoutlen;//19 == vin + "&&"
					memcpy(PP_CertDL.para.CertDLReq.infoList,vin,17);
					memcpy(PP_CertDL.para.CertDLReq.infoList+17,"&&",2);
					memcpy(PP_CertDL.para.CertDLReq.infoList+19,gcsroutdata,gcoutlen);
					PP_CertDL_CertDLReq(task,&PP_CertDL.para);
					PP_CertDL.state.waittime = tm_get_time();
					PP_CertDL.state.dlSt = PP_CERTDL_DLREQWAIT;
				}
				else
				{
					log_e(LOG_HOZON,"vin invalid\n");
					PP_CertDL.state.dlSt = PP_CERTDL_END;
				}
			}
			else
			{
				log_e(LOG_HOZON,"HzTboxApplicationData error+++++++++++++++iRet[%d] \n", iRet);
				PP_CertDL.state.dlSt = PP_CERTDL_END;
			}

		}
		break;
		case PP_CERTDL_DLREQWAIT:
		{
			if((tm_get_time() - PP_CertDL.state.waittime) <= PP_CERTDL_DLTIMEOUT)
			{
				if(PP_CERTDL_INITVAL != PP_CertDL.state.dlsuccess)
				{
					log_i(LOG_HOZON, "auth download \n");
					PP_CertDL.state.dlSt = PP_CERTDL_END;
				}
			}
			else//timeout
			{
				log_e(LOG_HOZON, "auth download timeout\n");
				PP_CertDL.state.dlSt = PP_CERTDL_END;
			}
		}
		break;
		case PP_CERTDL_END:
		{
			PP_CertDL.state.dlSt = PP_CERTDL_IDLE;
			if(PP_CERTDL_SUCCESS == PP_CertDL.state.dlsuccess)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_do_CertRenew

*��  �Σ�

*����ֵ��

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_CertRenew(PrvtProt_task_t *task)
{
	static char checkAvailSt = 0;

	switch(checkAvailSt)
	{
		case 0:
		{
			PP_CertRevoList.checkRevoFlag = 1;
			checkAvailSt = 1;
		}
		break;
		case 1://请求吊销列表
		{
			int checkRevolistSt;
			checkRevolistSt = PP_CertDL_do_checkRevocationList(task);
			if(1 == checkRevolistSt)
			{
				PP_checkCertSt.CertUpdataReq = 1;
				checkAvailSt = 2;//
			}
			else if(checkRevolistSt < 0)
			{
				PP_checkCertSt.CertUpdataReq = 1;
				checkAvailSt = 2;//
			}
			else
			{}
		}
		break;
		case 2://更新证书
		{
			uint8_t updateSt;
			updateSt = PP_CertDL_checkRevoRenewCert(task);
			if(1 == updateSt)//更新完成
			{
				log_i(LOG_HOZON,"update userAuth.cer success\n");
				checkAvailSt = 0;
				return 1;
			}
			else if(updateSt < 0)//更新失败
			{
				log_i(LOG_HOZON,"update userAuth.cer fail\n");
				checkAvailSt = 0;
				return -1;
			}
			else
			{}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_do_EnableCertificate

*��  �Σ�

*����ֵ�� 启用证书

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_EnableCertificate(PrvtProt_task_t *task)
{
	static 	uint8_t	CertEnSt = PP_CERTEN_IDLE;
	#define PP_CERTEN_TIMES		1
	static 	uint64_t 	Enwaittime;

	if(1 != PP_CertDL.state.CertValid)
	{
		return 0;
	}

	switch(CertEnSt)
	{
		case PP_CERTEN_IDLE:
		{
			if((PP_CertDL.state.CertEnflag == 0) &&	(PP_CertEn.CertEnCnt < PP_CERTEN_TIMES))//证书未启用,启用证书
			{
				log_i(LOG_HOZON, "certificate unenable,start to enable certificate\n");
				PP_CertEn.CertEnCnt++;
				CertEnSt = PP_CERTEN_REQ;
			}
		}
		break;
		case PP_CERTEN_REQ:
		{
			if(0 == PP_CertDL_getCertSn())
			{
				PP_CertEn.para.mid = PP_CERTDL_MID_CERT_EN;
				PP_CertEn.para.eventid = 0x55;
				PP_CertEn.para.certType = 1;
				(void)PP_CertDL_CertEnReq(task,&PP_CertEn);
				PP_CertEn.enSt = 0;
				Enwaittime = tm_get_time();
				CertEnSt = PP_CERTEN_WAIT;
			}
			else
			{
				log_e(LOG_HOZON, "get certificate Sn fail\n");
				CertEnSt = PP_CERTEN_END;
			}
		}
		break;
		case PP_CERTEN_WAIT:
		{
			if((tm_get_time() - Enwaittime) <= 5000)
			{
				if(PP_CertEn.enSt == 1)
				{
					log_i(LOG_HOZON, "enable certificate success\n");
					PP_CertDL.state.CertEnflag = 1;
					(void)cfg_set_para(CFG_ITEM_HOZON_TSP_CERT_EN,&PP_CertDL.state.CertEnflag,1);
					(void)cfg_set_para(CFG_ITEM_HOZON_TSP_CERT,PP_CertEn.para.certSn,32);
					PP_CertRevoList.checkRevoFlag = 1;//检查吊销列表
					CertEnSt = PP_CERTEN_END;
				}
				else if(PP_CertEn.enSt == 2)
				{
					log_e(LOG_HOZON, "enable certificate fail\n");
					CertEnSt = PP_CERTEN_END;
				}
				else
				{}
			}
			else
			{
				log_e(LOG_HOZON, "enable certificate timeout\n");
				CertEnSt = PP_CERTEN_END;
			}
		}
		break;
		case PP_CERTEN_END:
		{
			CertEnSt = PP_CERTEN_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_do_checkRevocationList

*��  �Σ�

*����ֵ�� 启用证书

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_checkRevocationList(PrvtProt_task_t *task)
{
	static 	uint8_t		CheckRevoSt = PP_CHECK_REVO_IDLE;
	static 	uint64_t 	checkrevowaittime;

	switch(CheckRevoSt)
	{
		case PP_CHECK_REVO_IDLE:
		{
			if(1 == PP_CertRevoList.checkRevoFlag)//检查吊销列表
			{
				log_i(LOG_HOZON, "check certificate revocation list\n");
				PP_CertRevoList.checkRevoFlag = 0;
				CheckRevoSt = PP_CHECK_REVO_REQ;
			}
		}
		break;
		case PP_CHECK_REVO_REQ:
		{
			int crtlen=0;
			FILE *fp;
			fp=fopen("/usrdata/pem/tbox.crl","r");
			if(fp!=NULL)
			{
			    fseek(fp, 0, SEEK_END);//将文件指针移动文件结尾
			    crtlen = ftell (fp); ///求出当前文件指针距离文件开始的字节数
			    log_i(LOG_HOZON, "crtlen2 = %d\n",crtlen);
			    fclose (fp);

				PP_CertRevoList.para.mid = PP_CERTDL_MID_REVOLISTREQ;
				PP_CertRevoList.para.eventid = 0x55;
				PP_CertRevoList.para.certType = 1;
				PP_CertRevoList.para.failureType = 0;
				PP_CertRevoList.para.crlLength = crtlen;
				PP_CertDL_RevoListRenewReq(task,&PP_CertRevoList);
				PP_CertRevoList.RLSt = 0;
				checkrevowaittime = tm_get_time();
				CheckRevoSt = PP_CHECK_REVO_WAIT;
			}
			else
			{
				log_e(LOG_HOZON, "error open /usrdata/pem/tbox.crl file\n");
				CheckRevoSt = PP_CHECK_REVO_END;
				return -1;
			}
		}
		break;
		case PP_CHECK_REVO_WAIT:
		{
			if((tm_get_time() - checkrevowaittime) <= 5000)
			{
				if(1 == PP_CertRevoList.RLSt)
				{
					log_i(LOG_HOZON, "check revo list success\n");
					CheckRevoSt = PP_CHECK_REVO_END;
					return 1;
				}
				else if(2 == PP_CertRevoList.RLSt)
				{
					log_i(LOG_HOZON, "check revo list fail\n");
					CheckRevoSt = PP_CHECK_REVO_END;
					return -1;
				}
				else
				{}
			}
			else
			{
				log_e(LOG_HOZON, "check certificate revo list timeout\n");
				CheckRevoSt = PP_CHECK_REVO_END;
				return -1;
			}
		}
		break;
		case PP_CHECK_REVO_END:
		{
			CheckRevoSt = PP_CHECK_REVO_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}


/******************************************************
*��������PP_CertDL_do_checkCertStatus

*��  �Σ�

*����ֵ�� 检查证书更新和吊销状态

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_do_checkCertStatus(void)
{
	int iRet;

	//if(1 == PP_checkCertSt.checkStFlag)
	{
		//PP_checkCertSt.checkStFlag = 0;
		/*
		 *检查证书更新
		 */
		{
			int statecert;
			//int stat = 0;

			time_t tm = time(NULL);//其值表示从UTC（Coordinated Universal Time）时间1970年1月1日00:00:00（称为UNIX系统的Epoch时间）到当前时刻的>秒数
			log_i(LOG_HOZON,"tm = [%d]\n", tm);

			iRet = HzTboxCertUpdCheck(PP_CERTDL_CERTPATH,"DER",tm,&statecert);
			if(iRet!=6010)
			{
				log_i(LOG_HOZON,"HzTboxCertUpdCheck error+++++++++++++++iRet[%d] \n", iRet);
				return -1;
			}
			else
			{
				if(statecert==1)
				{
					log_i(LOG_HOZON,"Update time not available\n");
				}
				else
				{
					if(statecert==2)
					{
						log_i(LOG_HOZON,"Update time is up，\n");
					}
					else if(statecert==3)
					{
						log_i(LOG_HOZON,"Certificate invalid\n");
					}
					else
					{}
					return 1;
				}
			}
		}

		/*
		 * 检查证书吊销状态
		 */
		{
			int crlstatus;
			if(0==PP_CertDL_getCertSn())
			{
				iRet = HzTboxUcRevokeStatus("/usrdata/pem/tbox.crl",PP_CertEn.para.certSn,&crlstatus);
				if(iRet != 6205)
				{
					log_i(LOG_HOZON,"HzTboxUcRevokeStatus error+++++++++++++++iRet[%d] \n", iRet);
					return -1;
				}
				else
				{
					if(1 == crlstatus)//已吊销
					{
						log_i(LOG_HOZON,"Certificate revoked\n");
						return 1;
					}
					else
					{}
				}
			}
			else
			{
				log_e(LOG_HOZON,"PP_CertDL_getCertSn error+++++++++++++++iRet[%d] \n");
				return -1;
			}

		}
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_checkRevoRenewCert

*��  �Σ�

*����ֵ�� 更新证书

*��  �������������

*��  ע��
******************************************************/
static int PP_CertDL_checkRevoRenewCert(PrvtProt_task_t *task)
{
	//int i;
	int iRet;
	static 	uint64_t 	updatawaittime;

	switch(PP_checkCertSt.updataSt)
	{
		case PP_CERTUPDATA_IDLE:
		{
			if(1 == PP_checkCertSt.CertUpdataReq)
			{
				PP_CertDL.state.dlsuccess = PP_CERTDL_INITVAL;
				PP_checkCertSt.CertUpdataReq = 0;
				PP_checkCertSt.CertUpdataSt = 1;
				PP_checkCertSt.updataSt = PP_CERTUPDATA_CKREQ;
			}
		}
		break;
		case PP_CERTUPDATA_CKREQ:
		{
			int len;
			//int dstlen;
			unsigned char signinfo[1024]={0};
			//char signsernum[1024]={0};
			PP_CertUpdata.para.mid = PP_CERTDL_MID_UDREQ;
			PP_CertUpdata.para.eventid = 0x55;
			PP_CertUpdata.para.certType = 1;
			//dev_get_KL15_signal();
			memset(PP_CertUpdata.para.certSn,0,sizeof(PP_CertUpdata.para.certSn));
			iRet = HzTboxGetserialNumber(PP_CERTDL_CERTPATH,"DER",(char*)PP_CertUpdata.para.certSn);//DER
			if(iRet == 6107)
			{
				int certSnSignLength;
				PP_CertUpdata.para.certSnLength = strlen((char*)PP_CertUpdata.para.certSn);
				log_i(LOG_HOZON,"PPP_CertUpdata.para.certSnLength = %d ",PP_CertUpdata.para.certSnLength);
				log_i(LOG_HOZON,"PP_CertUpdata.para.certSn = %s\n", PP_CertUpdata.para.certSn);
				HzTboxdoSign(PP_CertUpdata.para.certSn,PP_CERTDL_TWOCERTKEYPATH,(char*)signinfo,&len);
				memset(PP_CertUpdata.para.certSnSign,0,sizeof(PP_CertUpdata.para.certSnSign));
				certSnSignLength = sizeof(PP_CertUpdata.para.certSnSign);
				hz_base64_encode(PP_CertUpdata.para.certSnSign,&certSnSignLength,signinfo,len);
				PP_CertUpdata.para.certSnSignLength = certSnSignLength;
				log_i(LOG_HOZON,"PPP_CertUpdata.para.certSnSignLength = %d ",PP_CertUpdata.para.certSnSignLength);
				log_i(LOG_HOZON,"PP_CertUpdata.para.certSnSign = %s\n", PP_CertUpdata.para.certSnSign);
				PP_CertDL_CertRenewReq(task,&PP_CertUpdata);
				PP_CertUpdata.allowupdata = 0;
				updatawaittime = tm_get_time();
				PP_checkCertSt.updataSt = PP_CERTUPDATA_CKREQWAIT;
			}
			else
			{
				log_e(LOG_HOZON,"HzTboxGetserialNumber error+++++++++++++++iRet[%d] \n", iRet);
				PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
			}
		}
		break;
		case PP_CERTUPDATA_CKREQWAIT:
		{
			if((tm_get_time() - updatawaittime) < 5000)
			{
				if(1 == PP_CertUpdata.allowupdata)
				{
					log_i(LOG_HOZON,"Cert update allow\n");
					PP_checkCertSt.updataSt = PP_CERTUPDATA_UDREQ;
				}
				else if(2 == PP_CertUpdata.allowupdata)
				{
					log_i(LOG_HOZON,"Cert update not allow\n");
					PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
				}
				else
				{}
			}
			else
			{
				log_e(LOG_HOZON,"Cert update timeout\n");
				PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
			}
		}
		break;
		case PP_CERTUPDATA_UDREQ:
		{
			char vin[18] = {0};

			if(dir_exists("/usrdata/pki/update/") == 0 &&
			        dir_make_path("/usrdata/pki/update/", S_IRUSR | S_IWUSR, false) != 0)
			{
		        log_e(LOG_HOZON, "open cache path fail, reset all index");
		        PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
			}
			else
			{
				log_i(LOG_HOZON, "/usrdata/pki/update exist or create seccess\n");

				CAR_INFO car_information = {0};

				gb32960_getvin(vin);
				car_information.tty_type="TBOX";
				car_information.unique_id=vin;
				car_information.carowner_acct="18221802221";
				car_information.impower_acct="12900100101";
				iRet = HzTboxGenCertCsr("SM2", NULL, &car_information, \
						"CN|shanghai|shanghai|hezhong|hezhong|two_certreqmain|sm2_ecc_two_certreqmain@160.com", \
						"/usrdata/pki/update" ,"two_certreqmain", "PEM");
				if(iRet == 3180)
				{
					FILE *fp;
					fp=fopen("/usrdata/pki/update/two_certreqmain.csr","r");
					if(fp != NULL)
					{
						int size = 0;
						char *filedata_ptr ;
						//求得文件的大小
						fseek(fp, 0, SEEK_END);
						size = ftell(fp);
						rewind(fp);
						//申请一块能装下整个文件的空间
						filedata_ptr = (char*)malloc(sizeof(char)*size);
						fread(filedata_ptr,1,size,fp);//每次读一个，共读size次

						log_i(LOG_HOZON,"two_certreqmain.csr =  %s",filedata_ptr);
						PP_CertDL.para.CertDLReq.infoListLength = size + strlen(vin) + strlen("&&0&&0&&");
						memcpy(PP_CertDL.para.CertDLReq.infoList,vin,strlen(vin));
						memcpy(PP_CertDL.para.CertDLReq.infoList+17,"&&0&&0&&",strlen("&&0&&0&&"));
						memcpy(PP_CertDL.para.CertDLReq.infoList+17+8,filedata_ptr,size);

						fclose(fp);
						free(filedata_ptr);

						PP_CertDL.para.CertDLReq.mid = PP_CERTDL_MID_REQ;
						PP_CertDL.para.CertDLReq.eventid = PP_CertDownloadPara.eventid;
						PP_CertDL.para.CertDLReq.cerType = 1;//tbox
						PP_CertDL_CertDLReq(task,&PP_CertDL.para);
						//PP_CertDL.state.dlsuccess = PP_CERTDL_INITVAL;
						updatawaittime = tm_get_time();
						PP_checkCertSt.updataSt = PP_CERTUPDATA_UDWAIT;
					}
					else
					{
						log_e(LOG_HOZON,"open two_certreqmain.csr fail\n");
						PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
					}
				}
				else
				{
					log_e(LOG_HOZON,"HzTboxGenCertCsr error ---------------++++++++++++++.[%d]\n", iRet);
					PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
				}
			}
		}
		break;
		case PP_CERTUPDATA_UDWAIT:
		{
			if((tm_get_time() - updatawaittime) < PP_CERTDL_DLTIMEOUT)
			{
				if(PP_CERTDL_INITVAL != PP_CertDL.state.dlsuccess)
				{
					PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
				}
			}
			else
			{
				log_e(LOG_HOZON,"Cert update req timeout\n");
				PP_checkCertSt.updataSt = PP_CERTUPDATA_END;
			}
		}
		break;
		case PP_CERTUPDATA_END:
		{
			//PP_checkCertSt.CertUpdataSt = 0;
			PP_checkCertSt.updataSt = PP_CERTUPDATA_IDLE;

			if(((1 == PP_CertUpdata.allowupdata) && (PP_CERTDL_SUCCESS == PP_CertDL.state.dlsuccess)) || \
					(2 == PP_CertUpdata.allowupdata))
			{
				log_i(LOG_HOZON,"Cert update success\n");
				return 1;
			}
			else
			{
				log_i(LOG_HOZON,"Cert update fail\n");
				return -1;
			}
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static int PP_CertDL_CertRenewReq(PrvtProt_task_t *task,PP_CertUpdata_t *CertUpdata)
{
	int i;
	PrvtProt_pack_t	PP_Certupdata_pack;

	/* header */
	memcpy(PP_Certupdata_pack.Header.sign,"**",2);
	PP_Certupdata_pack.Header.commtype.Byte = 0xe1;
	PP_Certupdata_pack.Header.ver.Byte = 0x30;
	PP_Certupdata_pack.Header.opera = 0x05;
	PP_Certupdata_pack.Header.ver.Byte = task->version;
	PP_Certupdata_pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_Certupdata_pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/* message data */
	PP_Certupdata_pack.msgdata[0] = CertUpdata->para.mid;
	PP_Certupdata_pack.msgdata[1] = (uint8_t)(CertUpdata->para.eventid >> 24);
	PP_Certupdata_pack.msgdata[2] = (uint8_t)(CertUpdata->para.eventid >> 16);
	PP_Certupdata_pack.msgdata[3] = (uint8_t)(CertUpdata->para.eventid >> 8);
	PP_Certupdata_pack.msgdata[4] = (uint8_t)CertUpdata->para.eventid;
	PP_Certupdata_pack.msgdata[5] = CertUpdata->para.certType;//证书类型：1- tbox
	PP_Certupdata_pack.msgdata[6] = CertUpdata->para.certSnLength;
	for(i = 0;i < CertUpdata->para.certSnLength;i++)
	{
		PP_Certupdata_pack.msgdata[7+i]  = CertUpdata->para.certSn[i];
	}

	PP_Certupdata_pack.msgdata[7+CertUpdata->para.certSnLength] = CertUpdata->para.certSnSignLength;
	for(i = 0;i < CertUpdata->para.certSnSignLength;i++)
	{
		PP_Certupdata_pack.msgdata[7+CertUpdata->para.certSnLength + 1 + i]  = CertUpdata->para.certSnSign[i];
	}

	log_i(LOG_HOZON, "CertUpdata->mid = %d\n",CertUpdata->para.mid);
	log_i(LOG_HOZON, "CertUpdata->eventid = %d\n",CertUpdata->para.eventid);
	log_i(LOG_HOZON, "CertUpdata->certType = %d\n",CertUpdata->para.certType);
	log_i(LOG_HOZON, "CertUpdata->certSnLength = %d\n",CertUpdata->para.certSnLength);
	log_i(LOG_HOZON, "CertUpdata->certSn = %s\n",CertUpdata->para.certSn);
	log_i(LOG_HOZON, "CertUpdata->certSnSignLength = %d\n",CertUpdata->para.certSnSignLength);
	log_i(LOG_HOZON, "CertUpdata->certSnSign = %s\n",CertUpdata->para.certSnSign);

	PP_Certupdata_pack.totallen = 18 + 5 + 1 + CertUpdata->para.certSnLength + 1 + CertUpdata->para.certSnSignLength;
	PP_Certupdata_pack.Header.msglen = PrvtPro_BSEndianReverse((long)PP_Certupdata_pack.totallen);

	i = PP_CertDL_getIdleNode();
	memset(&CertDL_TxInform[i],0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform[i].mid = CertUpdata->para.mid;
	CertDL_TxInform[i].eventtime = tm_get_time();
	CertDL_TxInform[i].pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_Certupdata_pack.Header.sign,PP_Certupdata_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform[i]);
	//protocol_dump(LOG_HOZON, "Cert_updata_request", PP_Certupdata_pack.Header.sign,PP_Certupdata_pack.totallen,1);

	return 1;
}

/******************************************************
*��������

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static int PP_CertDL_RevoListRenewReq(PrvtProt_task_t *task,PP_CertRevoList_t *CertRevoList)
{
	int i;
	PrvtProt_pack_t	PP_CertRevoList_pack;

	/* header */
	memcpy(PP_CertRevoList_pack.Header.sign,"**",2);
	PP_CertRevoList_pack.Header.commtype.Byte = 0xe1;
	PP_CertRevoList_pack.Header.ver.Byte = 0x30;
	PP_CertRevoList_pack.Header.opera = 0x05;
	PP_CertRevoList_pack.Header.ver.Byte = task->version;
	PP_CertRevoList_pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_CertRevoList_pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/* message data */
	PP_CertRevoList_pack.msgdata[0] = CertRevoList->para.mid;
	PP_CertRevoList_pack.msgdata[1] = (uint8_t)(CertRevoList->para.eventid >> 24);
	PP_CertRevoList_pack.msgdata[2] = (uint8_t)(CertRevoList->para.eventid >> 16);
	PP_CertRevoList_pack.msgdata[3] = (uint8_t)(CertRevoList->para.eventid >> 8);
	PP_CertRevoList_pack.msgdata[4] = (uint8_t)CertRevoList->para.eventid;
	PP_CertRevoList_pack.msgdata[5] = CertRevoList->para.certType;//证书类型：1- tbox
	PP_CertRevoList_pack.msgdata[6] = CertRevoList->para.failureType;
	PP_CertRevoList_pack.msgdata[7] = CertRevoList->para.crlLength;
	log_i(LOG_HOZON, "CertRevoList->mid = %d\n",CertRevoList->para.mid);
	log_i(LOG_HOZON, "CertRevoList->eventid = %d\n",CertRevoList->para.eventid);
	log_i(LOG_HOZON, "CertRevoList->certType = %d\n",CertRevoList->para.certType);
	log_i(LOG_HOZON, "CertRevoList->failureType = %d\n",CertRevoList->para.failureType);
	log_i(LOG_HOZON, "CertRevoList->crlLength = %d\n",CertRevoList->para.crlLength);

	PP_CertRevoList_pack.totallen = 18 + 9;
	PP_CertRevoList_pack.Header.msglen = PrvtPro_BSEndianReverse((long)PP_CertRevoList_pack.totallen);

	i = PP_CertDL_getIdleNode();
	memset(&CertDL_TxInform[i],0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform[i].mid = CertRevoList->para.mid;
	CertDL_TxInform[i].eventtime = tm_get_time();
	CertDL_TxInform[i].pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_CertRevoList_pack.Header.sign,PP_CertRevoList_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform[i]);
	//protocol_dump(LOG_HOZON, "CertDownload_request", PP_CertRevoList_pack.Header.sign,PP_CertRevoList_pack.totallen,1);

	return 1;
}

/******************************************************
*��������

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static int PP_CertDL_CertEnReq(PrvtProt_task_t *task,PP_CertificateEn_t *CertificateEn)
{
	int i;
	PrvtProt_pack_t	PP_CertEn_pack;

	/* header */
	memcpy(PP_CertEn_pack.Header.sign,"**",2);
	PP_CertEn_pack.Header.commtype.Byte = 0xe1;
	PP_CertEn_pack.Header.ver.Byte = 0x30;
	PP_CertEn_pack.Header.opera = 0x05;
	PP_CertEn_pack.Header.ver.Byte = task->version;
	PP_CertEn_pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_CertEn_pack.Header.tboxid = PrvtPro_BSEndianReverse((uint32_t)task->tboxid);

	/* message data */
	PP_CertEn_pack.msgdata[0] = CertificateEn->para.mid;
	PP_CertEn_pack.msgdata[1] = (uint8_t)(CertificateEn->para.eventid >> 24);
	PP_CertEn_pack.msgdata[2] = (uint8_t)(CertificateEn->para.eventid >> 16);
	PP_CertEn_pack.msgdata[3] = (uint8_t)(CertificateEn->para.eventid >> 8);
	PP_CertEn_pack.msgdata[4] = (uint8_t)CertificateEn->para.eventid;
	PP_CertEn_pack.msgdata[5] = CertificateEn->para.certType;//证书类型：1- tbox
	PP_CertEn_pack.msgdata[6] = CertificateEn->para.failureType;
	PP_CertEn_pack.msgdata[7] = CertificateEn->para.certSnLength;
	log_i(LOG_HOZON, "CertificateEn->mid = %d\n",CertificateEn->para.mid);
	log_i(LOG_HOZON, "CertificateEn->eventid = %d\n",CertificateEn->para.eventid);
	log_i(LOG_HOZON, "CertificateEn->certType = %d\n",CertificateEn->para.certType);
	log_i(LOG_HOZON, "CertificateEn->failureType = %d\n",CertificateEn->para.failureType);
	log_i(LOG_HOZON, "CertificateEn->certSnLength = %d\n",CertificateEn->para.certSnLength);
	for(i = 0;i < CertificateEn->para.certSnLength;i++)
	{
		PP_CertEn_pack.msgdata[8+i] = CertificateEn->para.certSn[i];
	}

	log_i(LOG_HOZON, "CertificateEn->certSn = %s\n",CertificateEn->para.certSn);

	PP_CertEn_pack.totallen = 18 + 8 + CertificateEn->para.certSnLength;
	PP_CertEn_pack.Header.msglen = PrvtPro_BSEndianReverse((long)PP_CertEn_pack.totallen);

	i = PP_CertDL_getIdleNode();
	memset(&CertDL_TxInform[i],0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform[i].mid = CertificateEn->para.mid;
	CertDL_TxInform[i].eventtime = tm_get_time();
	CertDL_TxInform[i].pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_CertEn_pack.Header.sign,PP_CertEn_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform[i]);
	//protocol_dump(LOG_HOZON, "CertDownload_request", PP_CertEn_pack.Header.sign,PP_CertEn_pack.totallen,1);

	return 1;
}

/******************************************************
*��������

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static int PP_CertDL_CertDLReq(PrvtProt_task_t *task,PP_CertificateDownload_t *CertificateDownload)
{
	int i;
	PrvtProt_pack_t	PP_CertDL_pack;

	/* header */
	memcpy(PP_CertDL.pack.Header.sign,"**",2);
	PP_CertDL.pack.Header.commtype.Byte = 0xe1;
	PP_CertDL.pack.Header.ver.Byte = 0x30;
	PP_CertDL.pack.Header.opera = 0x05;
	PP_CertDL.pack.Header.ver.Byte = task->version;
	PP_CertDL.pack.Header.nonce  = PrvtPro_BSEndianReverse((uint32_t)task->nonce);
	PP_CertDL.pack.Header.tboxid = PrvtPro_BSEndianReverse(0);//请求下载证书时，tboxid == 0;返回消息里面会带tboxid
	memcpy(&PP_CertDL_pack, &PP_CertDL.pack.Header, sizeof(PrvtProt_pack_Header_t));

	/* message data */
	PP_CertDL_pack.msgdata[0] = CertificateDownload->CertDLReq.mid;
	PP_CertDL_pack.msgdata[1] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 24);
	PP_CertDL_pack.msgdata[2] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 16);
	PP_CertDL_pack.msgdata[3] = (uint8_t)(CertificateDownload->CertDLReq.eventid >> 8);
	PP_CertDL_pack.msgdata[4] = (uint8_t)CertificateDownload->CertDLReq.eventid;
	PP_CertDL_pack.msgdata[5] = CertificateDownload->CertDLReq.cerType;//证书类型：1- tbox
	PP_CertDL_pack.msgdata[6] = (uint8_t)(CertificateDownload->CertDLReq.infoListLength >> 8);
	PP_CertDL_pack.msgdata[7] = (uint8_t)CertificateDownload->CertDLReq.infoListLength;
	log_i(LOG_HOZON, "CertificateDownload->CertDLReq.mid = %d\n",CertificateDownload->CertDLReq.mid);
	log_i(LOG_HOZON, "CertificateDownload->CertDLReq.eventid = %d\n",CertificateDownload->CertDLReq.eventid);
	log_i(LOG_HOZON, "CertificateDownload->CertDLReq.cerType = %d\n",CertificateDownload->CertDLReq.cerType);
	for(i = 0;i < CertificateDownload->CertDLReq.infoListLength;i++)
	{
		PP_CertDL_pack.msgdata[8+i] = CertificateDownload->CertDLReq.infoList[i];
	}

	log_i(LOG_HOZON, "cert infoList Length = %d\n",CertificateDownload->CertDLReq.infoListLength);
	log_i(LOG_HOZON, "cert infoList = %s\n",CertificateDownload->CertDLReq.infoList);

	PP_CertDL_pack.totallen = 18 + 8 + CertificateDownload->CertDLReq.infoListLength;
	PP_CertDL_pack.Header.msglen = PrvtPro_BSEndianReverse((long)PP_CertDL_pack.totallen);

	i = PP_CertDL_getIdleNode();
	memset(&CertDL_TxInform[i],0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform[i].mid = CertificateDownload->CertDLReq.mid;
	CertDL_TxInform[i].eventtime = tm_get_time();
	CertDL_TxInform[i].pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform[i]);
	//protocol_dump(LOG_HOZON, "CertDownload_request", PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,1);

	return 1;
}


/******************************************************
*��������PP_CertDL_send_cb

*��  �Σ�

*����ֵ��

*��  ����

*��  ע��
******************************************************/
static void PP_CertDL_send_cb(void * para)
{
	PrvtProt_TxInform_t *TxInform_ptr = (PrvtProt_TxInform_t*)para;
	log_i(LOG_HOZON, "aid = %d",TxInform_ptr->aid);
	log_i(LOG_HOZON, "mid = %d",TxInform_ptr->mid);
	log_i(LOG_HOZON, "pakgtype = %d",TxInform_ptr->pakgtype);
	log_i(LOG_HOZON, "eventtime = %d",TxInform_ptr->eventtime);
	log_i(LOG_HOZON, "successflg = %d",TxInform_ptr->successflg);
	log_i(LOG_HOZON, "failresion = %d",TxInform_ptr->failresion);
	log_i(LOG_HOZON, "txfailtime = %d",TxInform_ptr->txfailtime);

	if(PP_CERTDL_MID_CERT_EN == TxInform_ptr->mid)
	{
		if(PP_TXPAKG_SUCCESS == TxInform_ptr->successflg)
		{
			PP_CertEn.enSt = 1;
		}
		else
		{
			PP_CertEn.enSt = 2;
		}
	}

	TxInform_ptr->idleflag = 0;
}

/******************************************************
*��������PP_CertDL_SetCertDLReq

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
void PP_CertDL_SetCertDLReq(unsigned char req)
{
	//PP_CertDL.state.CertValid 	   = 0;
	PP_CertDL.state.certDLTestflag = req;
}

/******************************************************
*��������PP_CertDL_SetCertDLUpdata

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
void PP_CertDL_SetCertDLUpdata(unsigned char req)
{
	PP_checkCertSt.CertUpdataReq = req;
}

/******************************************************
*��������PP_CertDL_CertDLReset

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
void PP_CertDL_CertDLReset(void)
{
	//PP_CertDL.state.CertValid = 0;
	//PP_CertDL.Cnt = 0;
	//(void)cfg_set_para(CFG_ITEM_HOZON_TSP_CERT_VALID,&PP_CertDL.state.CertValid,1);
}

/******************************************************
*��������GetPP_CertDL_CertValid

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
unsigned char GetPP_CertDL_CertValid(void)
{
	return PP_CertDL.state.CertValid;
}

/******************************************************
*��������GetPP_CertDL_allowBDLink

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
unsigned char GetPP_CertDL_allowBDLink(void)
{
	uint8_t allowSt = 0;
	if((1 == PP_CertDL.state.CertValid) && \
			(0 == PP_CertDL_do_checkCertStatus()))
	{
		allowSt = 1;
	}

	return allowSt;
}
/******************************************************
*��������GetPP_CertDL_CertUpdate

*��  �Σ�

*����ֵ��

*��  ��������

*��  ע��
******************************************************/
unsigned char GetPP_CertDL_CertUpdate(void)
{
	uint8_t St = 0;
	if((PP_CertUpdata.updatedFlag) && \
			(PP_checkCertSt.updataSt == PP_CERTUPDATA_IDLE))
	{
		PP_CertUpdata.updatedFlag = 0;
		St = 1;
	}

	return St;
}

/*
 * 转码
 */
static int PP_CertDL_base64_decode( unsigned char *dst, unsigned int *dlen, \
                     const unsigned char *src, int  slen )
{
    int i, j, n;
    unsigned long x;
    unsigned char *p;

    for( i = j = n = 0; i < slen; i++ )
    {
        if( ( slen - i ) >= 2 &&
           src[i] == '\r' && src[i + 1] == '\n' )
            continue;

        if( src[i] == '\n' )
            continue;

        if( src[i] == '=' && ++j > 2 )
            return( ERR_BASE64_INVALID_CHARACTER );

        if( src[i] > 127 || base64_dec_map[src[i]] == 127 )
            return( ERR_BASE64_INVALID_CHARACTER );

        if( base64_dec_map[src[i]] < 64 && j != 0 )
            return( ERR_BASE64_INVALID_CHARACTER );

        n++;
    }

    if( n == 0 )
        return( 0 );

    n = ((n * 6) + 7) >> 3;

    if( *dlen < n )
    {
        *dlen = n;
        return( ERR_BASE64_BUFFER_TOO_SMALL );
    }

    for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ )
    {
        if( *src == '\r' || *src == '\n' )
            continue;

        j -= ( base64_dec_map[*src] == 64 );
        x  = (x << 6) | ( base64_dec_map[*src] & 0x3F );

        if( ++n == 4 )
        {
            n = 0;
            if( j > 0 ) *p++ = (unsigned char)( x >> 16 );
            if( j > 1 ) *p++ = (unsigned char)( x >>  8 );
            if( j > 2 ) *p++ = (unsigned char)( x       );
        }
    }

    *dlen = (int)(p - dst);

    return( 0 );
}

static int  PP_CertDL_checkCipherCsr(void)
{
	int iRet = 0;
	/*sn_sim_encinfo.txt*/
	int datalen = 0;
	//system("rm /usrdata/pki/sn_sim_encinfo.txt");

	if (dir_exists("/usrdata/pki/") == 0 &&
	        dir_make_path("/usrdata/pki/", S_IRUSR | S_IWUSR, false) != 0)
	{
        log_e(LOG_HOZON, "open cache path fail, reset all index");
        return -1;
	}
	log_i(LOG_HOZON, "/usrdata/pki/ exist or create seccess\n");

	if(!PP_CertDL.state.cipherexist)
	{
		FILE *fp;
		if((fp = fopen("/usrdata/pki/sn_sim_encinfo.txt", "r")) == NULL)//检查密文文件不存在
		{
			iRet = HzTboxSnSimEncInfo(PP_CertDL_SN,PP_CertDL_ICCID,"/usrdata/pem/aeskey.txt", \
					"/usrdata/pki/sn_sim_encinfo.txt", &datalen);
			if(iRet != 3520)
			{
				log_e(LOG_HOZON,"HzTboxSnSimEncInfo error+++++++++++++++iRet[%d] \n", iRet);
				//fclose(fp);
				return -1;
			}
			log_i(LOG_HOZON,"------------------tbox_ciphers_info--------------------%d\n", datalen);
		}

		fclose(fp);
		PP_CertDL.state.cipherexist = 1;
	}
	/******HzTboxGenCertCsr *******/
    //C = cn, ST = shanghai, L = shanghai, O = hezhong, OU = hezhong,
	//CN = hu_client, emailAddress = sm2_hu_client@160.com
    //文件名：
    //格式: PEM / DER or 两个格式同时生成
	if(gb32960_vinValidity())
	{
		CAR_INFO car_information;
		char vin[18] = {0};
		gb32960_getvin(vin);
		car_information.tty_type="TBOX";
		car_information.unique_id= vin;
		car_information.carowner_acct="18221802221";
		car_information.impower_acct="12900100101";
		int iret = HzTboxGenCertCsr("SM2", NULL, &car_information, \
				"CN|shanghai|shanghai|hezhong|hezhong|two_certreqmain|sm2_ecc_two_certreqmain@160.com", \
				"/usrdata/pki","two_certreqmain", "PEM");
		if(iret != 3180)
		{
			log_e(LOG_HOZON,"HzTboxGenCertCsr error ---------------++++++++++++++.[%d]\n", iret);
			return -1;
		}
	}
	else
	{
		log_e(LOG_HOZON,"vin invalid\n");
		return -1;
	}

	return 0;
}
/*
 *验证证书
 */
static int  MatchCertVerify(void)
{
	char	 OnePath[128]="\0";
	char	 ScdPath[128]="\0";
	int 	 iRet;

	sprintf(OnePath, "%s","/usrdata/pem/HozonCA.cer");
	sprintf(ScdPath, "%s","/usrdata/pem/TerminalCA.cer");

	iRet = SgHzTboxCertchainCfg(OnePath, ScdPath);
	if(iRet != 1030)
	{
		log_e(LOG_HOZON,"SgHzTboxCertchainCfg error+++++++++++++++iRet[%d] \n", iRet);
		return -1;
	}

	iRet = HzTboxMatchCertVerify(PP_CERTDL_CERTPATH_UPDATE);
	if(iRet != 5110)
	{
		log_e(LOG_HOZON,"HzTboxMatchCertVerify error+++++++++++++++iRet[%d] \n", iRet);
		return -1;
	}

	if(PP_checkCertSt.CertUpdataSt == 1)//更新证书
	{
		file_copy(PP_CERTDL_CERTPATH,PP_CERTDL_CERTPATH_BKUP);//备份旧证书
		file_copy(PP_CERTDL_CERTPATH_UPDATE,PP_CERTDL_CERTPATH);//替换旧证书

		file_copy(PP_CERTDL_TWOCERTKEYPATH,PP_CERTDL_TWOCERTRKEYPATH_BKUP);//备份旧two_certreqmain.key
		file_copy(PP_CERTDL_TWOCERTRKEYPATH_UPDATE,PP_CERTDL_TWOCERTKEYPATH);//替换旧two_certreqmain.key

		file_copy(PP_CERTDL_TWOCERTCSRPATH,PP_CERTDL_TWOCERTRCSRPATH_BKUP);//备份旧two_certreqmain.csr
		file_copy(PP_CERTDL_TWOCERTRCSRPATH_UPDATE,PP_CERTDL_TWOCERTCSRPATH);//替换旧two_certreqmain.csr

	}
	else
	{
		file_copy(PP_CERTDL_CERTPATH_UPDATE,PP_CERTDL_CERTPATH);//拷贝证书到用户路径
	}

    return 0;
}

static int PP_CertDL_getIdleNode(void)
{
	int i;
	int res = 0;
	for(i = 0;i < PP_CERT_DL_TXINFORMNODE;i++)
	{
		if(CertDL_TxInform[i].idleflag == 0)
		{
			CertDL_TxInform[i].idleflag = 1;
			res = i;
			break;
		}
	}
	return res;
}

static int PP_CertDL_getCertSn(void)
{
	int iRet;
	memset(PP_CertEn.para.certSn,0,sizeof(PP_CertEn.para.certSn));
	iRet = HzTboxGetserialNumber(PP_CERTDL_CERTPATH,"DER",PP_CertEn.para.certSn);//DER
	if(iRet != 6107)
	{
		log_e(LOG_HOZON,"HzTboxGetserialNumber error+++++++++++++++iRet[%d] \n", iRet);
		return -1;
	}
	else
	{
		PP_CertEn.para.certSnLength = strlen(PP_CertEn.para.certSn);
		log_i(LOG_HOZON,"PP_CertEn.para.certSnLength = %d ",PP_CertEn.para.certSnLength);
		log_i(LOG_HOZON,"PP_CertEn.para.certSn = %s\n", PP_CertEn.para.certSn);
	}
	return 0;
}
