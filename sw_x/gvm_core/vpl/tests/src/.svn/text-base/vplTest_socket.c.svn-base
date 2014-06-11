//
//  Copyright (C) 2007-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplTest.h"

#include <vpl_socket.h>
#include <vpl_th.h>
#include <vpl_error.h>
#include <vpl_fs.h>

#ifdef _MSC_VER
#include <io.h>
#endif

#define ECHO_CLIENT_PORT        65432   /* Some random port */
#define ECHO_SERVER_PORT        7
#define POLL_TEST_PORT_1        65331
#define POLL_TEST_PORT_2        65332

// We alias these buffers as uint32_t arrays, so make sure this remains a
// multiple of 4.
#define VPLSOCKET_TEST_BUF_SIZE    1024
static u8 testBuf_out[VPLSOCKET_TEST_BUF_SIZE];
static u8 testBuf_in[VPLSOCKET_TEST_BUF_SIZE];

#define VPLSOCKET_TEST_TIMEOUT     3000

#define ONE_SEC 1000000
#define FIVE_SEC 5000000

#define ASSERT_RV(rv, expected, fname) \
    do { if (rv != expected) { \
        VPLTEST_NONFATAL_ERROR("got %d from %s, expected %d.", rv, fname, expected); \
        return; \
    } } while (ALWAYS_FALSE_CONDITIONAL)

#define ASSERT_SOCK(rv, fname) \
    do { if (VPLSocket_Equal(rv, VPLSOCKET_INVALID)) { \
        VPLTEST_NONFATAL_ERROR("got VPLSOCKET_INVALID from %s.", fname); \
        return; \
    } } while (ALWAYS_FALSE_CONDITIONAL)

static void doPollTest(void);
static void vplPollTestHelper1(int);
static void* vplPollTestHelper2(void*);

#ifndef _MSC_VER
#ifndef __USE_POSIX
// Need to declare this POSIX function.
int fileno(FILE* file);
#endif
#endif

static void testGetPeer(void)
{
    VPLSocket_addr_t  address;
    VPLSocket_t       server_socket;
    VPLSocket_t       out_socket;
    VPLSocket_t       socket;

    int  result;

    /*
     *  First make a server socket to which we can connect.
     */
    server_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_FALSE);

    if (VPLSocket_Equal(server_socket, VPLSOCKET_INVALID)) {
        VPLTEST_FAIL("testGetPeer:  create server");
        return;
    }

    /*
     *  Now bind the socket to an address.  We use port 0, the wildcard port.
     */
    memset(&address, 0, sizeof(address));

    address.family = VPL_PF_INET;
    address.addr   = VPLNET_ADDR_LOOPBACK;

    result = VPLSocket_Bind(server_socket, &address, sizeof(address));

    if (result != VPL_OK) {
        VPLTEST_FAIL("testGetPeer:  bind");
        return;
    }

    /*
     *  Make it possible to receive connections on the socket.
     */
    result = VPLSocket_Listen(server_socket, 10);

    if (result != VPL_OK) {
        VPLTEST_FAIL("testGetPeer:  listen");
        return;
    }

    /*
     *  Get the address of the server socket.  We used a wildcard port,
     *  so now we need to determine the actual target.
     */
    address.addr = VPLSocket_GetAddr(server_socket);
    address.port = VPLSocket_GetPort(server_socket);

    /*
     *  Now create a socket and connect to the server socket.
     */
    out_socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, VPL_TRUE);

    if (VPLSocket_Equal(out_socket, VPLSOCKET_INVALID)) {
        VPLTEST_FAIL("testGetPeer:  create 2");
        return;
    }

    result = VPLSocket_Connect(out_socket, &address, sizeof(address));

    if (result != VPL_OK) {
        VPLTEST_FAIL("testGetPeer:  connect");
        return;
    }

    /*
     *  Accept the incoming connection.
     */
    result = VPLSocket_Accept(server_socket, &address, sizeof(address), &socket);

    if (result != VPL_OK) {
        VPLTEST_FAIL("testGetPeer:  accept");
        return;
    }

    if (VPLSocket_GetPeerPort(socket) != VPLSocket_GetPort(out_socket)) {
        VPLTEST_FAIL("testGetPeer: GetPeerPort and GetPort disagree!\n");
        exit(1);
    }

    if (VPLSocket_GetPeerAddr(socket) != VPLSocket_GetAddr(out_socket)) {
        VPLTEST_FAIL("testGetPeer: GetPeerAddr and GetAddr disagree!\n");
        exit(1);
    }
}

void testVPLSocket(void)
{
    int rc;
    VPLNet_addr_t local_addr, remote_addr, gateway_addr;
    VPLNet_addr_t addrs[VPLNET_MAX_INTERFACES], masks[VPLNET_MAX_INTERFACES];
    VPLNet_port_t local_port;
    VPLSocket_t sockfd;
    VPLSocket_addr_t sock_addr, sock_addr_remote;
    VPLSocket_addr_t svc_addr;
    u32 i;
    int done_bytes = 0;

#ifndef VPL_PLAT_IS_WINRT
    // badSock is intentionally poisoned with a filesystem file descriptor instead of a socket
    // file descriptor.
    // This is a questionable thing to test, since it requires breaking the abstraction.
    // It is also not applicable on WinRT, since our impl doesn't even use file descriptors.
    VPLSocket_t badSock;
#  ifdef _MSC_VER
    badSock.s = (SOCKET)_get_osfhandle(_fileno(tmpfile()));
#  else
    badSock.fd = fileno(tmpfile());
#  endif
#endif

    VPLTEST_LOG("testGetPeer");
    testGetPeer();

// TODO: Bug 10826 Fix WinRT impl.
#if !defined(VPL_PLAT_IS_WINRT)
    // Bug 10696: Test VPLSocket_Accept() on a non-blocking socket with no incoming connection.
    {
        VPLSocket_t tempSock;
        VPLSocket_t socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/1 );
        ASSERT_SOCK(socket, "VPLSocket_Create");

        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(0);
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr)), VPL_OK);
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Listen(socket,1), VPL_OK);
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(socket, NULL, 0, &tempSock), VPL_ERR_AGAIN);
    }
