#include <vpl_types.h>
#include <vpl_time.h>
#include <vpl_th.h>
#include <vpl_socket.h>


#include <errno.h>

#include "gvmtest_load.h"
#include "microbench_profil.h"
#include "vpl_microbenchmark.h"  // shared parse_args()


/// Default number of smaples to collect.
#define NSAMPLES (1024 * 1024)
#define TEST_MAX_SOCKETS 1024



extern int bogus_test(void);


int
bogus_test(void)
{
    int i;
    VPLSocket_t sock;
    VPLSocket_t socks[TEST_MAX_SOCKETS];


    memset(&sock, 0, sizeof(sock));
    memset(&socks[0], 0, sizeof(socks));

    for (i = 0; i < TEST_MAX_SOCKETS; i++ ) {
        socks[i] = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, 0);
        if (VPLSocket_Equal(socks[i], VPLSOCKET_INVALID)) {
            fprintf(stderr, "VPL socket botch at iteration %d: %s\n", i, strerror(errno));
            break;
        }
        socks[i] = sock;
    }

    // try once more..
    sock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, 0);
    if (!VPLSocket_Equal(sock, VPLSOCKET_INVALID)) {
        sock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, 0);
        fprintf(stderr, "VPL socket botch: VPLSocket_Create() after max unexpectedly succeeds\n");
        return(2);
    }
    for (i = 0; i < TEST_MAX_SOCKETS; i++ ) {
        VPLSocket_Close(socks[i]);
    }
        
    return (0);
}

int
main(int argc, char *argv[])
{


    int nSamples = NSAMPLES;
    int samples_flags = 0;
    
    VPLSocket_t Socks[TEST_MAX_SOCKETS];
    int nSocks = 0;
    int i;

    VPLProfil_SampleVector_t *SocketCreate_timings = 0;
    VPLProfil_SampleVector_t *SocketClose_timings = 0;

    // Parse arguments...
    parse_args(argc, argv, &nSamples, &samples_flags);

    /// Time socket open...
    SocketCreate_timings = VPLProfil_SampleVector_Create(nSamples, "SocketCreate", samples_flags);
    nSocks = 0;
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        VPLSocket_t mySock;
        int reuseaddr_optval;
        VPLProfil_Sample_Start(&theSample);
        mySock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, 0);
        VPLProfil_Sample_End(&theSample);


        reuseaddr_optval = 1;
        (void) VPLSocket_SetSockOpt(mySock, VPLSOCKET_SOL_SOCKET,
                                        VPLSOCKET_SO_REUSEADDR,
                                        &reuseaddr_optval, sizeof(reuseaddr_optval));

        // save the sample...
        VPLProfil_SampleVector_Save(SocketCreate_timings, &theSample);

        VPLSocket_Close(mySock);
    }
    VPLProfil_SampleVector_Print(stdout, SocketCreate_timings);
    VPLProfil_SampleVector_Destroy(SocketCreate_timings);

    
    /// Time socket close...
    nSocks = 0;
    SocketClose_timings = VPLProfil_SampleVector_Create(nSamples, "SocketClose", samples_flags);
    for (i = 0;  i < nSamples; i++) {
        struct sample theSample;
        VPLSocket_t mySock;
        int reuseaddr_optval;

        mySock = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_STREAM, 0);

        // Issue VPLSOCKET_REUSEADDR against the socket.
        // If we don't, the (TCP) socket may stay in "linger" state,
        // and so still be charged against this processs' open-fd limit.
        reuseaddr_optval = 1;
        (void)  VPLSocket_SetSockOpt(mySock, VPLSOCKET_SOL_SOCKET,
                                        VPLSOCKET_SO_REUSEADDR,
                                        &reuseaddr_optval, sizeof(reuseaddr_optval));

        VPLProfil_Sample_Start(&theSample);
        VPLSocket_Close(mySock);
        VPLProfil_Sample_End(&theSample);

        // save the sample...
        VPLProfil_SampleVector_Save(SocketClose_timings, &theSample);

    }
    // close any leftover sockets...
    for (i = 0; i < nSocks; i++) {
        (void)VPLSocket_Close(Socks[i]);
    }
    VPLProfil_SampleVector_Print(stdout, SocketClose_timings);
    VPLProfil_SampleVector_Destroy(SocketClose_timings);


    // time socket bind()...
    SocketCreate_timings = VPLProfil_SampleVector_Create(nSamples, "SocketBind", samples_flags);
    for (i = 0; i < nSamples; i++) {
        struct sample theSample;
        VPLSocket_t sockfd;
        int reuseaddr_optval = 1;
        VPLSocket_addr_t bound_addr;
        int status;

        sockfd  = VPLSocket_Create(VPL_PF_INET, VPLSOCKET_DGRAM, 0);
        if (sockfd.fd == VPLSOCKET_INVALID.fd) {
            fprintf(stderr, "bind socket-create botch at iter %d: %s\n", i, strerror(errno));
            break;
        }
        
        status  =  VPLSocket_SetSockOpt(sockfd, VPLSOCKET_SOL_SOCKET,
                                    VPLSOCKET_SO_REUSEADDR,
                                        &reuseaddr_optval, sizeof(reuseaddr_optval));
        if (status != VPL_OK) {
            fprintf(stderr, "setsockopt botch at iter %d: %s\n", i, strerror(errno));
            break;
        }
        bound_addr.family = VPL_PF_INET;
        bound_addr.addr = VPLNET_ADDR_ANY;
        bound_addr.port = 42042; // random address...

        VPLProfil_Sample_Start(&theSample);
        status = VPLSocket_Bind(sockfd, &bound_addr, sizeof(bound_addr));
        VPLProfil_Sample_End(&theSample);

        if (status != 0) {
            fprintf(stderr, "VPL error: %s\n", strerror(errno));
        }

        // save the sample...
        VPLProfil_SampleVector_Save(SocketCreate_timings, &theSample);

        VPLSocket_Close(sockfd);
    }

    VPLProfil_SampleVector_Print(stdout, SocketCreate_timings);
    VPLProfil_SampleVector_Destroy(SocketCreate_timings);

    return 0;
}
