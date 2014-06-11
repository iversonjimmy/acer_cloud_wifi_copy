//
//  Copyright (C) 2009, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//
//  bal_cert.h: BroadOn Abstraction Library: Certificate Management.
//
//  This module manages certificate authentication chains between the root
//  certificate and device certificates. Each module provides the certificate
//  chains differently. This abstraction takes are of those differences
//  providing a simple interface by which device certificates can be verified
//  and relevant details from the certificates accessed.
//
//  Only device certificates should be processed here. Device-signed
//  certificates may be validated and imported directly by IOSC APIs,
//  which are already platform independent.

#ifndef __VPLEX_CERT_H__
#define __VPLEX_CERT_H__

#include "vplex_plat.h"

#ifndef VPLCERT_NOT_SUPPORTED // BW does not support this module.

#include "vplex__cert.h"

#ifdef  __cplusplus
extern "C" {
#endif

/// Device type codes for supported devices
/// These codes are referenced on the wiki at [[Definitions_(Infrastructure)]]

#define VPL_DEVICE_TYPE_IQUE       0x00 // End-of-life
#define VPL_DEVICE_TYPE_RVL        0x01 // Wii/NDEV
#define VPL_DEVICE_TYPE_NC         0x02 // NetCard - End-of-life
#define VPL_DEVICE_TYPE_TWL        0x03 // TWL/DSi
#define VPL_DEVICE_TYPE_PC         0x7f // Virtual PC clients
#define VPL_DEVICE_TYPE_P2P_SERVER 0x80 // BroadOn server
#define VPL_DEVICE_TYPE_CS_SERVER  0x81 // CS infrastructure server
#define VPL_DEVICE_TYPE_UNKNOWN    0xff // Unknown/Error device type

/// Initialize certificate manager.
/// This call loads the device certificate verification chains.
/// Before any device certificate may be verified and its public key imported,
/// the certificate management module must be initialized.
/// See the platform-specific implementation as to what must be set-up in the
/// environment for this to be successful (varies by platform).
/// @return 0 is returned on success, negative value on failure.
int VPLCert_Init(void);

/// Clean-up certificate manager.
/// This call clears the certificate state, destroying all keys loaded for
/// validating device certificates. After this call, certificate verification
/// is no longer possible.
/// @return 0 is returned on success, negative value on failure.
int VPLCert_Cleanup(void);

/// Verify a device certificate against known cert chains.
/// The certificate buffer contains a single device certificate. The 
/// certificate is verified against the certificate chains loaded at init.
/// If the certificate is authenticated, the device's public key is imported
/// from the certificate and the handle returned. Use this key handle to
/// authenticate any certificate signed by the device. 
/// It is the caller's responsibility to destroy the device public key when
/// it is no longer needed.
/// @param cert_buf    A buffer with a device certificate.
/// @param key_handle  Pointer to return location of the key handle for the
///                    device public key, imported if the cert is good.
/// @return 0 if the certificate was authenticated and key handle provided.
///         Negative on error. Error code is the IOSC error encountered.
int VPLCert_VerifyDeviceCert(const u8* cert_buf, 
                             IOSCPublicKeyHandle* key_handle);

/// Get the device ID from a device cert.
/// @param cert_buf    A buffer with a device certificate.
/// @param cert_len    Length of \a cert_buf.
/// @param device_type Pointer to return location of the device type read
///                    from the certificate.
/// @param device_id   Pointer to return location of the device ID read from
///                    the certificate.
/// @return 0 if the device ID was found and returned. Negative otherwise.
int VPLCert_GetDeviceId(u8* cert_buf,
                        size_t cert_len,
                        u32* device_type, 
                        u32* device_id);
                        
#ifdef __cplusplus
}
#endif

#endif // VPLCERT_NOT_SUPPORTED // BW does not support this module.

#endif // include guard