#endif

    VPLTEST_LOG("Init");
    rc = VPLSocket_Init();
    VPLTEST_ENSURE_OK(rc, "VPLSocket_Init");

    VPLTEST_LOG("Get local address");
    local_addr = VPLNet_GetLocalAddr();
    if (local_addr == VPLNET_ADDR_INVALID) {
        VPLTEST_NONFATAL_ERROR("VPLNet_GetLocalAddr() returned VPLNET_ADDR_INVALID");
    } else {
        VPLTEST_LOG("IPv4 address of local host is "FMT_VPLNet_addr_t, VAL_VPLNet_addr_t(local_addr));
    }

    VPLTEST_LOG("Get list of local IP addresses");
    rc = VPLNet_GetLocalAddrList(addrs, masks);
    VPLTEST_CHK_NONNEGATIVE(rc, "get list of local IP addresses");

    for (i = 0; ((int)i) < rc; i++) {
        VPLTEST_LOG("%d: addr - 0x%08x, netmask - 0x%08x", i, addrs[i], masks[i]);
    }

#ifdef TEST_DEBUG
    {
        const char *str;
        VPLTEST_LOG("Check IPv4 address conversion to string");
        str = VPLNet_Ntop(&local_addr, (char *) testBuf_in, VPLSOCKET_TEST_BUF_SIZE);
        if (str == NULL) {
            VPLTEST_FAIL("Convert IPv4 address 0x%08x to string.", local_addr);
        }
        VPLTEST_LOG("IPv4 address string %s", str);
    }
#endif

    VPLTEST_LOG("Get IPv4 address from host name");
    remote_addr = VPLNet_GetAddr("echo.ctbg.acer.com");
    if (remote_addr == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("Get echo server IPv4 address returns 0x%08x.", remote_addr);
        // nothing more we can do here...
        goto fail;
    }

    VPLTEST_LOG("IPv4 address of echo server is 0x%08x", remote_addr);
#ifndef VPL_PLAT_IS_WINRT
    VPLTEST_LOG("Get default gateway");
    gateway_addr = VPLNet_GetDefaultGateway();
    if (gateway_addr == VPLNET_ADDR_INVALID) {
        VPLTEST_FAIL("Get default gateway IPv4 address returns 0x%08x.", gateway_addr);
    }
    VPLTEST_LOG("IPv4 address of default gateway is 0x%08x", gateway_addr);
#endif
    VPLTEST_LOG("Create a new UDP socket to the echo server");
    sockfd = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, /*nonblock*/1);

    if (VPLSocket_Equal(sockfd, VPLSOCKET_INVALID)) {
        VPLTEST_FAIL("Create UDP socket.");
    }
#if !defined(_MSC_VER) && !defined(IOS)
    {
        int mtu_discover = 0;
        rc = VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
                                   (void*)&mtu_discover, sizeof(mtu_discover));
        if (rc != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt with error code: %d.", rc);
        }
    }
#endif
    sock_addr.family = VPL_PF_INET;
    sock_addr.addr = VPLNET_ADDR_INVALID;
    sock_addr.port = VPLNet_port_hton(ECHO_CLIENT_PORT);
    rc = VPLSocket_Bind(sockfd, &sock_addr, sizeof(sock_addr));
    if (rc != VPL_OK) {
        VPLTEST_FAIL("VPLSocket_Bind with error code: %d.", rc);
    }

#if !defined(_MSC_VER) && !defined(IOS)
    int sockOpt = 0;

    // Test calling Socket_SetSockOpt with NULL value.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET,
            VPLSOCKET_IP_MTU_DISCOVER, NULL, 0), VPL_ERR_INVALID);
    // Test calling Socket_SetSockOpt with bad socket.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SetSockOpt(badSock, VPLSOCKET_SOL_SOCKET,
            VPLSOCKET_IP_MTU_DISCOVER, (void*)&sockOpt, sizeof(sockOpt)), VPL_ERR_NOTSOCK);

    // Test calling Socket_GetSockOpt with bad socket.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SetSockOpt(badSock, VPLSOCKET_SOL_SOCKET,
            VPLSOCKET_IP_MTU_DISCOVER, (void*)&sockOpt, sizeof(sockOpt)), VPL_ERR_NOTSOCK);
#endif

    // Test calling Socket_Bind again on the same socket. Should get VPL_ERR_INVALID.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_Bind(sockfd, &sock_addr, sizeof(sock_addr)),
            VPL_ERR_INVALID);
    // Test calling Socket_Bind with NULL address. Should get VPL_ERR_INVALID.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_Bind(sockfd, NULL, sizeof(sock_addr)),
            VPL_ERR_INVALID);
#ifndef VPL_PLAT_IS_WINRT
    // Not applicable for WinRT (see badSock's declaration).
    // Test calling Socket_Bind with bad socket. Should get VPL_ERR_NOTSOCK.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_Bind(badSock, &sock_addr, sizeof(sock_addr)),
            VPL_ERR_NOTSOCK);
