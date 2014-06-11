#ifndef __AES_API_H__
#define __AES_API_H__

#include <km_types.h>

/*  Generic Defines  */
#define     AES_DIR_ENCRYPT           0 /*  Are we encrypting?  */
#define     AES_DIR_DECRYPT           1 /*  Are we decrypting?  */
#define     AES_MODE_ECB              1 /*  Are we ciphering in ECB mode?   */
#define     AES_MODE_CBC              2 /*  Are we ciphering in CBC mode?   */
/* #define     AES_MODE_CFB1             3   Are we ciphering in 1-bit CFB mode? */
#define     AES_TRUE                  1
#define     AES_FALSE                 0
#define     AES_BITSPERBLOCK        128 /* Default number of bits in a cipher block */

/*  Error Codes  */
#define     AES_BAD_KEY_DIR          -1 /*  Key direction is invalid, e.g., unknown value */
#define     AES_BAD_KEY_MAT          -2 /*  Key material not of correct length */
#define     AES_BAD_KEY_INSTANCE     -3 /*  Key passed is not valid */
#define     AES_BAD_CIPHER_MODE      -4 /*  Params struct passed to cipherInit invalid */
#define     AES_BAD_CIPHER_STATE     -5 /*  Cipher in wrong state (e.g., not initialized) */
#define     AES_BAD_BLOCK_LENGTH     -6
#define     AES_BAD_CIPHER_INSTANCE  -7
#define     AES_BAD_DATA             -8 /*  Data contents are invalid, e.g., invalid padding */
#define     AES_BAD_OTHER            -9 /*  Unknown error */

/*  Algorithm-specific Defines  */
#define     AES_MAX_KEY_SIZE         64 /* # of ASCII char's needed to represent a key */
#define     AES_MAX_IV_SIZE          16 /* # bytes needed to represent an IV  */

#define __AES_MAXNR 	14

/*  Typedefs  */

/*  The structure for key information */
typedef struct {
    unsigned char  direction;       /* Key used for encrypting or decrypting? */
    int   Nr;                       /* key-length-dependent number of rounds */
    uint32_t rk[4*(__AES_MAXNR + 1)];        /* key schedule */
    uint32_t ek[4*(__AES_MAXNR + 1)];        /* CFB1 key schedule (encryption only) */
} AesKeyInstance;

/*  The structure for cipher information */
typedef struct {                    /* changed order of the components */
    unsigned int  mode;            /* MODE_ECB, MODE_CBC, or MODE_CFB1 */
    unsigned char  IV[AES_MAX_IV_SIZE]; /* A possible Initialization Vector for ciphering */
} AesCipherInstance;

/*  Function prototypes  */

int aesMakeKey(AesKeyInstance *key, unsigned char direction, int keyLen, const unsigned char *AesKeyMaterial);

int aesCipherInit(AesCipherInstance *cipher, unsigned char mode, const unsigned char *IV);

int aesBlockEncrypt(AesCipherInstance *cipher, AesKeyInstance *key,
        const unsigned char *input, int inputLen, unsigned char *outBuffer);

#if 0
int aesPadEncrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		unsigned char *input, int inputOctets, unsigned char *outBuffer);
#endif

int aesBlockDecrypt(AesCipherInstance *cipher, AesKeyInstance *key,
        const unsigned char *input, int inputLen, unsigned char *outBuffer);

#if 0
int aesPadDecrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		unsigned char *input, int inputOctets, unsigned char *outBuffer);
#endif

#endif	/* __AES_API_H__ */
