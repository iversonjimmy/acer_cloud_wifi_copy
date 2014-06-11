// vim: ts=4:sw=4:expandtab

#define __HC_DEBUG  1
#include <km_types.h>
#include <esc_iosctypes.h>
#include <iosccert.h>
#include <estypes.h>
#include <esi.h>

#include <sha1.h>
#include <sha256.h>
#include <aes.h>
#include <csl.h>

#include "bsl.h"
#include "bsl_defs.h"

//#include "bsd_types.h"
#include "ioslibc.h"
#include "keystore.h"
#include "dev_cred.h"
#include "mt64.h"
#include "build_features.h"
#include "ghv_crypto_impl.h"
#include "crypto_impl.h"
#ifndef _KERNEL_MODULE
#include "string.h"
#endif
#include "ec_log.h"

#ifdef _KERNEL_MODULE
#include "core_glue.h"
#endif

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

#define IOSC_DEVICE_TYPE_VP             6

static int cacrlVer = 0;
static int crlVer   = 0;
static int osVer    = 0;

static u32 deviceId;
static u32 deviceType;

#if defined(GVM_FEAT_PLAT_CMN_KEY)
static IOSCSecretKeyHandle _plat_cmn_key;
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)

// These can't be static...
keyMetaElem keyMeta[KEY_META_MAX_SIZE];
keyStoreElem keyStore[KEY_STORE_MAX_SIZE];

static bool _initialized = false;
static bool _cred_loaded = false;

static dev_cred_default *_dc_default;
static dev_cred_secret *_dc_secret;
static dev_cred_clear *_dc_clear;
static u32 _dc_default_len;
static u32 _dc_clear_len;

static u8 *_rootKey;
static u32 _rootExp;

#ifndef MIN
#define MIN(a,b)        (((a) < (b)) ? (a) : (b))
#endif

#define SWAP32(x)       ((((x) >> 24) & 0x000000ff) | \
                         (((x) >>  8) & 0x0000ff00) | \
                         (((x) <<  8) & 0x00ff0000) | \
                         (((x) << 24) & 0xff000000))

