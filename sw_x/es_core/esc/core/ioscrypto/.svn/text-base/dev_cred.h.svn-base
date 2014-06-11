#ifndef __DEV_CRED_H__
#define __DEV_CRED_H__

#include "km_types.h"

#define SDC_VERSION	1
// network byte order
// all fields aligned at 4-byte boundary
#pragma pack(push, 4)
struct dev_cred_secret_s {
	u16	format_version;
	u8	_pad1[2];
	u8	k_plat[16];
	u8	k_ghvsym[16];
	u8	k_sym[16];
	u8	k_priv[30];
	u8	_pad2[2];
	u8	rand[64];
    	u8	hash_clear[32];
    	u8	hash_secret[32];
};
typedef struct dev_cred_secret_s dev_cred_secret;

struct dev_cred_clear_s {
	u16	format_version;
	u8	_pad1[2];

	u32	device_id;
	u32	issue_date;
	u32 	serial_number;

	u8	k_pub[60];
	u8	dev_cert[384];
	u8	root_name[64];
	u32	root_kpub_type;
};
typedef struct dev_cred_clear_s dev_cred_clear;

struct dev_cred_clear_rsa2048_s {
	dev_cred_clear		hdr;
	u8			root_kpub[256];
	u32			root_kpub_exp;
	u8			cert_chain[0];
};
typedef struct dev_cred_clear_rsa2048_s dev_cred_clear_rsa2048;

struct dev_cred_clear_rsa4096_s {
	dev_cred_clear		hdr;
	u8			root_kpub[512];
	u32			root_kpub_exp;
	u8			cert_chain[0];
};
typedef struct dev_cred_clear_rsa4096_s dev_cred_clear_rsa4096;

struct dev_cred_default_s {
	u16	format_version;
	u16 	pad;
	u32	root_kpub_type;
	u32	root_kpub_exp;
	u8	k_plat[16];
};
typedef struct dev_cred_default_s dev_cred_default;

struct dev_cred_default_rsa2048_s {
	dev_cred_default	hdr;
	u8			root_kpub[256];
};
typedef struct dev_cred_default_rsa2048_s dev_cred_default_rsa2048;

struct dev_cred_default_rsa4096_s {
	dev_cred_default	hdr;
	u8			root_kpub[512];
};
typedef struct dev_cred_default_rsa4096_s dev_cred_default_rsa4096;
#pragma pack(pop)

#endif // __DEV_CRED_H__
