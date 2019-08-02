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

#include "init.h"
#include "log.h"
#include "list.h"
#include "../sockproxy/sockproxy_txdata.h"
#include "../../support/protocol.h"
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
}__attribute__((packed))  PP_CertDL_t;

static PP_CertDL_t 				PP_CertDL;
static PP_CertDownloadPara_t	PP_CertDownloadPara;//需要掉电保存的参数
static char	PP_CertDL_SN[19];
static char	PP_CertDL_ICCID[21];

static PrvtProt_TxInform_t CertDL_TxInform;

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
static int PP_CertDL_CertDLReq(PrvtProt_task_t *task,PP_CertificateDownload_t *CertificateDownload);
static int  PP_CertDL_checkCipherCsr(void);
static int  MatchCertVerify(void);
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
	memset(&PP_CertDL,0 , sizeof(PP_CertDL_t));
	memcpy(PP_CertDL.pack.Header.sign,"**",2);
	PP_CertDL.pack.Header.commtype.Byte = 0xe1;
	PP_CertDL.pack.Header.ver.Byte = 0x30;
	PP_CertDL.pack.Header.opera = 0x05;
	PP_CertDL.state.dlSt = PP_CERTDL_IDLE;

	PP_CertDownloadPara.eventid = 0;

	PP_CertDL.state.certDLTestflag = 1;

	FILE *fp;
	if((fp=fopen("/usrdata/pki/auth.cer", "r")) != NULL)//检查证书文件是否存在
	{
		PP_CertDL.state.CertValid = 1;
		fclose(fp);
	}
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

	res = 		PP_CertDL_do_checksock(task_ptr) ||
				PP_CertDL_do_rcvMsg(task_ptr) ||
				PP_CertDL_do_wait(task_ptr) ||
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
	if(1 == sockproxy_sgsocketState())//socket open
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
	protocol_dump(LOG_HOZON, "PRVT_PROT", rcv_pack.Header.sign, rlen, 0);
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
			if(rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE] == 1)//成功  && tbox证书
			{
				PP_CertDL.state.dlsuccess = PP_CERTDL_SUCCESS;
				PP_CertDL.para.CertDLResp.certLength = 0;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 24;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 16;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN] << 8;
				PP_CertDL.para.CertDLResp.certLength |= (uint32_t)rxPack->msgdata[PP_CERTDL_RESP_CERTLEN];

				log_i(LOG_HOZON, "PP_CertDL.para.CertDLResp.certLength = %d\n",PP_CertDL.para.CertDLResp.certLength);
				log_i(LOG_HOZON, "certContent = %s\n",rxPack->msgdata[PP_CERTDL_RESP_CERTCONTENT]);


				//保存证书内容
#if 0
				FILE *pf = fopen("/usrdata/pki/userAuth.cer","w");
				if(pf == NULL)
				{
					printf("open a.txt error\n");
					fclose(pf);
					exit(0);
				}

				unsigned char ucCertData[2048] = {0};     // CA机构颁发的证书
		        unsigned int uiCertDataLen = sizeof(ucCertData);
				ks_base64_decode(ucCertData, &uiCertDataLen,recvBuffer+sizeof(MSG)+sizeof(CertificateDownloadRes),myres.crelen);

				fwrite(ucCertData,uiCertDataLen,1,pf);
				fclose(pf);
