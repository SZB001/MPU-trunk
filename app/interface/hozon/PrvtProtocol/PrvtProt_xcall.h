/******************************************************
�ļ�����	PrvtProt_xcall.h

������	��ҵ˽��Э�飨�㽭���ڣ�	

Data			  Vasion			author
2019/04/16		   V1.0			    liujian
*******************************************************/
#ifndef		_PRVTPROT_XCALL_H
#define		_PRVTPROT_XCALL_H
/*******************************************************
description�� include the header file
*******************************************************/

/*******************************************************
description�� macro definitions
*******************************************************/
/**********�꿪�ض���*********/

/**********�곣������*********/
#define PP_XCALL_ACK_WAIT 		0x01//Ӧ��ɹ�
#define PP_XCALL_ACK_SUCCESS 	0x02//Ӧ��ɹ�


#define	PP_ECALL_TYPE 	2//ecall
#define PP_BCALL_TYPE	1//bcall
#define	PP_ICALL_TYPE	3//icall
/***********�꺯��***********/

/*******************************************************
description�� struct definitions
*******************************************************/

/*******************************************************
description�� typedef definitions
*******************************************************/
/******enum definitions******/

typedef enum
{
	PP_BCALL = 0,//
    PP_ECALL,//
	PP_ICALL,
	PP_detection,
	PP_XCALL_MAX
} PP_Xcall_INDEX;

/*****struct definitions*****/

typedef struct
{
	uint8_t req;/* ����:box to tsp */
	uint8_t resp;/* ��Ӧ:box to tsp */
	uint8_t retrans;/* retransmission */
	uint8_t waitSt;
	uint64_t waittime;
}__attribute__((packed))  PrvtProt_xcallSt_t; /*xcall�ṹ��*/

/* application data struct */
/***********************************
			Xcall
***********************************/
typedef struct
{
	int  gpsSt;//gps״̬ 0-��Ч��1-��Ч
	long gpsTimestamp;//gpsʱ���
	long latitude;//γ�� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long longitude;//���� x 1000000,��GPS�ź���Чʱ��ֵΪ0
	long altitude;//�߶ȣ�m��
	long heading;//��ͷ����Ƕȣ�0Ϊ��������
	long gpsSpeed;//�ٶ� x 10����λkm/h
	long hdop;//ˮƽ�������� x 10
}PrvtProt_Rvsposition_t;

typedef struct
{
	long xcallType;//����  1-��·��Ԯ   2-������Ԯ��ecall��  3-400�绰����
	long engineSt;//����״̬��1-Ϩ��2-����
	long totalOdoMr;//�����Ч��Χ��0 - 1000000��km��
	PrvtProt_Rvsposition_t gpsPos;//������Ԯλ��
	long srsSt;//��ȫ����״̬ 1- ������2 - ����
	long updataTime;//����ʱ���
	long battSOCEx;//�������ʣ�������0-10000��0%-100%��
}PrvtProt_App_Xcall_t;
/******union definitions*****/

/*******************************************************
description�� variable External declaration
*******************************************************/

/*******************************************************
description�� function External declaration
*******************************************************/
extern void PP_xcall_init(void);
extern int PP_xcall_mainfunction(void *task);
extern void PP_xcall_SetXcallReq(unsigned char req);
//extern void PP_rmtCtrl_SetEcallResp(unsigned char resp);

#endif 
