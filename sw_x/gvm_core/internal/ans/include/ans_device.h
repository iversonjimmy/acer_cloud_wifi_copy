//
//  Copyright 2011-2013 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __ANS_DEVICE_H__
#define __ANS_DEVICE_H__

#include <vpl_time.h>
#include <vpl_net.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque handle to a client.
typedef struct client_s ans_client_t;

// Values for #ans_unpacked_t.type
//
//  Message types set the notification and notificationLength fields.  The
//  userId field is valid, too.  SEND_USER_NOTIFICATION messages set the
//  device id.
#define SEND_USER_NOTIFICATION   6
#define SEND_USER_MULTICAST      7

/// Sets the fields: #ans_unpacked_t.deviceId, #ans_unpacked_t.newDeviceState, and
/// #ans_unpacked_t.newDeviceTime.
#define SEND_DEVICE_UPDATE      43

/// Sets the fields: #ans_unpacked_t.asyncId, #ans_unpacked_t.deviceList,
/// #ans_unpacked_t.deviceStates, #ans_unpacked_t.deviceTimes, and #ans_unpacked_t.deviceCount.
/// The asyncId value is from the call to #ans_requestDeviceState().
#define SEND_STATE_LIST         42

/// Send generated sleep information for a specified IOAC type and MAC
/// address.  The IOAC type and MAC address are returned so that the
/// client can match up responses if desired.
#define SEND_SLEEP_SETUP        53

/// Send automatically-generated sleep information on login.
#define SEND_SETUP_INFO          0

/// Send a response packet.  This message is received as a
/// response to a unicast or a multicast, and for other
/// operations sent with a response id with the lowest bit
/// set.
#define SEND_RESPONSE            1

// Values for device states
#define DEVICE_ONLINE    1
#define DEVICE_SLEEPING  2
#define DEVICE_OFFLINE   3
#define QUERY_FAILED     4

/**
 *     When a packet is received, it is unpacked from wire format before being
 *  passed to a device client callback.  This structure describes the
 *  unpacked format.
 *
 *     The memory for this descriptor is released by the ANS library when the
 *  callback returns!  Any information that needs to be saved must be copied!
 */
typedef struct {
    uint16_t    type;
    uint64_t    userId;
    uint64_t    asyncId;
    uint64_t    deviceId;
    uint64_t    response;

    /*
     *  Device updates set these two fields, plus the deviceId.
     */
    char        newDeviceState;     /* DEVICE_ONLINE, DEVICE_SLEEPING, ... */
    uint64_t    newDeviceTime;      /* the time device went to sleep, in   */
                                    /* milliseconds since the epoch        */

    /*
     * Device queries set these fields.
     */
    uint64_t *  deviceList;         /* the device ids                      */
    char *      deviceStates;       /* DEVICE_ONLINE, DEVICE_SLEEPING, ... */
    uint64_t *  deviceTimes;        /* the time device went to sleep       */
    int         deviceCount;        /* the size of the arrays              */

    /*
     *  Message types set these fields, as well as the userId.  Unicasts
     *  (SEND_USER_NOTIFICATION) also set the deviceId field.
     */
    void *      notification;
    int         notificationLength;

    /*
     *  These fields are set by the SEND_SETUP_INFO ans SEND_SLEEP_SETUP
     *  messages.
     */
    char *      wakeupKey;          /* the wakeup key for the chip          */
    int         wakeupKeyLength;    /* the number of bytes in the key       */
    char *      sleepPacket;        /* the packet to be sent while sleeping */
    int         sleepPacketLength;  /* the length of the packet array       */

    char *      sleepDns;           /* the hostname portion of the DNS      */
    uint16_t    sleepPort;          /* the sleep port, in native byte order */
    uint32_t    sleepPacketInterval;/* the interval in seconds between      */
                                    /* sleep packets                        */
    /*
     *  These fields are set only by the SEND_SLEEP_SETUP message.
     */
    int         ioacType;
    char *      macAddress;
    int         macAddressLength;

    /*
     *  These fields are for internal use.
     */
    char *      plainSleep;
    char *      plainKey;
    int64_t     sequenceId;
    uint64_t    sendTime;
    uint64_t    rtt;
    uint64_t    connection;
    uint64_t    tag;
} ans_unpacked_t;

/**
 * The user of this library must supply a callback vector when creating a
 * client.  This structure describes that callback vector. connectionActive
 * and receiveNotification return a boolean which always is ignored.
 * connectionDown is called when the connection to the ANS instance is lost.
 * This warning is advisory: the connection will be re-established if and when
 * possible.  connectionClosed is called when a user-requested connection close
 * attempt is completed.
 */
typedef struct {
    VPL_BOOL (*connectionActive)   (ans_client_t *, VPLNet_addr_t   local_addr);
    VPL_BOOL (*receiveNotification)(ans_client_t *, ans_unpacked_t *unpacked  );
    void     (*receiveSleepInfo)   (ans_client_t *, ans_unpacked_t *unpacked  );
    void     (*receiveDeviceState) (ans_client_t *, ans_unpacked_t *unpacked  );
    void     (*connectionDown)     (ans_client_t *                            );
    void     (*connectionClosed)   (ans_client_t *                            );
    void     (*setPingPacket)      (ans_client_t *, char *packet, int length  );
    void     (*rejectCredentials)  (ans_client_t *                            );
    void     (*loginCompleted)     (ans_client_t *                            );
    void     (*rejectSubscriptions)(ans_client_t *                            );
    void     (*receiveResponse)    (ans_client_t *, ans_unpacked_t *response  );
} ans_callbacks_t;

