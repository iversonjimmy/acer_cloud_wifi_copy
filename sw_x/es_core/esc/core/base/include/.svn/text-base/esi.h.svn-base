/*
 *               Copyright (C) 2010, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */


#ifndef __ESI_H__
#define __ESI_H__

#include <esitypes.h>

ES_NAMESPACE_START

#ifdef __cplusplus
extern "C" {
#endif


/*
 * ESI_Translate_IOSC_Error() takes in an IOSC error code and returns
 * the corresponding ES error code
 */
ESError ESI_Translate_IOSC_Error(IOSCError err);


/*
 * ESI_VerifyContainer() takes in the data and certs.  It verifies
 * the entire cert chain.  If "data" is not NULL, it also verifies
 * the data against the specified signature.  If hPubKey is not 0,
 * then "data" is assumed to be a certificate itself and its public
 * key is imported into the handle
 *
 * If the containerType is ES_CONTAINER_TMD or ES_CONTAINER_TKT,
 * then the issuer's public key handle is cached by
 * ESI_VerifyContainer().  The cached handle is returned in
 * outIssuerPubKeyHandle
 */
ESError ESI_VerifyContainer(ESContainerType containerType,
                            const void *data, u32 dataSize,
                            const void *sig, const char *issuer,
                            const void *certs[], u32 nCerts,
                            IOSCPublicKeyHandle hPubKey,
                            IOSCPublicKeyHandle *outIssuerPubKeyHandle);


/*
 * ESI_PersonalizeTicket() takes in the v0 portion of the ticket and
 * personalize it (i.e. encrypt the title key with the shared key)
 */
ESError ESI_PersonalizeTicket(ESTicket *ticket);


/*
 * ESI_UnpersonalizeTicket() takes in the v0 portion of the ticket and
 * unpersonalize it (i.e. decrypt the title key with the shared key)
 */
ESError ESI_UnpersonalizeTicket(ESTicket *ticket);


/*
 * ESI_ParseCertList() takes in an appended list of certs and sets the
 * output array to point to each individual cert
 */
ESError ESI_ParseCertList(const void *certList, u32 certListSize,
                          void *outCerts[], u32 *outCertSizes,
                          u32 *outNumCerts);


/*
 * ESI_MergeCerts() takes in two arrays of certs and sets the output
 * array to point to each individual unique cert, eliminating the
 * duplicates
 */
ESError ESI_MergeCerts(const void *certs1[], u32 nCerts1,
                       const void *certs2[], u32 nCerts2,
                       void *outCerts[], u32 *outCertSizes,
                       u32 *outNumCerts);


/*
 * ESI_GetCertSize() takes a cert pointer and returns its length in bytes
 */
ESError ESI_GetCertSize(const void *cert, u32 *outCertSize);


/*
 * ESI_GetCertNames() takes a cert pointer and returns pointers to
 * the issuer and subject name fields
 */
ESError ESI_GetCertNames(const void *cert, u8 **issuerName, u8 **subjectName);


/*
 * ESI_GetCertPubKey() takes a cert pointer and returns information about
 * the public key embedded in it
 */
ESError ESI_GetCertPubKey(const void *cert, u8 **pubKey, u32 *pubKeySize, 
                          u32 *exponent);


/*
 * ESI_FindCert() takes a cert ID and searches an array of certs to find
 * the one that matches
 */
ESError ESI_FindCert(const char *id, u32 completeId,
                     const void *certs[], u32 nCerts,
                     void **cert, u32 *certSize, char **certIssuer);


/*
 * ESI_VerifyCert() verifies one cert against a cert chain that tracks
 * back either to a known Root of trust or a cert that is already in
 * the internal cache of verified certs.
 */
ESError ESI_VerifyCert(const void *cert, const void *verifyChain[], u32 nChainCerts);


/*
 * For testing only
 */
ESError ESI_ClearCertCache(void);


#ifdef __cplusplus
}
#endif //__cplusplus

ES_NAMESPACE_END

#endif  // __ESI_H__
