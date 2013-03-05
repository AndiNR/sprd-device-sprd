/*
 * Generated by asn1c-0.9.22 (http://lionet.info/asn1c)
 * From ASN.1 module "SUPL-END"
 * 	found in "supl.asn"
 * 	`asn1c -gen-PER -fnative-types`
 */

#include "SUPLEND.h"

static asn_TYPE_member_t asn_MBR_SUPLEND_1[] = {
	{ ATF_POINTER, 3, offsetof(struct SUPLEND, position),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Position,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"position"
		},
	{ ATF_POINTER, 2, offsetof(struct SUPLEND, statusCode),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_StatusCode,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"statusCode"
		},
	{ ATF_POINTER, 1, offsetof(struct SUPLEND, ver),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Ver,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"ver"
		},
};
static int asn_MAP_SUPLEND_oms_1[] = { 0, 1, 2 };
static ber_tlv_tag_t asn_DEF_SUPLEND_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_TYPE_tag2member_t asn_MAP_SUPLEND_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* position at 258 */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* statusCode at 259 */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 } /* ver at 260 */
};
static asn_SEQUENCE_specifics_t asn_SPC_SUPLEND_specs_1 = {
	sizeof(struct SUPLEND),
	offsetof(struct SUPLEND, _asn_ctx),
	asn_MAP_SUPLEND_tag2el_1,
	3,	/* Count of tags in the map */
	asn_MAP_SUPLEND_oms_1,	/* Optional members */
	3, 0,	/* Root/Additions */
	2,	/* Start extensions */
	4	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_SUPLEND = {
	"SUPLEND",
	"SUPLEND",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	SEQUENCE_decode_uper,
	SEQUENCE_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_SUPLEND_tags_1,
	sizeof(asn_DEF_SUPLEND_tags_1)
		/sizeof(asn_DEF_SUPLEND_tags_1[0]), /* 1 */
	asn_DEF_SUPLEND_tags_1,	/* Same as above */
	sizeof(asn_DEF_SUPLEND_tags_1)
		/sizeof(asn_DEF_SUPLEND_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_SUPLEND_1,
	3,	/* Elements count */
	&asn_SPC_SUPLEND_specs_1	/* Additional specs */
};