/// This procedure creates an instance of a device client.  The caller
/// must specify the DNS address of the ANS server and a callback vector
/// that will handle events.
//@{

/// No wakeup capability.
#define ioac_none     0
/// MAGIC_PACKET_TYPE_EXTENDED (supported by Atheros, Realtek).
/// 108 bytes: 0xFF x 6 + MAC_Address (6 bytes) x 16 + Wakeup_Secret (6 bytes)
/// When calling ans_requestSleepSetup, you must provide the true macAddress.
#define ioac_proto    1
/// MAGIC_PACKET_TYPE_ACER_SHORT (supported by Intel).
/// 60 bytes: 0xFF x 6 + Fixed_Pattern (6 bytes) x 8 + Wakeup_Secret (6 bytes)
/// When calling ans_requestSleepSetup, you must provide the Fixed_Pattern as the macAddress.
/// You can use IOCTL_CUSTOMER_FIXEDPATTERN_SET and IOCTL_CUSTOMER_FIXEDPATTERN_QUERY to set/get
/// the Fixed_Pattern with the hardware.
#define ioac_intel    2

typedef struct {
    const char *  clusterName;      /* cluster to which to connect             */
    const void *  blob;             /* the connection blob                     */
    int           blobLength;
    const void *  key;              /* the session encryption key              */
    int           keyLength;
    const char *  deviceType;       /* the device type for keep-alive          */
    const char *  application;      /* the application name, for WinRT, etc    */
    int           server_tcp_port;  /* Server port number                      */    
    int           verbose;          /* a boolean for requesting verbose output */

    const ans_callbacks_t *  callbacks;
} ans_open_t;

extern ans_client_t *ans_open(const ans_open_t *input);
//@}

/// This procedure shuts down the ANS client thread and frees any
/// resources associated with the given client instance.
void  ans_close(ans_client_t *client, VPL_BOOL waitForShutdown);

/// @return #VPL_TRUE if the request was successfully queued to be sent.
/// This operations causes the current device state to be returned in the
/// same way as ans_requestDeviceState.
/// If the connection is not established yet, the request will be queued.
VPL_BOOL ans_setSubscriptions(ans_client_t *client, uint64_t asyncId, const uint64_t *devices, int count);

/// @return #VPL_TRUE if the request was successfully queued to be sent.
VPL_BOOL ans_requestDeviceState(ans_client_t *client, uint64_t asyncId, const uint64_t *devices, int count);

VPL_BOOL ans_requestSleepSetup(ans_client_t *client, uint64_t asyncId, int ioac_type,
            const void *macAddress, int macLength);

/// @return #VPL_TRUE if the request was successfully queued to be sent.
VPL_BOOL ans_requestWakeup(ans_client_t *client, uint64_t device);

/// Notifies the ANS client that there was a change in the local device's
/// network connectivity.
/// @return #VPL_TRUE if successfully processed.
VPL_BOOL ans_onNetworkConnected(ans_client_t *client);

/// Notify the ANS client that the network connectivity has changed.  If
/// dropConnection is set to TRUE, kill the current TCP connection, if there
/// is one, and reconnect.
VPL_BOOL ans_declareNetworkChange(ans_client_t *client, VPL_BOOL dropConnection);

/*
 *  The async id should be an odd number if the caller wants to receive a
 *  response message.  The user_id field currently must match the user id
 *  logged onto the connection.  The ans_device library forces the bottom
 *  bit of the async_id field to one, to force a response from the server.
 */
VPL_BOOL ans_sendUnicast(ans_client_t *client, uint64_t user_id,
            uint64_t device_id, const void *message, int count,
            uint64_t async_id);

VPL_BOOL ans_sendMulticast(ans_client_t *client, uint64_t user_id,
            const void *message, int count, uint64_t async_id);

/*
 *  This routine returns success if the client is valid.
 */
VPL_BOOL ans_setVerbose(ans_client_t *, VPL_BOOL verbose);

/// Sets the foreground-background state.
/// @param isForeground When false, After telling the server that this connection is entering background mode,
/// ANS client will not use any CPU cycles or send any network traffic until an incoming packet arrives
/// or until another ans_* API is called.
/// @param interval Ignored when isForeground.  Otherwise, the caller promises to call
///     #ans_background() within \a interval after calling this function and again
///     within \a interval after each call to #ans_background().
VPL_BOOL ans_setForeground(ans_client_t *client, VPL_BOOL isForeground, VPLTime_t interval);

/// Does any work needed when a background device wakeup occurs.
/// The intention is for this operation to be batched with other operations that require CPU and
/// network adapter activity.  For example, on Android, we will drive this from
/// AlarmManager.setInexactRepeating() to minimize overall power consumption.
VPL_BOOL ans_background(ans_client_t *client);

#ifdef __cplusplus
}
#endif

#endif // include guard
