#ifndef __AES_IMP_H__
#define __AES_IMP_H__

#include <km_types.h>

#define __AES_MAXKC	(256/32)
#define __AES_MAXKB	(256/8)
#define __AES_MAXNR	14

int rijndaelKeySetupEnc(u32 rk[/*4*(Nr + 1)*/], const u8 cipherKey[], int keyBits);
int rijndaelKeySetupDec(u32 rk[/*4*(Nr + 1)*/], const u8 cipherKey[], int keyBits);
void rijndaelEncrypt(const u32 rk[/*4*(Nr + 1)*/], int Nr, const u8 pt[16], u8 ct[16]);
void rijndaelDecrypt(const u32 rk[/*4*(Nr + 1)*/], int Nr, const u8 ct[16], u8 pt[16]);

#endif /* __AES_IMP_H__ */
