/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "RRLP-Components"
 * 	found in "rrlp-components.asn"
 * 	`asn1c -gen-PER -fnative-types`
 */

#ifndef	_LocErrorReason_H_
#define	_LocErrorReason_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum LocErrorReason {
	LocErrorReason_unDefined	= 0,
	LocErrorReason_notEnoughBTSs	= 1,
	LocErrorReason_notEnoughSats	= 2,
	LocErrorReason_eotdLocCalAssDataMissing	= 3,
	LocErrorReason_eotdAssDataMissing	= 4,
	LocErrorReason_gpsLocCalAssDataMissing	= 5,
	LocErrorReason_gpsAssDataMissing	= 6,
	LocErrorReason_methodNotSupported	= 7,
	LocErrorReason_notProcessed	= 8,
	LocErrorReason_refBTSForGPSNotServingBTS	= 9,
	LocErrorReason_refBTSForEOTDNotServingBTS	= 10
	/*
	 * Enumeration is extensible
	 */
} e_LocErrorReason;

/* LocErrorReason */
typedef long	 LocErrorReason_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_LocErrorReason;
asn_struct_free_f LocErrorReason_free;
asn_struct_print_f LocErrorReason_print;
asn_constr_check_f LocErrorReason_constraint;
ber_type_decoder_f LocErrorReason_decode_ber;
der_type_encoder_f LocErrorReason_encode_der;
xer_type_decoder_f LocErrorReason_decode_xer;
xer_type_encoder_f LocErrorReason_encode_xer;
per_type_decoder_f LocErrorReason_decode_uper;
per_type_encoder_f LocErrorReason_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _LocErrorReason_H_ */
#include <asn_internal.h>
