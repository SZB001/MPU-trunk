/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "HOZON"
 * 	found in "HOZON_PRIV_v1.0.asn"
 * 	`asn1c -gen-PER`
 */

#ifndef	_Bodyinfo_H_
#define	_Bodyinfo_H_


#include <asn_application.h>

/* Including external dependencies */
#include <OCTET_STRING.h>
#include <NativeInteger.h>
#include <BOOLEAN.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bodyinfo */
typedef struct Bodyinfo {
	OCTET_STRING_t	 aID;
	long	 mID;
	long	 eventTime;
	long	*eventId	/* OPTIONAL */;
	long	*ulMsgCnt	/* OPTIONAL */;
	long	*dlMsgCnt	/* OPTIONAL */;
	long	*msgCntAcked	/* OPTIONAL */;
	BOOLEAN_t	*ackReq	/* OPTIONAL */;
	long	*appDataLen	/* OPTIONAL */;
	long	*appDataEncode	/* OPTIONAL */;
	long	*appDataProVer	/* OPTIONAL */;
	long	*testFlag	/* OPTIONAL */;
	long	*result	/* OPTIONAL */;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Bodyinfo_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Bodyinfo;

#ifdef __cplusplus
}
#endif

#endif	/* _Bodyinfo_H_ */
#include <asn_internal.h>