#endif

    VPLTEST_LOG("Check local port");
    local_port = VPLSocket_GetPort(sockfd);
    if (local_port != VPLNet_port_hton(ECHO_CLIENT_PORT)) {
        VPLTEST_FAIL("Check local port, code %d.", local_port);
    }
    
    // UDP Socket Test
    {
        u32 runCount = 1;
        u32 successCount = 0;
        // Repeat 10 times
        while (runCount < 11) {
            VPLTEST_LOG("Send data to echo server(Run:%d)", runCount);
            /* Fill buffer with some random data */
            for (i = 0; i < VPLSOCKET_TEST_BUF_SIZE/sizeof(uint32_t); i++) {
                ((uint32_t*)testBuf_out)[i] = rand();
            }
            
            sock_addr_remote.family = VPL_PF_INET;
            sock_addr_remote.addr = remote_addr;
            sock_addr_remote.port = VPLNet_port_hton(ECHO_SERVER_PORT);
            rc = VPLSocket_SendTo(sockfd, testBuf_out, VPLSOCKET_TEST_BUF_SIZE,
                                  &sock_addr_remote, sizeof(sock_addr_remote));
            if (rc != VPLSOCKET_TEST_BUF_SIZE) {
                VPLTEST_FAIL("Send data to echo server(Run:%d), code %d.", runCount, rc);
            }
            
            VPLTEST_LOG("Receive data from echo server(Run:%d)", runCount);
            memset(testBuf_in, 0, VPLSOCKET_TEST_BUF_SIZE);
            {
                u32 eAgainCount = 0;
                done_bytes = 0;
                
                while (done_bytes < VPLSOCKET_TEST_BUF_SIZE) {
                    rc = VPLSocket_RecvFrom(sockfd, testBuf_in + done_bytes,
                                            VPLSOCKET_TEST_BUF_SIZE - done_bytes,
                                            &svc_addr, sizeof(svc_addr));
                    if (rc > 0) {
                        done_bytes += rc;
                    }
                    else if (rc == VPL_ERR_AGAIN) {
                        eAgainCount++;
                        VPLThread_Sleep(10000); // wait 10ms for reply.
                        if ((eAgainCount % 512 == 0)) {
                            VPLTEST_LOG("excessive back-to-back EAGAIN from UDP socket(Run:%d), %d times!?", runCount, eAgainCount);
                            break;
                        }
                    }
                    else { // failure, but not EAGAIN
                        // Directly mark this test as fail
                        successCount = 0;
                        runCount = 11;
                        if (rc == 0) {
                            VPLTEST_FAIL("Disconnect from UDP socket!?(Run:%d)", runCount);
                        }
                        else {
                            VPLTEST_FAIL("Unexpected error(Run:%d) %d", runCount, rc);
                        }
                    } // failure, but not EAGAIN
                }
            }
            
            if (done_bytes != VPLSOCKET_TEST_BUF_SIZE ||
                svc_addr.addr != remote_addr ||
                svc_addr.port != VPLNet_port_hton(ECHO_SERVER_PORT)) {
                VPLTEST_LOG("Receive data from echo server(Run:%d). Received:%d, code:%d, sender addr:0x%08x, sender port:0x%04x.",
                             runCount, done_bytes, rc, svc_addr.addr, svc_addr.port);
            } else {
                VPLTEST_LOG("Sender address - 0x%08x, sender port - 0x%04x", svc_addr.addr, svc_addr.port);
                
                /* Check data */
                if (memcmp(testBuf_out, testBuf_in, VPLSOCKET_TEST_BUF_SIZE) != 0) {
                    VPLTEST_LOG("Receive data check failed(Run:%d)", runCount);
                } else {
                    VPLTEST_LOG("Data from echo server received(Run:%d)", runCount);
                    successCount++;
                }
            }
            runCount++;
        }

        // Tolerate up to 50% fail rate.
        if (successCount > 4) {
            VPLTEST_LOG("UDP socket test SUCCESS with %d/10 success rate", successCount);
        } else {
            VPLTEST_FAIL("UDP socket test FAIL with %d/10 success rate", successCount);
        }
    }
    // UDP Socket Test ends
    
    // Test calling Socket_RecvFrom with NULL buffer. Should get VPL_ERR_AGAIN.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_RecvFrom(sockfd, NULL, 0,
            &svc_addr, sizeof(svc_addr)), VPL_ERR_AGAIN);
    // Test calling Socket_RecvFrom with NULL address. Should get VPL_ERR_AGAIN.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_RecvFrom(sockfd, testBuf_in, VPLSOCKET_TEST_BUF_SIZE,
            NULL, 0), VPL_ERR_AGAIN);
    // Test calling Socket_RecvFrom on the same socket. Should get VPL_ERR_AGAIN.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_RecvFrom(sockfd, NULL, 0, NULL, 0), VPL_ERR_AGAIN);
#ifndef VPL_PLAT_IS_WINRT
    // Not applicable for WinRT (see badSock's declaration).
    // Test calling Socket_RecvFrom on a bad socket. Should get VPL_ERR_NOTSOCK.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_RecvFrom(badSock, NULL, 0, NULL, 0), VPL_ERR_NOTSOCK);
#endif

    // Test calling Socket_SendTo with NULL data. Should get VPL_OK.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SendTo(sockfd, NULL, 0,
                                             &sock_addr_remote, sizeof(sock_addr_remote)), VPL_OK);
    // Test calling Socket_SendTo with NULL address. Should get VPL_ERR_INVALID.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SendTo(sockfd, testBuf_out, VPLSOCKET_TEST_BUF_SIZE,
                                             NULL, 0), VPL_ERR_INVALID);
#ifndef VPL_PLAT_IS_WINRT
    // Not applicable for WinRT (see badSock's declaration).
    // Test calling Socket_SendTo with bad socket. Should get VPL_ERR_NOTSOCK.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SendTo(badSock, testBuf_out, VPLSOCKET_TEST_BUF_SIZE,
                                             &sock_addr_remote, sizeof(sock_addr_remote)), VPL_ERR_NOTSOCK);
