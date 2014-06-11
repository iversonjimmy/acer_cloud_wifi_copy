/*
 *               Copyright (C) 2005, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */

/* 
 * defs common to SK and ES
 */

#ifndef __IOSCCERT_H__
#define __IOSCCERT_H__

#include <esc_iosctypes.h>

typedef u8   IOSCName[64];         /* cert chain name, used to describe 
                                    * ascii list hierarchy as: 
                                    *   string with xxxxxxxx representing 
                                    *   serial number in hex. (eg XS0000000f 
                                    *   is ticket server 15). pad with nulls.
                                    *   Root-CAxxxxxxxx-XSxxxxxxxx 
                                    *   Root-CAxxxxxxxx-CPxxxxxxxx
                                    *   Root-CAxxxxxxxx-MSxxxxxxxx
                                    *   Root-CAxxxxxxxx-MSxxxxxxxx-NCxxxxxxxx
                                    */
typedef u8   IOSCDeviceId[64];       /* device ID in the form of NCxxxxxxxx */
typedef u8   IOSCServerId[64];       /* holds only suffix name for ESServerName
                                    * (i.e., XSxxxxxxxx, where xxxxxxxx is
                                    * the serial number). */
typedef u8   IOSCSigDummy[60];
typedef u8   IOSCCertPad[52];
typedef u8   IOSCEccCertPad[4];

/* pack to 4 byte boundaries */
#pragma pack(push,4)

typedef struct {
    IOSCCertSigType sigType;
    IOSCRsaSig2048  sig;
    IOSCSigDummy    dummy;
    IOSCName        issuer;   
} IOSCSigRsa2048;

typedef struct {
    IOSCCertSigType sigType;
    IOSCRsaSig4096  sig;
    IOSCSigDummy    dummy;
    IOSCName        issuer;   /* Root */
} IOSCSigRsa4096;

typedef struct {
    IOSCCertSigType  sigType;
    IOSCEccSig       sig;
    IOSCEccPublicPad eccPad;
    IOSCSigDummy     dummy;
    IOSCName         issuer;  /* Root-CAxxxxxxxx-MSxxxxxxxx-NCxxxxxxxx */
} IOSCSigEcc;

typedef struct {
    IOSCCertPubKeyType pubKeyType;
    union {
        IOSCServerId serverId;
        IOSCDeviceId deviceId;
    } name;
    u32 date;
} IOSCCertHeader;

 /* Root cert */
typedef struct {
    IOSCSigRsa4096        sig;
    IOSCCertHeader        head;
    IOSCRsaPublicKey4096  pubKey;
    IOSCRsaExponent       exponent;
    IOSCCertPad           pad;
} IOSCRootCert;

/* public key 2048 bits, sign 4096 bits */
typedef struct {
    IOSCSigRsa4096        sig;
    IOSCCertHeader        head;
    IOSCRsaPublicKey2048  pubKey;
    IOSCRsaExponent       exponent;
    IOSCCertPad           pad;
} IOSCRsa4096RsaCert;

/* public key 2048 bits, sign 2048 bits */
typedef struct {
    IOSCSigRsa2048        sig;
    IOSCCertHeader        head;
    IOSCRsaPublicKey2048  pubKey;
    IOSCRsaExponent       exponent;
    IOSCCertPad           pad;
} IOSCRsa2048RsaCert;

/* device certs */
typedef struct {
    IOSCSigRsa2048      sig;        /* 2048 bit RSA signature */
    IOSCCertHeader      head;
    IOSCEccPublicKey    pubKey;     /* 60 byte public key */
    IOSCEccPublicPad    eccPad;
    IOSCCertPad         pad;
    IOSCEccCertPad      pad2;
} IOSCRsa2048EccCert;

/* devices signed certs */
typedef struct {
    IOSCSigEcc          sig;        /* ECC signature struct */
    IOSCCertHeader      head;
    IOSCEccPublicKey    pubKey;     /* 60 byte ECC public key */
    IOSCEccPublicPad    eccPad;
    IOSCCertPad         pad;
    IOSCEccCertPad      pad2;
} IOSCEccEccCert;

#pragma pack(pop)

#endif
