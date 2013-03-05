/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "ULP-Components"
 * 	found in "supl.asn"
 * 	`asn1c -gen-PER -fnative-types`
 */

#ifndef	_Horveluncert_H_
#define	_Horveluncert_H_


#include <asn_application.h>

/* Including external dependencies */
#include <BIT_STRING.h>
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Horveluncert */
typedef struct Horveluncert {
	BIT_STRING_t	 bearing;
	BIT_STRING_t	 horspeed;
	BIT_STRING_t	 uncertspeed;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Horveluncert_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Horveluncert;

#ifdef __cplusplus
}
#endif

#endif	/* _Horveluncert_H_ */
#include <asn_internal.h>