IOSCError
IOSCryptoEncryptAESCBC(u8 *aesKey, u8 *ivData,
		       u8 *inputData, u32 inputSize,
		       u8 *outputData)
{
    if (aes_SwEncrypt(aesKey, ivData, inputData, inputSize, outputData) == -1){
	return IOSC_ERROR_FAIL_INTERNAL;
    }
    memcpy(ivData, outputData + inputSize  - 16, 16);
    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoDecryptAESCBC(u8 *aesKey, u8 *ivData,
		       u8 *inputData, u32 inputSize,
		       u8 *outputData)
{
    if (aes_SwDecrypt(aesKey, ivData, inputData, inputSize, outputData) == -1){
	return IOSC_ERROR_FAIL_INTERNAL;
    }
    memcpy(ivData, inputData + inputSize - 16, 16);
    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoEncryptAESCBCHwKey(u32 hwKeyId, u8 *ivData,
		       u8 *inputData, u32 inputSize,
		       u8 *outputData)
{
    u8 *aesKey;

    if (hwKeyId == 0) {
        aesKey = _dc_secret->k_sym;
    } else {
        return IOSC_ERROR_INVALID;
    }

    return IOSCryptoEncryptAESCBC(aesKey, ivData, inputData, inputSize, outputData);
}

IOSCError
IOSCryptoDecryptAESCBCHwKey(u32 hwKeyId, u8 *ivData,
		       u8 *inputData, u32 inputSize,
		       u8 *outputData)
{
    u8 *aesKey;

    if (hwKeyId == 0) {
        aesKey = _dc_secret->k_sym;
    } else {
        return IOSC_ERROR_INVALID;
    }

    return IOSCryptoDecryptAESCBC(aesKey, ivData, inputData, inputSize, outputData);
}

IOSCError IOSCryptoComputeSHA1(IOSCHashContext context,
			       u8 *inputData, u32 inputSize,
			       u32 chainingFlag,
			       IOSCHash hashData)
{
    if (chainingFlag == IOSC_HASH_FIRST) {
	SHA1Reset((SHA1Context *)context);
    }

    if (inputSize > 0) {
	SHA1Input((SHA1Context *)context, inputData, inputSize);
    }

    if (chainingFlag == IOSC_HASH_LAST) {
	SHA1Result((SHA1Context *)context, hashData);
    }

    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoComputeSHA256(IOSCHashContext context,
    u8 *inputData, u32 inputSize,
    u32 chainingFlag,
    IOSCHash256 hashData)
{
    // assert(sizeof(IOSCHashContext) >= sizeof(SHA256_CTX));

    if (chainingFlag == IOSC_HASH_FIRST) {
        SHA256Reset((SHA256Context *)context);
    }

    if (inputSize > 0) {
        SHA256Input((SHA256Context *)context, inputData, inputSize);
    }

    if (chainingFlag == IOSC_HASH_LAST) {
        SHA256Result((SHA256Context *)context, hashData);
    }

    return IOSC_ERROR_OK;
}

/*
 * RSA public key decrypt
 *
 * Use software version in CSL
 */
IOSCError
IOSCryptoDecryptRSA(u8 *publicKey, u32 keySize,
    u8 *exponent, u32 expSize,
    u8 *inputData, u8 *outputData)
{
    IOSCError rv = IOSC_ERROR_OK;

    (void) CSL_DecryptRsa(publicKey, keySize, exponent, expSize,
            inputData, outputData);

    return rv;
}

void *IOSCryptoAllocAligned(u32 size, u32 alignment)
{
    // Malloc is 16 byte aligned, good enough for mostly everything
    return malloc(size);
}

void  IOSCryptoFreeAligned(void *p, u32 alignment)
{
    free(p);
}

u8 *IOSCryptoGetRootKey(void)
{
    return _rootKey;
}

u8 *IOSCryptoGetRootKeyExp(void)
{
    return (u8 *)&_rootExp;
}

IOSCError IOSCryptoIncVersion(IOSCDataHandle handle)
{
    switch(handle) {
    case IOSC_CACRLVER_HANDLE:
	cacrlVer++;
	break;
    case IOSC_SIGNERCRLVER_HANDLE:
	crlVer++;
	break;
    case IOSC_BOOTOSVER_HANDLE:
	osVer++;
	break;
    default:
	return IOSC_ERROR_INVALID;
    }
    return IOSC_ERROR_OK;
}

s32 IOSCryptoGetVersion(IOSCDataHandle handle)
{
    switch(handle) {
    case IOSC_CACRLVER_HANDLE:
	return cacrlVer;
    case IOSC_SIGNERCRLVER_HANDLE:
	return crlVer;
    case IOSC_BOOTOSVER_HANDLE:
	return osVer;
    }

    return IOSC_ERROR_INVALID;
}

IOSCError IOSCryptoGenerateRand(u8 *randoms, u32 size)
{
    static u64 cur_val;
    static u32 cur_cnt = 0;
    u32 xfer_cnt;
    u32 cur_ind;
    u8 *cvp;

    cvp = (u8 *)&cur_val;
    cur_ind = sizeof(cur_val) - cur_cnt;
    while (size) {
        // copy what we can from the cur_val
        if ( cur_cnt == 0 ) {
            cur_val = genrand64_int64();
            cur_cnt = sizeof(cur_val);
            cur_ind = 0;
        }
        xfer_cnt = MIN(size, cur_cnt);
        memcpy(randoms, &cvp[cur_ind], xfer_cnt);
        size -= xfer_cnt;
        cur_cnt -= xfer_cnt;
        cur_ind += xfer_cnt;
        randoms += xfer_cnt;
    }

    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoGetDeviceType(u32 *devType)
{
    *devType = deviceType;
    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoGetDeviceCert(u8 *cert)
{
    memcpy(cert, _dc_clear->dev_cert, sizeof(_dc_clear->dev_cert));
    return IOSC_ERROR_OK;
}

IOSCError
IOSCryptoInitialize(void)
{
    return IOSC_ERROR_OK;
}

static void
initDeviceKeys(void)
{
    IOSCEccEccCert *cert = (IOSCEccEccCert *)_dc_clear->dev_cert;
    int i;
    char *bp;
    int val;
    IOSCDeviceId *cert_id;

    deviceId = 0;
    bp = (char *)cert->head.name.deviceId + 2;
    for( i = 0 ; i < 8 ; i++ ) {
        deviceId <<= 4;
        if ( bp[i] >= '0' && bp[i] <= '9' ) {
            val = bp[i] - '0';
        }
        else if ( bp[i] >= 'A' && bp[i] <= 'F' ) {
            val = bp[i] - 'A' + 10;
        }
        else if ( bp[i] >= 'a' && bp[i] <= 'f' ) {
            val = bp[i] - 'a' + 10;
        }
        else {
            // XXX need to assert here or something.
            val = 0;
        }
        deviceId |= val;
    }

    cert_id = &(cert->head.name.deviceId);

    // Fill-in the device type and ID.
    if(strncmp((char*)(*cert_id), "VP", 2) == 0) {
        deviceType = IOSC_DEVICE_TYPE_VP;
    }
}

#define _MAX_CERT_CHAIN     5
static int
_root_key_extract(void)
{
	ESError ee;
    u32 cert_len;
    void *certs[_MAX_CERT_CHAIN];
    void *cert_chain;
    u32 cert_lens[_MAX_CERT_CHAIN];
    u32 cert_cnt;
    dev_cred_clear_rsa4096 *dc4096 = NULL;
    dev_cred_clear_rsa2048 *dc2048 = NULL;

    // figure out what type of cert we have
    switch (SWAP32(_dc_clear->root_kpub_type)) {
    case IOSC_PUBKEY_RSA4096:
        dc4096 = (dev_cred_clear_rsa4096 *)_dc_clear;
        _rootExp = dc4096->root_kpub_exp;
        _rootKey = dc4096->root_kpub;
        cert_chain = dc4096->cert_chain;
        cert_len = _dc_clear_len - offsetof(dev_cred_clear_rsa4096, cert_chain);
        break;

    case IOSC_PUBKEY_RSA2048:
        dc2048 = (dev_cred_clear_rsa2048 *)_dc_clear;
        _rootExp = dc2048->root_kpub_exp;
        _rootKey = dc2048->root_kpub;
        cert_chain = dc2048->cert_chain;
        cert_len = _dc_clear_len - offsetof(dev_cred_clear_rsa2048, cert_chain);
        break;
    default:
        LOG("Unknown root_kpub_type 0x%08x\n",
            SWAP32(_dc_clear->root_kpub_type));
        return -1;
    }

    cert_cnt = _MAX_CERT_CHAIN;
	if ( (ee = ESI_ParseCertList(cert_chain, cert_len, certs, cert_lens,
                                 &cert_cnt)) ) {
		LOG("ESI_ParseCertList for certs failed - %d\n", ee);
        return -1;
	}

    DBLOG_N("Found %d certs", cert_cnt);

    // Verify the chain we have.
    if ( ESI_VerifyCert(_dc_clear->dev_cert, (const void **)certs, cert_cnt) ) {
        LOG("Failed verifying the device cert chains!\n");
        return -1;
    }

#ifdef _MSC_VER
    DBLOG_N("Device cert chain verified.", 0);
#else
    DBLOG_N("Device cert chain verified.");
#endif

    return 0;
}

static int
_root_key_dflt_extract(void)
{
    dev_cred_default_rsa4096 *dc4096 = NULL;
    dev_cred_default_rsa2048 *dc2048 = NULL;

    // figure out what type of cert we have
    switch (SWAP32(_dc_default->root_kpub_type)) {
    case IOSC_PUBKEY_RSA4096:
        dc4096 = (dev_cred_default_rsa4096 *)_dc_default;
        _rootExp = dc4096->hdr.root_kpub_exp;
        _rootKey = dc4096->root_kpub;
        break;

    case IOSC_PUBKEY_RSA2048:
        dc2048 = (dev_cred_default_rsa2048 *)_dc_default;
        _rootExp = dc2048->hdr.root_kpub_exp;
        _rootKey = dc2048->root_kpub;
        break;
    default:
        LOG("Unknown root_kpub_type 0x%08x\n",
            SWAP32(_dc_default->root_kpub_type));
        return -1;
    }

    return 0;
}

/* 
 * Initialization
 */
int
crypto_impl_init(dev_cred_default *cred_default, u32 default_len)
{
    if ( _initialized ) {
        return 0;
    }

#ifdef __KERNEL__
    _dc_default = kmalloc(default_len, GFP_KERNEL);
#else
    _dc_default = malloc(default_len);
#endif

    if (_dc_default == NULL ) {
        DBLOG("Failed to allocate memory for credential storage");
        return -1;
    }
    memcpy(_dc_default, cred_default, default_len);
    _dc_default_len = default_len;

    BSL_KeyStoreInit();

    BSL_InstallObject(IOSC_BOOTOSVER_HANDLE, IOSC_DATA_TYPE,
                IOSC_VERSION_SUBTYPE, NULL, 0, (u8 *)&osVer);

    BSL_InstallObject(IOSC_CACRLVER_HANDLE, IOSC_DATA_TYPE, IOSC_VERSION_SUBTYPE,
		      NULL, 0, (u8 *)&cacrlVer);

    BSL_InstallObject(IOSC_SIGNERCRLVER_HANDLE, IOSC_DATA_TYPE, IOSC_VERSION_SUBTYPE,
		      NULL, 0, (u8 *)&crlVer);
    
#if defined(GVM_FEAT_PLAT_CMN_KEY)
	if ( BSL_CreateObject(&_plat_cmn_key, IOSC_SECRETKEY_TYPE,
                           IOSC_ENC_SUBTYPE) != IOSC_ERROR_OK ) {
		DBLOG("plat common key object create failed\n");
		return -1;
	}

    if ( BSL_ImportSecretKey(_plat_cmn_key, 0, 0, IOSC_NOSIGN_NOENC, NULL,
                              NULL, _dc_default->k_plat) != IOSC_ERROR_OK) {
        DBLOG("plat common secret key import failed\n");
        return -1;
    }
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)

    if (_root_key_dflt_extract()) {
        return -1;
    }

    _initialized = true;

    return 0;
}

int
crypto_impl_load_cred(dev_cred_secret *secret, dev_cred_clear *clear,
                      u32 clear_len)
{
    if ( !_initialized ) {
        return -1;
    }

    if ( _cred_loaded ) {
        return 0;
    }

#ifdef __KERNEL__
    _dc_secret = kmalloc(sizeof(dev_cred_secret), GFP_KERNEL);
    _dc_clear = kmalloc(clear_len, GFP_KERNEL);
#else
    _dc_secret = malloc(sizeof(dev_cred_secret));
    _dc_clear = malloc(clear_len);
#endif
    if (_dc_secret == NULL || _dc_clear == NULL) {
        DBLOG("Failed to allocate memory for credential storage");
        return -1;
    }
    memcpy(_dc_secret, secret, sizeof(dev_cred_secret));
    memcpy(_dc_clear, clear, clear_len);
    _dc_clear_len = clear_len;

    initDeviceKeys();

    BSL_InstallObject(IOSC_DEV_ID_HANDLE, IOSC_DATA_TYPE, IOSC_CONSTANT_SUBTYPE,
		      NULL, 0, (u8 *)&deviceId);
    
    BSL_InstallObject(IOSC_DEV_SIGNING_KEY_HANDLE, IOSC_SECRETKEY_TYPE,
                IOSC_ECC233_SUBTYPE, _dc_secret->k_priv,
                sizeof(_dc_secret->k_priv), NULL);

    BSL_InstallObject(IOSC_COMMON_ENC_HANDLE, IOSC_SECRETKEY_TYPE,
                IOSC_ENC_SUBTYPE, _dc_secret->k_sym, sizeof(_dc_secret->k_sym),
                NULL);

    /* set up protection for secret keys */
    BSL_SetProtection(IOSC_DEV_SIGNING_KEY_HANDLE, BSL_PROT_NO_EXPORT);
    BSL_SetProtection(IOSC_COMMON_ENC_HANDLE, BSL_PROT_NO_EXPORT);

#if defined(GVM_FEAT_PLAT_CMN_KEY)
    if ( BSL_ImportSecretKey(_plat_cmn_key, 0, 0, IOSC_NOSIGN_NOENC, NULL,
                              NULL, _dc_secret->k_plat) != IOSC_ERROR_OK) {
        DBLOG("plat common secret key import failed\n");
        return -1;
    }
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)

    if (_root_key_extract()) {
        return -1;
    }

    _cred_loaded = true;

    return 0;
}


#if defined(GVM_FEAT_PLAT_CMN_KEY)
void
crypto_impl_platkey_get(IOSCSecretKeyHandle *key)
{
    *key = _plat_cmn_key;
}
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)
