#include "ec_pers.h"
#include "dev_cred.h"
#include "core_glue.h"
#include "ec_log.h"

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stddef.h>
#endif

#include "aes.h"
#include "sha1.h"
#include "sha256.h"

#define SWAP16(x)	((((x) >> 8) & 0xff) | (((x) << 8) & 0xff00))

#ifdef _MSC_VER
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

// network byte order
struct secret_device_credentials_IV {
	u32	device_id;
	u32	serial_number;
	u32	_pad[2];      	// zeros
};

// XXX
// development known answer
static u8 _a_1_4[] = {
	0x1f, 0xcf, 0xcd, 0x08, 0xa7, 0xe9, 0x19, 0x57,
	0x99, 0x33, 0xaa, 0x78, 0xb4, 0x42, 0x02, 0x6b,
};

static int	
pers_aes_decrypt(u8 *key, u8 *iv, u8 *data_in, u32 bytes, u8 *data_out)
{
	u8 saved_iv_data[16];
	bool use_saved_iv = false;

	/*
	* When decrypting "in-place", the iv for the next block must saved
	* before the decrypt operation.
	*/
	if (data_in == data_out) {
		memcpy(saved_iv_data, data_in + bytes - 16, 16);
		use_saved_iv = true;
	}

	if (aes_SwDecrypt(key, iv, data_in, bytes, data_out) == -1) {
		return -1;
	}

	if (use_saved_iv) {
		memcpy(iv, saved_iv_data, 16);
	} else {
		memcpy(iv, data_in + bytes - 16, 16);
	}

	return 0;
}

static int
pers_compute_sha256(void *ctxt, u8 *in, u32 in_len, bool is_first, bool is_last,
		void *hash)
{
	if (is_first) {
		SHA256Reset((SHA256Context *)ctxt);
	}

	if ( in_len ) {
		SHA256Input((SHA256Context *)ctxt, in, in_len);
	}

	if ( is_last ) {
		SHA256Result((SHA256Context *)ctxt, hash);
	}

	return 0;
}

static int
_ms_decrypt(dev_cred_secret *ms, u8 *key, dev_cred_clear *dcc, u32 dcc_len)
{
	struct secret_device_credentials_IV ms_iv;
	SHA256Context ctx;
	u8 hash[32];
	u32 len;

	// Verify the version of the master secret
	if ( ms->format_version != SWAP16(SDC_VERSION)) {
		LOG_ERROR("Unknown master secret format - %d", ms->format_version);
		return -1;
	}

	// Verify the version of the clear device credential
	if ( dcc->format_version != SWAP16(SDC_VERSION)) {
		LOG_ERROR("Unknown clear dev cred format - %d", dcc->format_version);
		return -1;
	}

	// set up the IV
	memset(&ms_iv, 0, sizeof(ms_iv));
	ms_iv.device_id = dcc->device_id;
	ms_iv.serial_number = dcc->serial_number;

	// Decrypt the master secret
	len = offsetof(dev_cred_secret, hash_clear) -
		offsetof(dev_cred_secret, k_plat);

	// dbg_bytes_dump("master secret[enc]", ms->k_plat, len);

	if ( pers_aes_decrypt(key, (u8 *)&ms_iv, ms->k_plat, len,
			      ms->k_plat) ) {
		LOG_ERROR("ms decrypt failed");
		return -1;
	}

	//dbg_bytes_dump("master secret[dec]", ms->k_plat, len);

	// Verify the hash of the master secret
	if ( pers_compute_sha256(&ctx, (u8 *)ms,
				 sizeof(*ms) - sizeof(ms->hash_secret),
				 true, true, hash) ) {
		LOG_ERROR("Failed computing hash of master secret");
		return -1;
	}

	if ( memcmp(hash, ms->hash_secret, sizeof(hash)) ) {
		LOG_ERROR("Master secret hash check failed!");
#if 0
		dbg_bytes_dump("master hash:", hash, sizeof(hash));
		dbg_bytes_dump("expected hash:", ms->hash_secret, sizeof(hash));
#endif // __HC_DEBUG
		return -1;
	}

	// Verify the hash of the clear credentials
	if ( pers_compute_sha256(&ctx, (u8 *)dcc, dcc_len, true, true, hash) ) {
		LOG_ERROR("Failed computing hash of clear dev cred");
		return -1;
	}

	if ( memcmp(hash, ms->hash_clear, sizeof(hash)) ) {
		LOG_ERROR("clear dev cred hash check failed!");
		//dbg_bytes_dump("clear dev hash:", hash, sizeof(hash));
		//dbg_bytes_dump("expected hash:", ms->hash_clear, sizeof(hash));
		return -1;
	}

	return 0;
}

// decrypt encrypted secret credential blob in-place
int
esc_decrypt_cred(u8 *edc_blob, u32 edc_blob_len,
		 u8 *cdc_blob, u32 cdc_blob_len)
{
	int err = -1;

	if ( sizeof(dev_cred_secret) != edc_blob_len ) {
		LOG_ERROR("Size mismatch for dev_cred_secret %d vs %d",
		    (s32)sizeof(dev_cred_secret), (s32)edc_blob_len);
		goto fail;
	}

	if ( sizeof(dev_cred_clear) > cdc_blob_len ) {
		LOG_ERROR("Size mismatch for dev_cred_clear %d vs %d",
		    (s32)sizeof(dev_cred_clear), (s32)cdc_blob_len);
		goto fail;
	}

	// decrypt the master secret in place
	if ( _ms_decrypt((dev_cred_secret*)edc_blob, _a_1_4, (dev_cred_clear*)cdc_blob, cdc_blob_len) ) {
		LOG_ERROR("Failed decrypting the master secret");
		goto fail;
	}

	err = 0;

fail:
	return err;
}

// The following lines are needed by Emacs to maintain the format the file started in.
// Local Variables:
// c-basic-offset: 8
// tab-width: 8
// indent-tabs-mode: t
// End:
