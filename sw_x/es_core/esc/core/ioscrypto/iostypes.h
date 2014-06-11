/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#ifndef __IOSTYPES_H__
#define __IOSTYPES_H__

#include <km_types.h>

#define IOS_ERROR_OK                     0
#define IOS_ERROR_ACCESS                -1
#define IOS_ERROR_EXISTS                -2
#define IOS_ERROR_INTR                  -3
#define IOS_ERROR_INVALID               -4
#define IOS_ERROR_MAX                   -5
#define IOS_ERROR_NOEXISTS              -6
#define IOS_ERROR_QEMPTY                -7
#define IOS_ERROR_QFULL                 -8
#define IOS_ERROR_UNKNOWN               -9
#define IOS_ERROR_NOTREADY             -10
#define IOS_ERROR_ECC                  -11
#define IOS_ERROR_ECC_CRIT             -12
#define IOS_ERROR_BADBLOCK             -13

#define IOS_ERROR_INVALID_OBJTYPE      -14
#define IOS_ERROR_INVALID_RNG          -15
#define IOS_ERROR_INVALID_FLAG         -16
#define IOS_ERROR_INVALID_FORMAT       -17
#define IOS_ERROR_INVALID_VERSION      -18
#define IOS_ERROR_INVALID_SIGNER       -19
#define IOS_ERROR_FAIL_CHECKVALUE      -20
#define IOS_ERROR_FAIL_INTERNAL        -21
#define IOS_ERROR_FAIL_ALLOC           -22
#define IOS_ERROR_INVALID_SIZE         -23

#define IOS_ERROR_HW_RESET             -24

#define IOS_THREAD_CREATE_JOINABLE 0
#define IOS_THREAD_CREATE_DETACHED 1

#define IOS_MESSAGE_BLOCK        0
#define IOS_MESSAGE_NOBLOCK      1

#define IOS_EVENT_AES    0
#define IOS_EVENT_SHA    1
#define IOS_EVENT_USB    2
#define IOS_EVENT_FLASH  3
#define IOS_EVENT_ADC    4
#define IOS_EVENT_DAC    5
#define IOS_EVENT_CIF    6
#define IOS_EVENT_TIMER  7
#define IOS_NUM_EVENTS   8

#define IOS_SEEK_SET     0
#define IOS_SEEK_CURRENT 1
#define IOS_SEEK_END     2

#define IOS_OPEN     1
#define IOS_CLOSE    2
#define IOS_READ     3
#define IOS_WRITE    4
#define IOS_SEEK     5
#define IOS_IOCTL    6
#define IOS_IOCTLV   7
#define IOS_REPLY    8
#define IOS_DSMODE   9

#define IOS_DEVICE_USB      0
#define IOS_DEVICE_AUDIO    1
#define IOS_DEVICE_FLASH    2
#define IOS_DEVICE_AES      3
#define IOS_DEVICE_SHA      4

#define IOS_ROOT_ID     0
#define IOS_ROOT_GROUP  0

#define IOS_LEGACY_NONE         0
#define IOS_LEGACY_SRAM_256K    1
#define IOS_LEGACY_FLASH_512K   2
#define IOS_LEGACY_FLASH_1024K  3
#define IOS_LEGACY_EEROM_4K     4
#define IOS_LEGACY_EEROM_64K    5

#define IOS_KERNEL_PROCESS   0
#define IOS_ETICKET_PROCESS  1
#define IOS_FS_PROCESS       2
#define IOS_USB_PROCESS      3
#define IOS_AUDIO_PROCESS    4
#define IOS_VIEWER_PROCESS   5
#define IOS_NC_PROCESS       6
#define IOS_GBA_PROCESS      7
#define IOS_VN_PROCESS       8
#define IOS_DEVMON_PROCESS   9
#define IOS_EC_PROCESS       10
#define IOS_SOCP_PROCESS     11

#define IOS_PROCESS_MAX  12

#define IOS_USAGE_THREADS   (1 << 0)
#define IOS_USAGE_QUEUES    (1 << 1)
#define IOS_USAGE_TIMERS    (1 << 2)
#define IOS_USAGE_RESOURCES (1 << 3)
#define IOS_USAGE_HEAPS     (1 << 4)

#if !defined(ASSEMBLER)
typedef s32 IOSError;

typedef s32 IOSThreadId;
typedef s32 IOSProcessId;
typedef s32 IOSMessageQueueId;
typedef s32 IOSMessage;
typedef s32 IOSTimerId;
typedef s32 IOSHeapId;

typedef s32 IOSFd;

typedef u32 IOSUid;
typedef u16 IOSGid;

typedef u32 IOSTime;

typedef void (*IOSEntryProc)(u32);

typedef u32 IOSEvent;

typedef u32 IOSResourceHandle;

typedef struct {
    const u8 *path;
    u32 flags;
    IOSUid uid;
    IOSGid gid;
} IOSResourceOpen;

typedef struct {
    u8 *outPtr;
    u32 outLen;
} IOSResourceRead;

typedef struct {
    u8 *inPtr;
    u32 inLen;
} IOSResourceWrite;

typedef struct {
    s32 offset;
    u32 whence;
} IOSResourceSeek;

typedef struct {
    u32 cmd;
    u8 *inPtr;
    u32 inLen;
    u8 *outPtr;
    u32 outLen;
} IOSResourceIoctl;

typedef struct {
    u8 *base;
    u32 length;
} IOSIoVector;

typedef struct {
    u32 cmd;
    u32 readCount;
    u32 writeCount;
    IOSIoVector *vector;
} IOSResourceIoctlv;

typedef union {
    u8 key[16];      // crypto key;
    u32 rom_base;    // DS ROM base;
} IOSResourceDs;

typedef struct {
    u32 cmd;
    IOSError status;
    IOSResourceHandle handle;
    union {
        IOSResourceOpen open;
        IOSResourceRead read;
        IOSResourceWrite write;
        IOSResourceSeek seek;
        IOSResourceIoctl ioctl;
        IOSResourceIoctlv ioctlv;
        IOSResourceDs ds;
    } args;
} IOSResourceRequest;

typedef struct {
    u8 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
} IOSDate;

#endif

#endif /*__IOSTYPES_H__*/