#endif

    VPLTEST_LOG("Close socket");
    rc = VPLSocket_Close(sockfd);
    VPLTEST_CHK_OK(rc, "VPLSocket_Close");

    // Test calling Socket_Close on socket that is already closed. Should get VPL_ERR_BADF for non-windows platform, and VPL_ERR_NOTSOCK for win32/winRT.
#ifndef _MSC_VER
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_Close(sockfd), VPL_ERR_BADF);
#else
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_Close(sockfd), VPL_ERR_NOTSOCK);
#endif

#if !defined(_MSC_VER) && !defined(IOS)
    // Test calling Socket_SetSockOpt on socket that is already closed. Should get VPL_ERR_BADF.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
            (void*)&sockOpt, sizeof(sockOpt)), VPL_ERR_BADF);

    // Test calling Socket_GetSockOpt on socket that is already closed. Should get VPL_ERR_BADF.
    VPLTEST_CALL_AND_CHK_RV(VPLSocket_GetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
            (void*)&sockOpt, sizeof(sockOpt)), VPL_ERR_BADF);
#endif
    VPLTEST_LOG("Polling test");
    doPollTest();

    VPLTEST_LOG("Cleanup");
    rc = VPLSocket_Quit();
    VPLTEST_CHK_OK(rc, "VPLSocket_Quit");

 fail:
    NO_OP();
}

static void doPollTest()
{
    vplPollTestHelper1(0);
    vplPollTestHelper1(1);
    vplPollTestHelper1(2);
#ifdef VPL_PLAT_IS_WINRT
    vplPollTestHelper1(3);
#endif
}

VPLMutex_t pollMutex;

// action 0: receive and send UDP
// action 1: accept TCP, receive and send, remote close
// action 2: accept TCP, remote reset
static void vplPollTestHelper1(int action)
{
    u8 bytes[10] = {64,1,75,33,243,22,97,0,177,2};
    VPLSocket_t socket, tcpsock;
    VPLThread_t helper;
    VPLSocket_poll_t pollsock;
    int rv, rc;
    void* rval;
    VPLNet_addr_t localhost = VPLNet_GetLocalAddr();
    VPLSocket_addr_t sock_addr, sock_addr_remote;

#ifndef VPL_PLAT_IS_WINRT
    // badSock is intentionally poisoned with a filesystem file descriptor instead of a socket
    // file descriptor.
    // This is a questionable thing to test, since it requires breaking the abstraction.
    // It is also not applicable on WinRT, since our impl doesn't even use file descriptors.
    VPLSocket_t badSock;
#  ifdef _MSC_VER
    badSock.s = (SOCKET)_get_osfhandle(_fileno(tmpfile()));
#  else
    badSock.fd = fileno(tmpfile());
#  endif
#endif

    rv = VPLMutex_Init(&pollMutex);
    ASSERT_RV(rv, VPL_OK, "VPLMutex_Init()");

    rv = VPLMutex_Lock(&pollMutex);
    ASSERT_RV(rv, VPL_OK, "VPLMutex_Lock()");

    rv = VPLThread_Create(&helper,
            vplPollTestHelper2,
            VPL_AS_THREAD_FUNC_ARG(action),
             0, // default VPLThread thread-attributes: prio, stack-size, etc.
            "VPL Poll test helper");
    ASSERT_RV(rv, VPL_OK, "VPLThread_Create()");

    if (action == 0) {
        VPLTEST_LOG("Polling with readable UDP socket.");

        socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, /*nonblock*/1);
        if (VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
            VPLTEST_FAIL("Create UDP socket.");
        }
#if !defined(_MSC_VER) && !defined(IOS)
        {
            int mtu_discover = 0;
            rv = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
                    (void*)&mtu_discover, sizeof(mtu_discover));
            if (rv != VPL_OK) {
                VPLTEST_FAIL("VPLSocket_SetSockOpt with error code: %d.", rv);
            }

            int mtu_discover_result = 0;
            rv = VPLSocket_GetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
                    (void*)&mtu_discover_result, sizeof(mtu_discover_result));
            if (rv != VPL_OK) {
                VPLTEST_FAIL("VPLSocket_GetSockOpt with error code: %d.", rv);
            }
            if (mtu_discover_result != mtu_discover) {
                VPLTEST_FAIL("VPLSocket_GetSockOpt read back value that is different from what was set.");
            }

        }
