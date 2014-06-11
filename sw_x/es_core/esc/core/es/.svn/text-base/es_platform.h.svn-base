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

#ifndef __ES_PLATFORM_H__
#define __ES_PLATFORM_H__

#include "km_types.h"
#include "ioslibc.h"

#if defined(__cplusplus) && defined(USE_ES_NAMESPACE)

#define ES_NAMESPACE_START \
    namespace bcc { \
    namespace client { \
    namespace es {

#define ES_NAMESPACE_END \
    } } }

#define ES_NAMESPACE bcc::client::es
#define USING_ES_NAMESPACE using namespace ES_NAMESPACE;

#else

/*
 * Namespaces don't matter for the hypervisor
 */
#define ES_NAMESPACE_START 
#define ES_NAMESPACE_END
#define ES_NAMESPACE
#define USING_ES_NAMESPACE

#endif

#if defined(ES_DEBUG)
#  if defined(esLog)
#    undef esLog
#  endif

#  if defined(GHV) || defined(_KERNEL_MODULE)
#    define esLog(l, ...)   kprintf("[ES] "__VA_ARGS__)
#  else // !GHV
#    include "log.h"
#    define esLog(l, ...)  LOG_INFO(__VA_ARGS__)
#  endif // !GHV
#endif // ES_DEBUG

typedef u16 ESGVMGSSSize;
typedef u16 ESGVMTmdFlags;
typedef u64 ESGVMUserId;
typedef u16 ESGVMTicketType;

#define ES_GVM_TMD_FLAG_IS_GSS                  0x0001

#define ES_GVM_TICKET_TYPE_USER                 0x0001
#define ES_GVM_TICKET_TYPE_SIGNATURE            0x0002

typedef enum 
{
    ESGVM_KEYID_DEVELOPMENT = 0,
    ESGVM_KEYID_OFFLINE = 1,
    ESGVM_KEYID_ONLINE = 2,
    ESGVM_KEYID_PLATFORM_APC = 16       // acer personal cloud
} ESGVMKeyId;

#pragma pack(push, 2)
typedef struct {
    ESGVMGSSSize	gssSize;
    ESGVMUserId		userId;
    ESGVMTmdFlags	flags;
} ESGVMTmdPlatformData;


typedef struct {
    ESGVMTicketType     type;
    u16                 reserved;
    u64                 userId;
} ESGVMTicketPlatformData;
#pragma pack(pop)

#endif  // __ES_PLATFORM_H__
