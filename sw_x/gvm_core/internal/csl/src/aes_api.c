#include "aes_api.h"

#include "csl_impl.h"
#include "aes_imp.h"

typedef unsigned char BYTE;

int aesMakeKey(AesKeyInstance *key, BYTE direction, int keyLen, const BYTE *keyMaterial) {
	if (key == NULL) {
		return AES_BAD_KEY_INSTANCE;
	}

	if ((direction == AES_DIR_ENCRYPT) || (direction == AES_DIR_DECRYPT)) {
		key->direction = direction;
	} else {
		return AES_BAD_KEY_DIR;
	}

	if ((keyLen != 128) && (keyLen != 192) && (keyLen != 256)) {
		return AES_BAD_KEY_MAT;
	}

	/* initialize key schedule: */
	if (direction == AES_DIR_ENCRYPT) {
		key->Nr = rijndaelKeySetupEnc((u32 *)key->rk, keyMaterial, keyLen);
	} else {
		key->Nr = rijndaelKeySetupDec((u32 *)key->rk, keyMaterial, keyLen);
	}
	rijndaelKeySetupEnc((u32 *)key->ek, keyMaterial, keyLen);
	return AES_TRUE;
}

int aesCipherInit(AesCipherInstance *cipher, BYTE mode, const BYTE *IV) {
	if (0
#ifdef AES_MODE_ECB
	    || (mode == AES_MODE_ECB)
#endif
#ifdef AES_MODE_CBC
	    || (mode == AES_MODE_CBC)
#endif
#ifdef AES_MODE_CFB1
	    || (mode == AES_MODE_CFB1)
#endif
	    ) {
		cipher->mode = mode;
	} else {
		return AES_BAD_CIPHER_MODE;
	}
	if (IV != NULL) {
	        memcpy(cipher->IV, IV, AES_MAX_IV_SIZE);
	} else {
		memset(cipher->IV, 0, AES_MAX_IV_SIZE);
	}
	return AES_TRUE;
}

int aesBlockEncrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		const BYTE *input, int inputLen, BYTE *outBuffer) {
	int i,  numBlocks;
#ifdef AES_MODE_CFB1
	int k, t;
#endif
	u8 block[16], *iv;

	if (cipher == NULL ||
		key == NULL ||
		key->direction == AES_DIR_DECRYPT) {
		return AES_BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputLen/128;
	
	switch (cipher->mode) {
#ifdef AES_MODE_ECB
	case AES_MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelEncrypt((u32 *)key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;
#endif
		
#ifdef AES_MODE_CBC
	case AES_MODE_CBC: {
		u32 *b32, *in32, *iv32;
		iv = cipher->IV;
		iv32 = (u32 *)cipher->IV;
		b32 = (u32 *)block;
		in32 = (u32 *)input;
		for (i = numBlocks; i > 0; i--) {
			b32[0] = in32[0] ^ iv32[0];
			b32[1] = in32[1] ^ iv32[1];
			b32[2] = in32[2] ^ iv32[2];
			b32[3] = in32[3] ^ iv32[3];
			rijndaelEncrypt((u32 *)key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
                        iv32 = (u32*)iv;
			input += 16;
                        in32 = (u32*)input;
			outBuffer += 16;
		}
		memcpy(cipher->IV, iv, AES_MAX_IV_SIZE);
		break;
	}
#endif

#ifdef AES_MODE_CFB1
    case AES_MODE_CFB1:
		iv = cipher->IV;
        for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
            for (k = 0; k < 128; k++) {
				rijndaelEncrypt(key->ek, key->Nr, iv, block);
                outBuffer[k >> 3] ^= (block[0] & 0x80U) >> (k & 7);
                for (t = 0; t < 15; t++) {
                	iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
                }
               	iv[15] = (iv[15] << 1) | ((outBuffer[k >> 3] >> (7 - (k & 7))) & 1);
            }
            outBuffer += 16;
            input += 16;
        }
        break;
#endif

	default:
		return AES_BAD_CIPHER_STATE;
	}
	
	return 128*numBlocks;
}

#if 0
/**
 * Encrypt data partitioned in octets, using RFC 2040-like padding.
 *
 * @param   input           data to be encrypted (octet sequence)
 * @param   inputOctets		input length in octets (not bits)
 * @param   outBuffer       encrypted output data
 *
 * @return	length in octets (not bits) of the encrypted output buffer.
 */
int aesPadEncrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		BYTE *input, int inputOctets, BYTE *outBuffer) {
	int i, numBlocks, padLen;
	u8 block[16], *iv;

	if (cipher == NULL ||
		key == NULL ||
		key->direction == AES_DIR_DECRYPT) {
		return AES_BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
#ifdef AES_MODE_ECB
	case AES_MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelEncrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		memcpy(block, input, 16 - padLen);
		memset(block + 16 - padLen, padLen, padLen);
		rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;
#endif

#ifdef AES_MODE_CBC
	case AES_MODE_CBC:
		iv = cipher->IV;
		for (i = numBlocks; i > 0; i--) {
			((u32*)block)[0] = ((u32*)input)[0] ^ ((u32*)iv)[0];
			((u32*)block)[1] = ((u32*)input)[1] ^ ((u32*)iv)[1];
			((u32*)block)[2] = ((u32*)input)[2] ^ ((u32*)iv)[2];
			((u32*)block)[3] = ((u32*)input)[3] ^ ((u32*)iv)[3];
			rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
			iv = outBuffer;
			input += 16;
			outBuffer += 16;
		}
		padLen = 16 - (inputOctets - 16*numBlocks);
		for (i = 0; i < 16 - padLen; i++) {
			block[i] = input[i] ^ iv[i];
		}
		for (i = 16 - padLen; i < 16; i++) {
			block[i] = (BYTE)padLen ^ iv[i];
		}
		rijndaelEncrypt(key->rk, key->Nr, block, outBuffer);
		break;
#endif

	default:
		return AES_BAD_CIPHER_STATE;
	}

	return 16*(numBlocks + 1);
}
#endif

int aesBlockDecrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		const BYTE *input, int inputLen, BYTE *outBuffer) {
	int i, numBlocks;
#ifdef AES_MODE_CFB1
	int k, t;
#endif
	u8 block[16];
	u32 *iv;

	if (cipher == NULL ||
		key == NULL ||
#ifdef AES_MODE_ECB
		(cipher->mode == AES_MODE_ECB && key->direction == AES_DIR_ENCRYPT) ||
#endif
#ifdef AES_MODE_CBC
		(cipher->mode == AES_MODE_CBC && key->direction == AES_DIR_ENCRYPT) ||
#endif
	0) {
		return AES_BAD_CIPHER_STATE;
	}
	if (input == NULL || inputLen <= 0) {
		return 0; /* nothing to do */
	}

	numBlocks = inputLen/128;

	switch (cipher->mode) {
#ifdef AES_MODE_ECB
	case AES_MODE_ECB:
		for (i = numBlocks; i > 0; i--) {
			rijndaelDecrypt((u32 *)key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		break;
#endif
		
#ifdef AES_MODE_CBC
	case AES_MODE_CBC: {
		u32 *b32;
		iv = (u32 *)cipher->IV;
		b32 = (u32 *)block;
		for (i = numBlocks; i > 0; i--) {
			rijndaelDecrypt((u32 *)key->rk, key->Nr, input, block);
			b32[0] ^= iv[0];
			b32[1] ^= iv[1];
			b32[2] ^= iv[2];
			b32[3] ^= iv[3];
			memcpy(cipher->IV, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		break;
	}
#endif

#ifdef AES_MODE_CFB1
    case AES_MODE_CFB1:
		iv = cipher->IV;
        for (i = numBlocks; i > 0; i--) {
			memcpy(outBuffer, input, 16);
            for (k = 0; k < 128; k++) {
				rijndaelEncrypt(key->ek, key->Nr, iv, block);
                for (t = 0; t < 15; t++) {
                	iv[t] = (iv[t] << 1) | (iv[t + 1] >> 7);
                }
               	iv[15] = (iv[15] << 1) | ((input[k >> 3] >> (7 - (k & 7))) & 1);
                outBuffer[k >> 3] ^= (block[0] & 0x80U) >> (k & 7);
            }
            outBuffer += 16;
            input += 16;
        }
        break;
#endif

	default:
		return AES_BAD_CIPHER_STATE;
	}
	
	return 128*numBlocks;
}

#if 0
int aesPadDecrypt(AesCipherInstance *cipher, AesKeyInstance *key,
		BYTE *input, int inputOctets, BYTE *outBuffer) {
	int i, numBlocks, padLen;
	u8 block[16];

	if (cipher == NULL ||
		key == NULL ||
		key->direction == AES_DIR_ENCRYPT) {
		return AES_BAD_CIPHER_STATE;
	}
	if (input == NULL || inputOctets <= 0) {
		return 0; /* nothing to do */
	}
	if (inputOctets % 16 != 0) {
		return AES_BAD_DATA;
	}

	numBlocks = inputOctets/16;

	switch (cipher->mode) {
#ifdef AES_MODE_ECB
	case AES_MODE_ECB:
		/* all blocks but last */
		for (i = numBlocks - 1; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, outBuffer);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		rijndaelDecrypt(key->rk, key->Nr, input, block);
		padLen = block[15];
		if (padLen >= 16) {
			return AES_BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return AES_BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;
#endif
		
#ifdef AES_MODE_CBC
	case AES_MODE_CBC:
		/* all blocks but last */
		for (i = numBlocks - 1; i > 0; i--) {
			rijndaelDecrypt(key->rk, key->Nr, input, block);
			((u32*)block)[0] ^= ((u32*)cipher->IV)[0];
			((u32*)block)[1] ^= ((u32*)cipher->IV)[1];
			((u32*)block)[2] ^= ((u32*)cipher->IV)[2];
			((u32*)block)[3] ^= ((u32*)cipher->IV)[3];
			memcpy(cipher->IV, input, 16);
			memcpy(outBuffer, block, 16);
			input += 16;
			outBuffer += 16;
		}
		/* last block */
		rijndaelDecrypt(key->rk, key->Nr, input, block);
		((u32*)block)[0] ^= ((u32*)cipher->IV)[0];
		((u32*)block)[1] ^= ((u32*)cipher->IV)[1];
		((u32*)block)[2] ^= ((u32*)cipher->IV)[2];
		((u32*)block)[3] ^= ((u32*)cipher->IV)[3];
		padLen = block[15];
		if (padLen <= 0 || padLen > 16) {
			return AES_BAD_DATA;
		}
		for (i = 16 - padLen; i < 16; i++) {
			if (block[i] != padLen) {
				return AES_BAD_DATA;
			}
		}
		memcpy(outBuffer, block, 16 - padLen);
		break;
#endif
	
	default:
		return AES_BAD_CIPHER_STATE;
	}
	
	return 16*numBlocks - padLen;
}
#endif