#endif
        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_Bind with error code: %d.", rv);
        }
        ASSERT_SOCK(socket, "VPLSocket_CreateUdp()");

        rv = VPLMutex_Unlock(&pollMutex);
        ASSERT_RV(rv, VPL_OK, "VPLMutex_Unlock()");

        pollsock.socket = socket;
        pollsock.events = VPLSOCKET_POLL_RDNORM;
        rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
        if (rv != 1) {
            VPLTEST_NONFATAL_ERROR("Expected poll to return 1, got %d.", rv);
        }
        if (pollsock.revents != VPLSOCKET_POLL_RDNORM) {
            VPLTEST_NONFATAL_ERROR("Poll flags; expected 0x%x, got 0x%x.",
                    (int)(VPLSOCKET_POLL_RDNORM), (int)(pollsock.revents));
        }

        // Test calling Socket_Poll with NULL sockets. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Poll(NULL, 1, ONE_SEC), VPL_ERR_INVALID);

        rv = 0;
        while (rv < 10) {
            int rc = VPLSocket_Recv(pollsock.socket, &bytes[rv], 10 - rv);
            if (rc >= 0) {
                rv += rc;
            } else if (rc == VPL_ERR_AGAIN) {
                VPLThread_Yield();
            } else {
                VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
                break;
            }
        }
        ASSERT_RV(rv, 10, "VPLSocket_Recv()");

        VPLTEST_LOG("Polling with writable UDP socket.");
        pollsock.events = VPLSOCKET_POLL_OUT;
        rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
        if (rv != 1) {
            VPLTEST_NONFATAL_ERROR("Expected poll to return 1, got %d.", rv);
        }
        if (pollsock.revents != VPLSOCKET_POLL_OUT) {
            VPLTEST_NONFATAL_ERROR("Poll flags; expected 0x%x, got 0x%x.",
                    (int)(VPLSOCKET_POLL_OUT), (int)(pollsock.revents));
        }

        sock_addr_remote.family = VPL_PF_INET;
        sock_addr_remote.addr = localhost;
        sock_addr_remote.port = VPLNet_port_hton(POLL_TEST_PORT_2);
        rv = VPLSocket_SendTo(socket, bytes, 10, &sock_addr_remote, sizeof(sock_addr_remote));
        ASSERT_RV(rv, 10, "VPLSocket_SendTo()");
        rv = VPLSocket_Close(socket);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
    }
    else if (action < 3) {
        int reuse = 1, nodelay = 1;
        socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/1 );
        ASSERT_SOCK(socket, "VPLSocket_CreateTcp()");
        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                   (void*)&reuse, sizeof(reuse) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }

        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                                   (void*)&nodelay, sizeof(nodelay) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }

        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_Bind with error code %d", rv);
        }

        // Try calling Socket_Accept before Socket_Listen is called. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(socket, NULL, 0, &tcpsock), VPL_ERR_INVALID);

        rv = VPLSocket_Listen(socket, 1);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Listen()");

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Try calling Socket_Listen with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Listen(badSock, 1), VPL_ERR_NOTSOCK);
#endif
        rv = VPLMutex_Unlock(&pollMutex);
        ASSERT_RV(rv, VPL_OK, "VPLMutex_Unlock()");

        VPLTEST_LOG("Polling with pending TCP connection.");
        pollsock.socket = socket;
        pollsock.events = VPLSOCKET_POLL_RDNORM;
        rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
        if (rv != 1) {
            VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
        }
        if (pollsock.revents != VPLSOCKET_POLL_RDNORM) {
            VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                    (int)(VPLSOCKET_POLL_RDNORM), (int)(pollsock.revents));
        }

        // Try calling Socket_Accept with NULL connfd. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(socket, NULL, 0, NULL), VPL_ERR_INVALID);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(badSock, NULL, 0, &tcpsock), VPL_ERR_NOTSOCK);
#endif
        VPLSocket_Accept(socket, NULL, 0, &tcpsock);
        ASSERT_SOCK(tcpsock, "VPLSocket_Accept()");

        pollsock.socket = tcpsock;

        if (action == 1) {
            VPLTEST_LOG("Polling with readable TCP connection.");
            pollsock.events = VPLSOCKET_POLL_RDNORM;
            rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
            if (rv != 1) {
                VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            }
            if (pollsock.revents != VPLSOCKET_POLL_RDNORM) {
                VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                        (int)(VPLSOCKET_POLL_RDNORM), (int)(pollsock.revents));
            }

            rv = 0;
            while (rv < 10) {
                int rc = VPLSocket_Recv(tcpsock, &bytes[rv], 10 - rv);
                if (rc >= 0) {
                    rv += rc;
                }
                else if (rc == VPL_ERR_AGAIN) {
                    VPLThread_Yield();
                }
                else {
                    VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
                    break;
                }
            }
            ASSERT_RV(rv, 10, "VPLSocket_Recv()");

            VPLTEST_LOG("Polling with writable TCP connection.");
            pollsock.events = VPLSOCKET_POLL_OUT;
            rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
            if (rv != 1) {
                VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            }
            if (pollsock.revents != VPLSOCKET_POLL_OUT) {
                VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                        (int)(VPLSOCKET_POLL_OUT), (int)(pollsock.revents));
            }

            rv = 0;
            while (rv < 10) {
                int rc = VPLSocket_Send(tcpsock, &bytes[rv], 10-rv);
                if (rc >= 0) {
                    rv += rc;
                }
                else if (rc == VPL_ERR_AGAIN) {
                    VPLThread_Yield();
                }
                else {
                    VPLTEST_FAIL("VPLSocket_Send failed with error code: %d.", rc);
                }

                // Test calling Socket_Send with NULL buffer. Should get VPL_OK.
                VPLTEST_CALL_AND_CHK_RV(VPLSocket_Send(tcpsock, NULL, 0), VPL_OK);

#ifndef VPL_PLAT_IS_WINRT
                // Not applicable for WinRT (see badSock's declaration).
                // Test calling Socket_Send with bad socket. Should get VPL_ERR_NOTSOCK.
                VPLTEST_CALL_AND_CHK_RV(VPLSocket_Send(badSock, NULL, 0), VPL_ERR_NOTSOCK);
#endif
            }
            ASSERT_RV(rv, 10, "VPLSocket_Send()");
            //% TODO: enforce strict API and reenable this test
            //% See comment on #VPLSocket_Poll
#if 0
            VPLTEST_LOG("Polling with closed TCP connection.");
            pollsock.events = VPLSOCKET_POLL_RDNORM;
            rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
            if (rv != 1) {
                VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            }
            if (pollsock.revents != VPLSOCKET_POLL_HUP) {
                VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                        (int)(VPLSOCKET_POLL_HUP), (int)(pollsock.revents));
            }
#endif
        }
        else {
            //% TODO: enforce strict API and reenable this test
            //% See comment on #VPLSocket_Poll
#if 0
            VPLTEST_LOG("Polling with reset TCP connection.");
            pollsock.events = VPLSOCKET_POLL_RDNORM;
            rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
            if (rv != 1) {
                VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            }
            if (pollsock.revents != (VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_ERR)) {
                VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                        (int)(VPLSOCKET_POLL_HUP | VPLSOCKET_POLL_ERR),
                        (int)(pollsock.revents));
            }
#endif
        }
