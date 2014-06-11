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

#include <google/protobuf/compiler/command_line_interface.h>

#include "ServiceGenerator.h"
#include "WsdlGenerator.h"
#include "XmlUtilsGenerator.h"
#include "XsdGenerator.h"

static void
extractExtraParams(int* argc, char* argv[], char** xsd_ns_out,
        char** wsdl_server_out, char** wsdl_port_out)
{
    for (int i = 0; i < *argc; i++) {
        if (strncmp(argv[i], "--xsd_ns=", 9) == 0) {
            *xsd_ns_out = argv[i] + 9;
            for (int j = i; j < *argc - 1; j++) {
                argv[j] = argv[j+1];
            }
            (*argc)--;
        }
        if (strncmp(argv[i], "--wsdl_server=", 14) == 0) {
            *wsdl_server_out = argv[i] + 14;
            for (int j = i; j < *argc - 1; j++) {
                argv[j] = argv[j+1];
            }
            (*argc)--;
        }
        if (strncmp(argv[i], "--wsdl_port=", 12) == 0) {
            *wsdl_port_out = argv[i] + 12;
            for (int j = i; j < *argc - 1; j++) {
                argv[j] = argv[j+1];
            }
            (*argc)--;
        }
    }
}

int
main(int argc, char* argv[])
{
    google::protobuf::compiler::CommandLineInterface cli;

    char* xsd_ns = NULL;
    char* wsdl_server = NULL;
    char* wsdl_port = NULL;
    extractExtraParams(&argc, argv, &xsd_ns, &wsdl_server, &wsdl_port);

    ServiceGenerator rpcClientGenerator(SERVICE_GENERATOR_TARGET_CLIENT);
    cli.RegisterGenerator("--client_out", &rpcClientGenerator,
            "Generate files for a blocking client (as C++).");
    
    ServiceGenerator rpcAsyncClientGenerator(SERVICE_GENERATOR_TARGET_ASYNC_CLIENT);
    cli.RegisterGenerator("--async_client_out", &rpcAsyncClientGenerator,
            "Generate files for an asynchronous client (as C++).");

    ServiceGenerator rpcServerGenerator(SERVICE_GENERATOR_TARGET_SERVER);
    cli.RegisterGenerator("--server_out", &rpcServerGenerator,
            "Generate files for the RPC server (as C++).");

    ServiceGenerator serviceGenerator(SERVICE_GENERATOR_TARGET_CLIENT | SERVICE_GENERATOR_TARGET_SERVER);
    cli.RegisterGenerator("--service_out", &serviceGenerator,
            "Generate files for RPC server and blocking client (as C++) in one pass.");

    ServiceGenerator rpcClientJavaGenerator(SERVICE_GENERATOR_TARGET_CLIENT_JAVA);
    cli.RegisterGenerator("--client_java_out", &rpcClientJavaGenerator,
            "Generate files for a blocking client (as Java).");
    
    ServiceGenerator rpcClientCSharpGenerator(SERVICE_GENERATOR_TARGET_CLIENT_CSHARP);
    cli.RegisterGenerator("--client_csharp_out", &rpcClientCSharpGenerator,
            "Generate files for a blocking client (as C#).");

    XmlUtilsGenerator xmlUtilsGenerator;
    cli.RegisterGenerator("--xmlu_out", &xmlUtilsGenerator,
            "Generate xml reader/writer files.");

    XsdGenerator xsdGenerator(xsd_ns);
    cli.RegisterGenerator("--xsd_out", &xsdGenerator,
            "Generate xsd file.");

    WsdlGenerator wsdlGenerator(xsd_ns, wsdl_server, wsdl_port);
    cli.RegisterGenerator("--wsdl_out", &wsdlGenerator,
            "Generate wsdl file.");

    return cli.Run(argc, argv);
}
