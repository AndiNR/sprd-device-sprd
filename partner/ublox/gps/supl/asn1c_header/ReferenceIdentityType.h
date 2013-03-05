/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "RRLP-Components"
 * 	found in "rrlp-components.asn"
 * 	`asn1c -gen-PER -fnative-types`
 */

#ifndef	_ReferenceIdentityType_H_
#define	_ReferenceIdentityType_H_


#include <asn_application.h>

/* Including external dependencies */
#include "BSICAndCarrier.h"
#include "CellID.h"
#include "RequestIndex.h"
#include "SystemInfoIndex.h"
#include "CellIDAndLAC.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum ReferenceIdentityType_PR {
	ReferenceIdentityType_PR_NOTHING,	/* No components present */
	ReferenceIdentityType_PR_bsicAndCarrier,
	ReferenceIdentityType_PR_ci,
	ReferenceIdentityType_PR_requestIndex,
	ReferenceIdentityType_PR_systemInfoIndex,
	ReferenceIdentityType_PR_ciAndLAC
} ReferenceIdentityType_PR;

/* ReferenceIdentityType */
typedef struct ReferenceIdentityType {
	ReferenceIdentityType_PR present;
	union ReferenceIdentityType_u {
		BSICAndCarrier_t	 bsicAndCarrier;
		CellID_t	 ci;
		RequestIndex_t	 requestIndex;
		SystemInfoIndex_t	 systemInfoIndex;
		CellIDAndLAC_t	 ciAndLAC;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} ReferenceIdentityType_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_ReferenceIdentityType;

#ifdef __cplusplus
}
#endif

#endif	/* _ReferenceIdentityType_H_ */
#include <asn_internal.h>