#ifndef IOS
        // On iOS, if the socket is closed on server-side, the following function returns fail.
        rv = VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_RD);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");
#endif
        rv = VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_WR);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

        // Disabled: this isn't really deterministic; it's possible for it to still return VPL_OK after
        //   a second.
        //
        //        // Sleep and wait for the shutdown to finish.
        //        VPLThread_Sleep(1000);
        //        // Test calling Shutdown on that socket again. Should get VPL_ERR_NOTCONN.
        //        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_RD), VPL_ERR_NOTCONN);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Shutdown on bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Shutdown(badSock, VPLSOCKET_SHUT_RD), VPL_ERR_NOTSOCK);
#endif
        rv = VPLSocket_Close(tcpsock);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");

        rv = VPLSocket_Close(socket);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
    }
    else if (action == 3) {
        int reuse = 1, nodelay = 1;
        socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*block*/0 );
        ASSERT_SOCK(socket, "VPLSocket_CreateTcp()");

        //WinRT not support
#ifndef VPL_PLAT_IS_WINRT
        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
            (void*)&reuse, sizeof(reuse) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }
#endif

        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
            (void*)&nodelay, sizeof(nodelay) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }

        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_Bind with error code %d", rv);
        }

        // Try calling Socket_Accept before Socket_Listen is called. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(socket, NULL, 0, &tcpsock), VPL_ERR_INVALID);

        rv = VPLSocket_Listen(socket, 1);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Listen()");

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Try calling Socket_Listen with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Listen(badSock, 1), VPL_ERR_NOTSOCK);
#endif

        rv = VPLMutex_Unlock(&pollMutex);
        ASSERT_RV(rv, VPL_OK, "VPLMutex_Unlock()");

        VPLTEST_LOG("Polling with pending TCP connection.");
        pollsock.socket = socket;
        pollsock.events = VPLSOCKET_POLL_RDNORM;
        rv = VPLSocket_Poll(&pollsock, 1, FIVE_SEC);
        if (rv != 1) {
            VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            goto BLOCK_ACCEPT_FAIL;
        }
        if (pollsock.revents != VPLSOCKET_POLL_RDNORM) {
            VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                (int)(VPLSOCKET_POLL_RDNORM), (int)(pollsock.revents));
        }

        // Try calling Socket_Accept with NULL connfd. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(socket, NULL, 0, NULL), VPL_ERR_INVALID);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Try calling Socket_Accept with bad socket. Should get VPL_ERR_BADF.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Accept(badSock, NULL, 0, &tcpsock), VPL_ERR_BADF);
#endif

        VPLSocket_Accept(socket, NULL, 0, &tcpsock);
        ASSERT_SOCK(tcpsock, "VPLSocket_Accept()");

        pollsock.socket = tcpsock;


        VPLTEST_LOG("Polling with readable TCP connection.");
        pollsock.events = VPLSOCKET_POLL_RDNORM;
        rv = VPLSocket_Poll(&pollsock, 1, FIVE_SEC);
        if (rv != 1) {
            VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            goto BLOCK_SR_FAIL;
        }
        if (pollsock.revents != VPLSOCKET_POLL_RDNORM) {
            VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                (int)(VPLSOCKET_POLL_RDNORM), (int)(pollsock.revents));
        }

        rc = VPLSocket_Recv(tcpsock, &bytes[0], 10);
        if (rc < 0) {
            VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
            goto BLOCK_SR_FAIL;
        }
        ASSERT_RV(rc, 10, "VPLSocket_Recv()");

        VPLTEST_LOG("Polling with writable TCP connection.");
        pollsock.events = VPLSOCKET_POLL_OUT;
        rv = VPLSocket_Poll(&pollsock, 1, FIVE_SEC);
        if (rv != 1) {
            VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
            goto BLOCK_SR_FAIL;
        }
        if (pollsock.revents != VPLSOCKET_POLL_OUT) {
            VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                (int)(VPLSOCKET_POLL_OUT), (int)(pollsock.revents));
        }

        rc = VPLSocket_Send(tcpsock, &bytes[0], 10);
        if (rc < 0) {
            VPLTEST_FAIL("VPLSocket_Send failed with error code: %d.", rc);
            goto BLOCK_SR_FAIL;
        }
        ASSERT_RV(rc, 10, "VPLSocket_Recv()");

        // Test calling Socket_Send with NULL buffer. Should get VPL_OK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Send(tcpsock, NULL, 0), VPL_OK);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Socket_Send with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Send(badSock, NULL, 0), VPL_ERR_NOTSOCK);
#endif

        //% TODO: enforce strict API and reenable this test
        //% See comment on #VPLSocket_Poll
#if 0
        VPLTEST_LOG("Polling with closed TCP connection.");
        pollsock.events = VPLSOCKET_POLL_RDNORM;
        rv = VPLSocket_Poll(&pollsock, 1, ONE_SEC);
        if (rv != 1) {
            VPLTEST_FAIL("Expected poll to return 1, got %d.", rv);
        }
        if (pollsock.revents != VPLSOCKET_POLL_HUP) {
            VPLTEST_FAIL("Poll flags; expected 0x%x, got 0x%x.",
                (int)(VPLSOCKET_POLL_HUP), (int)(pollsock.revents));
        }
#endif

        rv = VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_RD);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

        rv = VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_WR);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

        // Disabled: this isn't really deterministic; it's possible for it to still return VPL_OK after
        //   a second.
        //
        //        // Sleep and wait for the shutdown to finish.
        //        VPLThread_Sleep(1000);
        //        // Test calling Shutdown on that socket again. Should get VPL_ERR_NOTCONN.
        //        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Shutdown(tcpsock, VPLSOCKET_SHUT_RD), VPL_ERR_NOTCONN);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Shutdown on bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Shutdown(badSock, VPLSOCKET_SHUT_RD), VPL_ERR_NOTSOCK);