#endif
				if(0 == MatchCertVerify())//验证证书
				{
					PrvtPro_SettboxId((unsigned int)rxPack->Header.tboxid);
					PP_CertDL.state.CertValid = 1;
				}

				//PrvtPro_SettboxId((unsigned int)rxPack->Header.tboxid);
			}
			else
			{
				log_i(LOG_HOZON, "rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE] = %d\n",rxPack->msgdata[PP_CERTDL_RESP_CERTTYPE]);
				PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
			}
		}
		else
		{
			log_i(LOG_HOZON, "rxPack->msgdata[PP_CERTDL_RESP_RESULT] = %d\n",rxPack->msgdata[PP_CERTDL_RESP_RESULT]);
			PP_CertDL.state.dlsuccess = PP_CERTDL_FAIL;
		}
	}
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
	switch(PP_CertDL.state.dlSt)
	{
		case PP_CERTDL_IDLE:
		{
			if((1 != PP_CertDL.state.CertValid) && \
					(PP_CertDL.state.certDLTestflag))//证书文件不存在
			{
				//PP_CertDL.state.certDLTestflag = 1;
				PP_CertDL.state.waittime = tm_get_time();
				PP_CertDL.state.dlSt = PP_CERTDL_CHECK_CIPHER_CSR;
			}
		}
		break;
		case PP_CERTDL_CHECK_CIPHER_CSR:
		{
			if((tm_get_time() - PP_CertDL.state.waittime) <= 30000)
			{
				if((PP_rmtCfg_getIccid((uint8_t*)PP_CertDL_ICCID)) && \
						((tm_get_time() - PP_CertDL.state.waittime) >= 10000))
				{
					PrvtProt_gettboxsn(PP_CertDL_SN);
					if(PP_CertDL_checkCipherCsr() == 0)
					{
						PP_CertDL.state.dlSt = PP_CERTDL_DLREQ;
					}
					else
					{
						log_i(LOG_HOZON, "check cipher fail\n");
						PP_CertDL.state.dlSt = PP_CERTDL_END;
					}
				}
			}
			else
			{
				log_i(LOG_HOZON, "iccid read timeout\n");
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
			iRet = HzTboxApplicationData("/usrdata/pki/two_certreqmain.csr" , "/usrdata/pki/sn_sim_encinfo.txt", gcsroutdata, &gcoutlen);
			if(iRet != 3630)
			{
				log_i(LOG_HOZON,"HzTboxApplicationData error+++++++++++++++iRet[%d] \n", iRet);
				PP_CertDL.state.dlSt = PP_CERTDL_END;
				return -1;
			}
		    char vin[18] = {0};
		    gb32960_getvin(vin);
			PP_CertDL.para.CertDLReq.infoListLength = 19 + gcoutlen;//19 == vin + "&&"
			memcpy(PP_CertDL.para.CertDLReq.infoList,vin,17);
			memcpy(PP_CertDL.para.CertDLReq.infoList+17,"&&",2);
			memcpy(PP_CertDL.para.CertDLReq.infoList+19,gcsroutdata,gcoutlen);
			PP_CertDL_CertDLReq(task,&PP_CertDL.para);
			PP_CertDL.state.dlsuccess = PP_CERTDL_INITVAL;
			PP_CertDL.state.waittime = tm_get_time();
			PP_CertDL.state.dlSt = PP_CERTDL_DLREQWAIT;
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
				log_i(LOG_HOZON, "auth download timeout\n");
				PP_CertDL.state.dlSt = PP_CERTDL_END;
			}
		}
		break;
		case PP_CERTDL_END:
		{
			PP_CertDL.state.dlSt = PP_CERTDL_IDLE;
		}
		break;
		default:
		break;
	}

	return 0;
}

/******************************************************
*��������PP_CertDL_send_cb

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

	memset(&CertDL_TxInform,0,sizeof(PrvtProt_TxInform_t));
	CertDL_TxInform.mid = CertificateDownload->CertDLReq.mid;
	CertDL_TxInform.eventtime = tm_get_time();
	CertDL_TxInform.pakgtype = PP_TXPAKG_SIGTIME;

	SP_data_write(PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,PP_CertDL_send_cb,&CertDL_TxInform);
	protocol_dump(LOG_HOZON, "CertDownload_request", PP_CertDL_pack.Header.sign,PP_CertDL_pack.totallen,1);

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
	PP_CertDL.state.CertValid 	   = 0;
	PP_CertDL.state.certDLTestflag = req;
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

/*
 * 转码
 */
int PP_CertDL_base64_decode( unsigned char *dst, int *dlen, \
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
        //ft_index_r = ft_index_w = 0;
        return 0;
	}
	log_i(LOG_HOZON, "/usrdata/pki/ exist or create seccess\n");

	FILE *fp;
	if((fp = fopen("/usrdata/pki/sn_sim_encinfo.txt", "r")) == NULL)//检查密文文件不存在
	{
		iRet = HzTboxSnSimEncInfo(PP_CertDL_SN,PP_CertDL_ICCID,"/usrdata/pem/aeskey.txt", \
				"/usrdata/pki/sn_sim_encinfo.txt", &datalen);
		if(iRet != 3520)
		{
			log_i(LOG_HOZON,"HzTboxSnSimEncInfo error+++++++++++++++iRet[%d] \n", iRet);
			return -1;
		}
		log_i(LOG_HOZON,"------------------tbox_ciphers_info--------------------%d\n", datalen);
	}
	else
	{
		fclose(fp);
	}

	/******HzTboxGenCertCsr *******/
    //C = cn, ST = shanghai, L = shanghai, O = hezhong, OU = hezhong,
	//CN = hu_client, emailAddress = sm2_hu_client@160.com
    //文件名：
    //格式: PEM / DER or 两个格式同时生成
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
    	log_i(LOG_HOZON,"HzTboxGenCertCsr error ---------------++++++++++++++.[%d]\n", iret);
		return -1;
    }
    //printf("HzTboxGenCertCsr ---------------++++++++++++++.[%d]\n", iret);

	return 0;
}
/*
 *验证证书
 */
static int  MatchCertVerify(void)
{
	char	 OnePath[128]="\0";
	char	 ScdPath[128]="\0";
	int iRet;

	sprintf(OnePath, "%s","/usrdata/pem/HozonCA.cer");
	sprintf(ScdPath, "%s","/usrdata/pem/TerminalCA.cer");

	iRet = SgHzTboxCertchainCfg(OnePath, ScdPath);
	if(iRet != 1030)
	{
		log_i(LOG_HOZON,"SgHzTboxCertchainCfg error+++++++++++++++iRet[%d] \n", iRet);
		return -1;
	}

	iRet=HzTboxMatchCertVerify("/usrdata/pki/userAuth.cer");
	if(iRet != 5110)
	{
		log_i(LOG_HOZON,"HzTboxMatchCertVerify error+++++++++++++++iRet[%d] \n", iRet);
		return -1;
	}

    return 0;
}
