/* Get AcerCloud access tokens (x-ac-userId, x-ac-sessionHandle, x-ac-serviceTicket) from Infra.
 */

#include "vplex_vs_directory.h"
#include "vplex_ias.hpp"
#include "vplex_serialization.h"
#include "log.h"
#include "csltypes.h"
#include "cslsha.h"
#include "vpl_conv.h"
#include "vpl_user.h"
#include <gvm_misc_utils.h>

#include <string>
#include <iostream>
#include <sstream>

static std::string default_username = "user";
static std::string default_password = "password";
static std::string default_iashost = "www-c100.pc-int.igware.net";
static u16 default_iasport = 443;
static std::string default_vsdshost = "www-c100.pc-int.igware.net";
static u16 default_vsdsport = 443;

//------------------------------------------------------------
// code from vsTest_infra.cpp (with minor modifications)

static std::string makeMessageId()
{
    std::stringstream stream;
    VPLTime_t currTimeMillis = VPLTIME_TO_MILLISEC(VPLTime_GetTime());
    stream << "vsTest-" << currTimeMillis;
    return std::string(stream.str());
}

template<class RequestT>
static void
setIasAbstractRequestFields(RequestT& request)
{
    request.mutable__inherited()->set_version("2.0");
    request.mutable__inherited()->set_country("US");
    request.mutable__inherited()->set_language("en");
    request.mutable__inherited()->set_region("US");
    request.mutable__inherited()->set_messageid(makeMessageId());
}

static int userLogin(const std::string& ias_name, u16 port, 
               const std::string& user, const std::string& ns, 
               const std::string& pass,
               u64& uid, vplex::vsDirectory::SessionInfo& session)
{
    int rc, rv = 0;
    VPLIas_ProxyHandle_t iasproxy;
    vplex::ias::LoginRequestType loginReq;
    vplex::ias::LoginResponseType loginRes;

    // Testing VSDS login operation
    // Use the VPLIAS API to log in to infrastructure.
    // Relying on VPL unit testing to cover testing for login.
    rv = VPLIas_CreateProxy(ias_name.c_str(), port, &iasproxy);
    if (rv != 0) {
        LOG_ERROR("VPLIas_CreateProxy returned %d.", rv);
        rv++;
        goto fail;
    }
    
    setIasAbstractRequestFields(loginReq);
    loginReq.set_username(user);
    loginReq.set_namespace_(ns);
    loginReq.set_password(pass);
    rc = VPLIas_Login(iasproxy, VPLTIME_FROM_SEC(30), loginReq, loginRes);
    if(rc != 0) {
        LOG_ERROR("FAIL:"
                         "Failed login: %d", rc);
        rv++;
        goto fail;
    }
    else {
        VPLUser_SessionSecret_t session_secret;
        std::string service = "Virtual Storage";
        size_t encode_len = VPL_BASE64_ENCODED_SINGLE_LINE_BUF_LEN(sizeof(session_secret));
        char secret_data_encoded[encode_len];
        VPLUser_ServiceTicket_t ticket_data;
        CSL_ShaContext context;

        session.set_sessionhandle(loginRes.sessionhandle());

        memcpy(session_secret, loginRes.sessionsecret().data(),
               VPL_USER_SESSION_SECRET_LENGTH);
        // Compute service ticket like so:
        // ticket = SHA1(base64(session_secret) + "Virtual Storage")
        VPL_EncodeBase64(session_secret, sizeof(session_secret),
                         secret_data_encoded, &encode_len, false, false);
        CSL_ResetSha(&context);
        CSL_InputSha(&context, secret_data_encoded, encode_len - 1); // remove terminating NULL
        CSL_InputSha(&context, service.data(), service.size());
        CSL_ResultSha(&context, (unsigned char*)ticket_data);
        session.set_serviceticket(ticket_data, VPL_USER_SERVICE_TICKET_LENGTH);
        uid = loginRes.userid();
    }

 fail:
    rc = VPLIas_DestroyProxy(iasproxy);
    if(rc != 0) {
        LOG_ERROR("FAIL:"
                         "Failed to destroy IAS proxy: %d", rc);
        rv++;
    }

    return rv;
}

// end of code from vsTest_infra.cpp
//------------------------------------------------------------


static void print_usage_and_exit(const char *progname)
{
    std::cerr
        << "Usage: " << progname << " [options]" << std::endl
        << "\t[-u username]\t\t(default is \"user\")" << std::endl
        << "\t[-p password]\t\t(default is \"password\")" << std::endl
        << "\t[-I host[:port]]\tIAS (default is " << default_iashost << ":" << default_iasport << ")" << std::endl
        << "\t[-V host[:port]]\tVSDS (default is " << default_vsdshost << ":" << default_vsdsport << ")" << std::endl;
    exit(0);
}

class Args {
public:
    Args(int argc, char *argv[]);
    std::string username;
    std::string password;
    std::string iashost;
    u16 iasport;
    std::string vsdshost;
    u16 vsdsport;
private:
    Args();  // hide default constructor
};

static void parse_host_port(const char *host_port, std::string &host, u16 &port)
{
    const char *colonPos = strchr(host_port, ':');
    if (colonPos) {
        host.assign(host_port, 0, colonPos - host_port);
        port = atoi(colonPos+1);
    }
    else {
        host.assign(host_port);
        // no change to port
    }
}

Args::Args(int argc, char *argv[]) : 
    username(default_username), password(default_password),
    iashost(default_iashost), iasport(default_iasport),
    vsdshost(default_vsdshost), vsdsport(default_vsdsport)
{
    if (argc == 1) print_usage_and_exit(argv[0]);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'u':
                if (i+1 < argc)
                    username = argv[++i];
                else
                    LOG_ERROR("-i missing argument");
                break;
            case 'p':
                if (i+1 < argc)
                    password = argv[++i];
                else
                    LOG_ERROR("-p missing argument");
                break;
            case 'I':
                if (i+1 < argc) {
                    i++;
                    parse_host_port(argv[i], iashost, iasport);
                }
                else {
                    LOG_ERROR("-I missing argument");
                }
                break;
            case 'V':
                if (i+1 < argc) {
                    i++;
                    parse_host_port(argv[i], vsdshost, vsdsport);
                }
                else {
                    LOG_ERROR("-V missing argument");
                }
                break;
            default:
                LOG_ERROR("Unknown flag %s on command line", argv[i]);
            }
        }
        else {
            LOG_ERROR("Unexpected word %s on command line", argv[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    LOGInit("GetAcerCloudAccessTokens", NULL);
    LOGSetMax(0); // No limit
    VPL_Init();

    Args args(argc, argv);

    int rc = 0;
    VPLUser_Id_t uid = 0;
    vplex::vsDirectory::SessionInfo session;

    rc = userLogin(args.iashost, args.iasport, args.username, "acer", args.password, uid, session);
    if (rc != 0) {
        LOG_ERROR("userLogin failed: %d", rc);
        exit(1);
    }

    std::cout << "user_id: \"" << uid << "\"" << std::endl;

    s64 sessionHandle = (s64)session.sessionhandle();
    std::cout << "session_handle: \"" << sessionHandle << "\"" << std::endl;

    char *serviceTicket = NULL;
    Util_EncodeBase64(session.serviceticket().data(), session.serviceticket().size(),
                      &serviceTicket, NULL, false, false);
                     
    std::cout << "service_ticket: \"" << serviceTicket << "\"" << std::endl;

    exit(rc);
}