#endif

BLOCK_SR_FAIL:
        rv = VPLSocket_Close(tcpsock);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
BLOCK_ACCEPT_FAIL:
        rv = VPLSocket_Close(socket);
        ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
    }

    rv = VPLThread_Join(&helper, &rval);
    ASSERT_RV(rv, VPL_OK, "VPLThread_Join()");
#ifndef VPL_PLAT_IS_WINRT
    rv = VPLTHREAD_FUNC_ARG_TO_INT(rval);
    if (rv != 0) {
        VPLTEST_FAIL("Errors occurred in helper thread, see log.");
    }
#endif
    VPLMutex_Destroy(&pollMutex);
}

#define HELPER_ASSERT_RV(rv, expected, fname) \
    do { if (rv != expected) { \
        VPLTEST_LOG("ERROR: got %d from %s, expected %d.", rv, fname, expected); \
        return VPL_AS_THREAD_RETVAL(1); \
    } } while (ALWAYS_FALSE_CONDITIONAL)
#define HELPER_ASSERT_SOCK(rv, fname) \
    do { if (VPLSocket_Equal(rv, VPLSOCKET_INVALID)) { \
        VPLTEST_LOG("ERROR: got VPLSOCKET_INVALID from %s.", fname); \
        return VPL_AS_THREAD_RETVAL(1); \
    } } while (ALWAYS_FALSE_CONDITIONAL)

static void* vplPollTestHelper2(void* arg)
{
    u8 bytes[10] = {4,10,105,33,208,16,0,78,9,185};
    int action = VPLTHREAD_FUNC_ARG_TO_INT(arg);
    VPLNet_addr_t localhost = VPLNet_GetLocalAddr();
    int rv, rc;
    VPLSocket_addr_t sock_addr, sock_addr_remote;

#ifndef VPL_PLAT_IS_WINRT
    // badSock is intentionally poisoned with a filesystem file descriptor instead of a socket
    // file descriptor.
    // This is a questionable thing to test, since it requires breaking the abstraction.
    // It is also not applicable on WinRT, since our impl doesn't even use file descriptors.
    VPLSocket_t badSock;
# ifdef _MSC_VER
    badSock.s = (SOCKET)_get_osfhandle(_fileno(tmpfile()));
# else
    badSock.fd = fileno(tmpfile());
# endif
#endif

    rv = VPLMutex_Lock(&pollMutex);
    HELPER_ASSERT_RV(rv, VPL_OK, "VPLMutex_Lock()");
    if (action == 0) {
        VPLSocket_t socket;
        // Create a UDP socket
        VPLTEST_LOG("Creating UDP socket.");
        socket = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, /*nonblock*/1);
        if (VPLSocket_Equal(socket, VPLSOCKET_INVALID)) {
            VPLTEST_FAIL("Failed to create UDP socket.");
        }
#if !defined(_MSC_VER) && !defined(IOS)
        {
            int mtu_discover = 0;
            VPLTEST_LOG("Calling SetSockOpt on UDP socket.");
            rv = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_IP_MTU_DISCOVER,
                                       (void*)&mtu_discover, sizeof(mtu_discover));
            VPLTEST_CHK_OK(rv, "VPLSocket_SetSockOpt");
        }
#endif
        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_2);
        VPLTEST_LOG("Calling Bind on UDP socket.");
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        VPLTEST_CHK_OK(rv, "VPLSocket_Bind");

        HELPER_ASSERT_SOCK(socket, "VPLSocket_CreateUdp()");

        // Send UDP
        sock_addr_remote.family = VPL_PF_INET;
        sock_addr_remote.addr = localhost;
        sock_addr_remote.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        VPLTEST_LOG("Calling SendTo on UDP socket.");
        rv = VPLSocket_SendTo(socket, bytes, 10, &sock_addr_remote, sizeof(sock_addr_remote));
        HELPER_ASSERT_RV(rv, 10, "VPLSocket_SendTo()");

        // Receive UDP
        VPLTEST_LOG("Calling Recv on UDP socket.");
        rv = 0;
        while (rv < 10) {
            int rc = VPLSocket_Recv(socket, &bytes[rv], 10 - rv);
            if (rc >= 0) {
                rv += rc;
            } else if (rc == VPL_ERR_AGAIN) {
                VPLThread_Yield();
            } else {
                VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
                break;
            }
        }
        HELPER_ASSERT_RV(rv, 10, "VPLSocket_Recv()");

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Socket_Recv with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Recv(badSock, NULL, 0), VPL_ERR_NOTSOCK);
#endif
        // Test calling Socket_Recv with NULL buffer. Should get VPL_ERR_AGAIN.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Recv(socket, NULL, 0), VPL_ERR_AGAIN);

        // Close
        VPLTEST_LOG("Calling Close on UDP socket.");
        rv = VPLSocket_Close(socket);
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
    }
    else if (action < 3) {

        // Create TCP Socket
        int reuse = 1, nodelay = 1;
        VPLSocket_t socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*nonblock*/1 );
        HELPER_ASSERT_SOCK(socket, "VPLSocket_CreateTcp()");
        rv = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
                                   (void*)&reuse, sizeof(reuse));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }
        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
                                   (void*)&nodelay, sizeof(nodelay) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }
#ifndef VPL_PLAT_IS_WINRT
        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_1+action);
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_Bind with error code %d.", rv);
        }
