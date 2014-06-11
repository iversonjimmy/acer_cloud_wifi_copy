#ifndef __GHV_CRYPTO_IMPL_H__
#define __GHV_CRYPTO_IMPL_H__

#include "dev_cred.h"

int crypto_impl_init(dev_cred_default *cred_default, u32 default_len);
int crypto_impl_load_cred(dev_cred_secret *secret, dev_cred_clear *clear, u32 clear_len);

#if defined(GVM_FEAT_PLAT_CMN_KEY)
void crypto_impl_platkey_get(IOSCSecretKeyHandle *key);
#endif // defined(GVM_FEAT_PLAT_CMN_KEY)

IOSCError IOSCryptoGetDeviceType(u32 *devType);

#endif // __GHV_CRYPTO_IMPL_H__