#endif
        // Connect TCP
        sock_addr_remote.family = VPL_PF_INET;
        sock_addr_remote.addr = localhost;
        sock_addr_remote.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        rv = VPLSocket_Connect(socket, &sock_addr_remote, sizeof(sock_addr_remote));
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Connect()");

        // TODO FIXME: Calling connect again does not fail with VPL_ERR_ISCONN.
        //VPLThread_Sleep(10000000);
        //VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(socket, &sock_addr_remote, sizeof(sock_addr_remote)),
        //      VPL_ERR_ISCONN);
        // Test calling Socket_Connect with NULL remote address. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(socket, NULL, sizeof(sock_addr_remote)),
                VPL_ERR_INVALID);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Socket_Connect with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(badSock, &sock_addr_remote, sizeof(sock_addr_remote)),
                VPL_ERR_NOTSOCK);
#endif

        if (action == 1) {
            // Send TCP
            rv = 0;
            while (rv < 10) {
                int rc = VPLSocket_Send(socket, &bytes[rv], 10 - rv);
                if (rc >= 0) {
                    rv += rc;
                }
                else if (rc == VPL_ERR_AGAIN) {
                    VPLThread_Yield();
                }
                else {
                    VPLTEST_FAIL("VPLSocket_Send failed with error code: %d.", rc);
                }
            }
            HELPER_ASSERT_RV(rv, 10, "VPLSocket_Send()");
            // Receive TCP
            rv = 0;
            while ( rv < 10 ) {
                int rc = VPLSocket_Recv(socket, &bytes[rv], 10 - rv);
                if (rc >= 0) {
                    rv += rc;
                }
                else if (rc == VPL_ERR_AGAIN) {
                    VPLThread_Yield();
                }
                else {
                    VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
                    break;
                }
            }

            HELPER_ASSERT_RV(rv, 10, "VPLSocket_Recv()");
#ifndef IOS
            // On iOS, if the socket is closed on server-side, the following function returns fail.
            // Shutdown read
            rv = VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RD);
            HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");
#endif
            // Shutdown write
            rv = VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_WR);
            HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

            // Close
            rv = VPLSocket_Close(socket);
            HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
        }
        else {
            // TODO: how to cause connection abort?
            // Close
            rv = VPLSocket_Close(socket);
            HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
        }
    }
    else if (action == 3) {
        // Create TCP Socket
        int reuse = 1, nodelay = 1;
        VPLSocket_t socket = VPLSocket_Create( VPL_PF_INET, VPLSOCKET_STREAM, /*block*/0 );
        HELPER_ASSERT_SOCK(socket, "VPLSocket_CreateTcp()");

        //WinRT not support
#ifndef VPL_PLAT_IS_WINRT
        rv = VPLSocket_SetSockOpt(socket, VPLSOCKET_SOL_SOCKET, VPLSOCKET_SO_REUSEADDR,
            (void*)&reuse, sizeof(reuse));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }
#endif
        rv = VPLSocket_SetSockOpt( socket, VPLSOCKET_IPPROTO_TCP, VPLSOCKET_TCP_NODELAY,
            (void*)&nodelay, sizeof(nodelay) );
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_SetSockOpt failed with error code: %d.", rv);
        }

        //WinRT not support connect after bind
#ifndef VPL_PLAT_IS_WINRT
        sock_addr.family = VPL_PF_INET;
        sock_addr.addr = VPLNET_ADDR_ANY;
        sock_addr.port = VPLNet_port_hton(POLL_TEST_PORT_1+action);
        rv = VPLSocket_Bind(socket, &sock_addr, sizeof(sock_addr));
        if (rv != VPL_OK) {
            VPLTEST_FAIL("VPLSocket_Bind with error code %d.", rv);
        }
#endif

        // Connect TCP
        sock_addr_remote.family = VPL_PF_INET;
        sock_addr_remote.addr = localhost;
        sock_addr_remote.port = VPLNet_port_hton(POLL_TEST_PORT_1);
        rv = VPLSocket_Connect(socket, &sock_addr_remote, sizeof(sock_addr_remote));
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Connect()");

        // TODO FIXME: Calling connect again does not fail with VPL_ERR_ISCONN.
        //VPLThread_Sleep(10000000);
        //VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(socket, &sock_addr_remote, sizeof(sock_addr_remote)),
        //      VPL_ERR_ISCONN);
        // Test calling Socket_Connect with NULL remote address. Should get VPL_ERR_INVALID.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(socket, NULL, sizeof(sock_addr_remote)),
            VPL_ERR_INVALID);

#ifndef VPL_PLAT_IS_WINRT
        // Not applicable for WinRT (see badSock's declaration).
        // Test calling Socket_Connect with bad socket. Should get VPL_ERR_NOTSOCK.
        VPLTEST_CALL_AND_CHK_RV(VPLSocket_Connect(badSock, &sock_addr_remote, sizeof(sock_addr_remote)),
            VPL_ERR_NOTSOCK);
#endif

        //send
        rc = VPLSocket_Send(socket, &bytes[0], 10);
        if (rc < 0) {
            VPLTEST_FAIL("VPLSocket_Send failed with error code: %d.", rc);
        }
        HELPER_ASSERT_RV(rc, 10, "VPLSocket_Send()");

        //recv
        rc = VPLSocket_Recv(socket, &bytes[0], 10);
        if (rc < 0) {
            VPLTEST_FAIL("VPLSocket_Recv failed with error code: %d.", rc);
        }
        HELPER_ASSERT_RV(rc, 10, "VPLSocket_Recv()");

        // Shutdown read
        rv = VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_RD);
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

        // Shutdown write
        rv = VPLSocket_Shutdown(socket, VPLSOCKET_SHUT_WR);
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Shutdown()");

        // Close
        rv = VPLSocket_Close(socket);
        HELPER_ASSERT_RV(rv, VPL_OK, "VPLSocket_Close()");
    }
    rv = VPLMutex_Unlock(&pollMutex);
    HELPER_ASSERT_RV(rv, VPL_OK, "VPLMutex_Unlock()");

    return VPL_AS_THREAD_RETVAL(0);
}


