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

#include "ServiceGenerator.h"

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/compiler/java/java_helpers.h>

#include "GeneratorUtils.h"

using google::protobuf::FileDescriptor;
using google::protobuf::io::ZeroCopyOutputStream;

#define ENDL "\n"

//----------------------------------------------
// Magic strings to replace with dynamic content in output
static const char* IDENT_PREFIX = ">>>";
static const int IDENT_LEN = 8;
#define PROTO_NAME_NO_PATH ">>>00<<<"
#define INCLUDE_GUARD      ">>>01<<<"
#define NAMESPACES_START   ">>>02<<<"
#define NAMESPACES_END     ">>>03<<<"
#define JAVA_PACKAGE       ">>>04<<<"
#define JAVA_OUTER_CLASS   ">>>05<<<"

#define SERVICE_NAME       ">>>10<<<"
#define SERVICE_NAME_FULL  ">>>11<<<"

#define METHOD_NAME               ">>>20<<<"
#define METHOD_INPUT_TYPE_CPP     ">>>21<<<"
#define METHOD_OUTPUT_TYPE_CPP    ">>>22<<<"
#define METHOD_INPUT_TYPE_JAVA    ">>>23<<<"
#define METHOD_OUTPUT_TYPE_JAVA   ">>>24<<<"
#define METHOD_INPUT_TYPE_CSHARP  ">>>25<<<"
#define METHOD_OUTPUT_TYPE_CSHARP ">>>26<<<"

//----------------------------------------------

ServiceGenerator::ServiceGenerator(ServiceGeneratorTarget targets) : targets(targets)
{}

ServiceGenerator::~ServiceGenerator(void)
{}

bool
ServiceGenerator::Generate(const FileDescriptor* file,
        const string& parameter, OutputDirectory* outputDirectory,
        string* error) const
{
    ServiceGeneratorHelper helper(file, parameter, targets, outputDirectory, *error);
    return helper.generate();
}

ServiceGeneratorHelper::ServiceGeneratorHelper(
        const FileDescriptor* _file, const string& _parameter,
        ServiceGeneratorTarget targets,
        OutputDirectory* _outputDirectory, string& _error)
    : file(_file), parameter(_parameter), outputDirectory(_outputDirectory),
      error(_error), protoFilenameNoExt(), protoFilenameNoExtNoPath(), targets(targets),
      cliHFile(NULL), cliCcFile(NULL), cliJavaFile(NULL), cliCSharpFile(NULL),
      asyncCliHFile(NULL), asyncCliCcFile(NULL),
      srvHFile(NULL), srvCcFile(NULL)
{}

ServiceGeneratorHelper::~ServiceGeneratorHelper(void)
{
    delete cliHFile;
    delete cliCcFile;
    delete cliJavaFile;
    delete cliCSharpFile;
    delete asyncCliHFile;
    delete asyncCliCcFile;
    delete srvHFile;
    delete srvCcFile;
}

bool
ServiceGeneratorHelper::generate(void)
{
    bool success = true;
    try {
        // Do not generate any output if no services.
        if (file->service_count() > 0) {
            createOutputFiles();
            if ((targets & SERVICE_GENERATOR_TARGET_CLIENT) != 0) {
                writeClientHeader();
                writeClientSource();
            }
            if ((targets & SERVICE_GENERATOR_TARGET_ASYNC_CLIENT) != 0) {
                writeAsyncClientHeader();
                writeAsyncClientSource();
            }
            if ((targets & SERVICE_GENERATOR_TARGET_SERVER) != 0) {
                writeServerHeader();
                writeServerSource();
            }
            if ((targets & SERVICE_GENERATOR_TARGET_CLIENT_JAVA) != 0) {
                writeClientJava();
            }
            if ((targets & SERVICE_GENERATOR_TARGET_CLIENT_CSHARP) != 0) {
                writeClientCSharp();
            }
        } else {
            error = "No services found in .proto file!";
            success = false;
        }
    } catch (string& e) {
        error = e;
        success = false;
    }
    return success;
}

void
ServiceGeneratorHelper::createOutputFiles(void)
{
    protoFilenameNoExt = stripExtension(file->name());
    protoFilenameNoExtNoPath = stripPath(protoFilenameNoExt);

    if ((targets & SERVICE_GENERATOR_TARGET_CLIENT) != 0) {
        string cliHFilename = protoFilenameNoExt;
        cliHFilename.append("-client.pb.h");
        cliHFile = outputDirectory->Open(cliHFilename);

        string cliCcFilename = protoFilenameNoExt;
        cliCcFilename.append("-client.pb.cc");
        cliCcFile = outputDirectory->Open(cliCcFilename);
    }

    if ((targets & SERVICE_GENERATOR_TARGET_CLIENT_JAVA) != 0) {
        // The filename needs to exactly match the top-level class, otherwise
        // the Java compiler rejects it.
        string filename = getJavaPackagePath(file);
        filename.append(google::protobuf::compiler::java::FileClassName(file));
        filename.append("Client.java");
        cliJavaFile = outputDirectory->Open(filename);
    }

    if ((targets & SERVICE_GENERATOR_TARGET_CLIENT_CSHARP) != 0) {
        string filename = protoFilenameNoExt;
        filename.append("-client.pb.cs");
        cliCSharpFile = outputDirectory->Open(filename);
    }

    if ((targets & SERVICE_GENERATOR_TARGET_ASYNC_CLIENT) != 0) {
        string asyncCliHFilename = protoFilenameNoExt;
        asyncCliHFilename.append("-client-async.pb.h");
        asyncCliHFile = outputDirectory->Open(asyncCliHFilename);

        string asyncCliCcFilename = protoFilenameNoExt;
        asyncCliCcFilename.append("-client-async.pb.cc");
        asyncCliCcFile = outputDirectory->Open(asyncCliCcFilename);
    }

    if ((targets & SERVICE_GENERATOR_TARGET_SERVER) != 0) {
        string srvHFilename = protoFilenameNoExt;
        srvHFilename.append("-server.pb.h");
        srvHFile = outputDirectory->Open(srvHFilename);

        string srvCcFilename = protoFilenameNoExt;
        srvCcFilename.append("-server.pb.cc");
        srvCcFile = outputDirectory->Open(srvCcFilename);
    }
}

void
ServiceGeneratorHelper::writeClientHeader(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT\n" \
            "\n" \
            "#ifndef "INCLUDE_GUARD"\n" \
            "#define "INCLUDE_GUARD"\n" \
            "\n" \
            "#include \""PROTO_NAME_NO_PATH".pb.h\"\n" \
            "#include <rpc.pb.h>\n" \
            "#include <ProtoChannel.h>\n" \
            "#include <ProtoRpcClient.h>\n" \
            "\n" \
            NAMESPACES_START \
            "\n");
    static const string serviceContentPre(
            "class "SERVICE_NAME"Client\n" \
            "{\n" \
            "public:\n" \
            "    "SERVICE_NAME"Client(ProtoChannel& channel);\n");
    static const string methodContent(
            "\n" \
            "    // Implemented by #"SERVICE_NAME"."METHOD_NAME"() \n" \
            "    void "METHOD_NAME"(const "METHOD_INPUT_TYPE_CPP"& request, " \
            METHOD_OUTPUT_TYPE_CPP"& response, RpcStatus& status);\n");
    static const string serviceContentPost(
            "\n" \
            "private:\n" \
            "    ProtoRpcClient _client;\n" \
            "};\n" \
            "\n");
    static const string mainContentPost(
            NAMESPACES_END \
            "\n" \
            "#endif /* "INCLUDE_GUARD" */\n");

    string includeGuard;
    includeGuard.append("__").append(protoFilenameNoExtNoPath).append("_CLIENT_PB_H__");
    writeFileContents(cliHFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, includeGuard);
}

void
ServiceGeneratorHelper::writeClientSource(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT\n" \
            "\n" \
            "#include \""PROTO_NAME_NO_PATH"-client.pb.h\"\n" \
            "#include <string>\n" \
            "#include \""PROTO_NAME_NO_PATH".pb.h\"\n" \
            "#include <rpc.pb.h>\n" \
            "#include <ProtoChannel.h>\n" \
            "#include <ProtoRpcClient.h>\n" \
            "\n" \
            "using std::string;\n" \
            "\n");
    static const string serviceContentPre(
            SERVICE_NAME_FULL"Client::"SERVICE_NAME"Client(ProtoChannel& channel)\n" \
            "    : _client(channel)\n" \
            "{}\n" \
            "\n");
    static const string methodContent(
            "void\n" \
            SERVICE_NAME_FULL"Client::"METHOD_NAME"(const "METHOD_INPUT_TYPE_CPP \
            "& request, "METHOD_OUTPUT_TYPE_CPP"& response, RpcStatus& status)\n" \
            "{\n" \
            "    static const string method(\""METHOD_NAME"\");\n" \
            "    _client.sendRpc(method, request, response, status);\n" \
            "}\n" \
            "\n");
    static const string serviceContentPost;
    static const string mainContentPost;

    string unused;
    writeFileContents(cliCcFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, unused);
}

void
ServiceGeneratorHelper::writeClientJava(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT" ENDL
            ENDL
            "package "JAVA_PACKAGE";" ENDL
            ENDL
            "public class "JAVA_OUTER_CLASS"Client {" ENDL
            ENDL
            );
    static const string serviceContentPre(
            "    public static class "SERVICE_NAME"Client extends igware.protobuf.ProtoRpcClient {" ENDL
            ENDL
            "        /**" ENDL
            "         * Allows sending RPC requests to an instance of "SERVICE_NAME"." ENDL
            "         * Please see {@link igware.protobuf.ProtoRpcClient}." ENDL
            "         */" ENDL
            "        public "SERVICE_NAME"Client(igware.protobuf.ProtoChannel channel, boolean throwOnAppError) {" ENDL
            "            super(channel, throwOnAppError);" ENDL
            "        }" ENDL
            ENDL
            );
    static const string methodContent(
            "        public int "METHOD_NAME"(" ENDL
            "                "METHOD_INPUT_TYPE_JAVA" request," ENDL
            "                "METHOD_OUTPUT_TYPE_JAVA".Builder responseBuilder)" ENDL
            "        throws igware.protobuf.ProtoRpcException {" ENDL
            "            return sendRpc(\""METHOD_NAME"\", request, responseBuilder);" ENDL
            "        }" ENDL
            ENDL
            );
    static const string serviceContentPost(
            "    }" ENDL
            ENDL
            );
    static const string mainContentPost(
            "}" ENDL
            );

    string unused;
    writeFileContents(cliJavaFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, unused);
}

void
ServiceGeneratorHelper::writeClientCSharp(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT" ENDL
            ENDL
            NAMESPACES_START
            ENDL
            );
    static const string serviceContentPre(
            "    public class "SERVICE_NAME"Client : protorpc.ProtoRpcClient {" ENDL
            ENDL
            "        /**" ENDL
            "         * Allows sending RPC requests to an instance of "SERVICE_NAME"." ENDL
            "         * Please see protorpc.ProtoRpcClient." ENDL
            "         */" ENDL
            "        public "SERVICE_NAME"Client(protorpc.ProtoChannel channel, bool throwOnAppError)" ENDL
            "            : base(channel, throwOnAppError)" ENDL
            "        {" ENDL
            "        }" ENDL
            ENDL
            );
    static const string methodContent(
            "        public int "METHOD_NAME"(" ENDL
            "                "METHOD_INPUT_TYPE_CSHARP" request," ENDL
            "                ref "METHOD_OUTPUT_TYPE_CSHARP" response)" ENDL
            "        {" ENDL
            "            object reference = response;" ENDL
            "            int rv = sendRpc(\""METHOD_NAME"\", request, response.GetType(), ref reference);" ENDL
            "            response = ("METHOD_OUTPUT_TYPE_CSHARP")reference;" ENDL
            "            return rv;" ENDL
            "        }" ENDL
            ENDL
            );
    static const string serviceContentPost(
            "    }" ENDL
            ENDL
            );
    static const string mainContentPost(
            NAMESPACES_END
            );

    string unused;
    writeFileContents(cliCSharpFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, unused);
}

void
ServiceGeneratorHelper::writeServerHeader(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT\n"
            "\n"
            "#ifndef "INCLUDE_GUARD"\n"
            "#define "INCLUDE_GUARD"\n"
            "\n"
            "#include \""PROTO_NAME_NO_PATH".pb.h\"\n"
            "#include <ProtoChannel.h>\n"
            "#include <ProtoRpc.h>\n"
            "\n"
            NAMESPACES_START
            "\n");
    static const string serviceContentPre(
            "class "SERVICE_NAME"\n"
            "{\n"
            "public:\n"
            "    "SERVICE_NAME"() {}\n"
            "    virtual ~"SERVICE_NAME"(void) {}\n"
            "    bool handleRpc(ProtoChannel& channel,\n"
            "                   DebugMsgCallback debugCallback = NULL,\n"
            "                   DebugRequestMsgCallback debugRequestCallback = NULL,\n"
            "                   DebugResponseMsgCallback debugResponseCallback = NULL);\n"
            "\n"
            "protected:\n");
    static const string methodContent(
            "    virtual google::protobuf::int32 "METHOD_NAME"(const "METHOD_INPUT_TYPE_CPP \
            "& request, "METHOD_OUTPUT_TYPE_CPP"& response) = 0;\n");
    static const string serviceContentPost(
            "};\n" \
            "\n");
    static const string mainContentPost(
            NAMESPACES_END \
            "\n" \
            "#endif /* "INCLUDE_GUARD" */\n");

    string includeGuard;
    includeGuard.append("__").append(protoFilenameNoExtNoPath).append("_SERVER_PB_H__");
    writeFileContents(srvHFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, includeGuard);
}

void
ServiceGeneratorHelper::writeServerSource(void)
{
    static const string mainContentPre(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT\n" \
            "\n" \
            "#include \""PROTO_NAME_NO_PATH"-server.pb.h\"\n" \
            "#include <memory>\n" \
            "#include <string>\n" \
            "#include \""PROTO_NAME_NO_PATH".pb.h\"\n" \
            "#include <rpc.pb.h>\n" \
            "#include <ProtoChannel.h>\n" \
            "#include <ProtoRpcServer.h>\n" \
            "\n" \
            "using std::string;\n" \
            "\n");
    static const string serviceContentPre(
            "bool\n"
            SERVICE_NAME_FULL"::handleRpc(ProtoChannel& _channel,\n"
            "                             DebugMsgCallback debugCallback,\n"
            "                             DebugRequestMsgCallback debugRequestCallback,\n"
            "                             DebugResponseMsgCallback debugResponseCallback)\n"
            "{\n"
            "    bool success = true;\n"
            "    RpcRequestHeader header;\n"
            "    RpcStatus status;\n"
            "    std::auto_ptr<ProtoMessage> response;\n"
            "    do {\n"
            "        if (_channel.inputStreamError() || _channel.outputStreamError()) {\n"
            "            break;\n"
            "        }\n"
            "        if (!_channel.extractMessage(header)) {\n"
            "            if (!_channel.inputStreamError()) {\n"
            "                status.set_status(RpcStatus::HEADER_ERROR);\n"
            "                string error(\"Error in request header: \");\n"
            "                error.append(header.InitializationErrorString());\n"
            "                status.set_errordetail(error);\n"
            "            }\n"
            "            break;\n"
            "        }\n"
            "        debugLogInvoke(debugCallback, header.methodname());\n"
            "        ");
    static const string methodContent(
            "if (header.methodname().compare(\""METHOD_NAME"\") == 0) {\n"
            "            "METHOD_INPUT_TYPE_CPP" request;\n"
            "            if (!topBoilerplate(_channel, request, status, debugRequestCallback, header)) { break; }\n"
            "            "METHOD_OUTPUT_TYPE_CPP"* typedResponse = new "METHOD_OUTPUT_TYPE_CPP"();\n"
            "            response.reset(typedResponse);\n"
            "            status.set_appstatus("METHOD_NAME"(request, *typedResponse));\n"
            "            bottomBoilerplate(*response, status);\n"
            "            break;\n"
            "        } else ");
    static const string serviceContentPost(
            "{\n" \
            "            status.set_status(RpcStatus::UNKNOWN_METHOD);\n" \
            "            string error(\"Unrecognized method: \");\n" \
            "            error.append(header.methodname());\n" \
            "            status.set_errordetail(error);\n" \
            "            break;\n" \
            "        }\n" \
            "    } while (false);\n" \
            "\n" \
            "    if (status.has_status()) {\n" \
            "        if (!_channel.writeMessage(status, debugCallback)) {\n" \
            "            success = false;\n" \
            "        } else {\n" \
            "            debugLogResponse(debugResponseCallback, header.methodname(), status, response.get());\n"
            "            if ((status.status() == RpcStatus::OK) && (status.appstatus() >= 0)) {\n"
            "                if (!_channel.writeMessage(*response, debugCallback)) {\n" \
            "                    success = false;\n" \
            "                }\n" \
            "            }\n" \
            "        }\n" \
            "        if (success) {\n" \
            "            success = _channel.flushOutputStream();\n" \
            "        }\n" \
            "    }\n" \
            "\n" \
            "    return success;\n" \
            "}\n" \
            "\n");
    static const string mainContentPost;

    string unused;
    writeFileContents(srvCcFile, mainContentPre, serviceContentPre,
            methodContent, serviceContentPost, mainContentPost, unused);
}

void
ServiceGeneratorHelper::writeAsyncClientHeader(void)
{
    static const string beforeServices(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT" ENDL
            ENDL
            "#ifndef "INCLUDE_GUARD ENDL
            "#define "INCLUDE_GUARD ENDL
            ENDL
            "#include \""PROTO_NAME_NO_PATH"-client.pb.h\"" ENDL
            "#include <rpc.pb.h>" ENDL
            "#include <ProtoChannel.h>" ENDL
            "#include <ProtoRpcClientAsync.h>" ENDL
            ENDL
            NAMESPACES_START
            ENDL
            );
    static const string serviceBeforeMethods(
            "class "SERVICE_NAME"ClientAsync : public proto_rpc::ServiceClientAsync" ENDL
            "{" ENDL
            "public:" ENDL
            "    /// The #AsyncServiceClientState can safely be shared by multiple service clients." ENDL
            "    "SERVICE_NAME"ClientAsync(" ENDL
            "            proto_rpc::AsyncServiceClientState& asyncState," ENDL
            "            AcquireClientCallback acquireClient," ENDL
            "            ReleaseClientCallback releaseClient);" ENDL
            ENDL
            "    /// Retrieve and process the response for a completed request." ENDL
            "    /// This method will dequeue the next available response and invoke the" ENDL
            "    /// callback that was specified when the request was made." ENDL
            "    /// The callback will be invoked in this thread's context." ENDL
            "    /// If no results are ready, this will immediately return." ENDL
            "    int ProcessAsyncResponse();" ENDL
            ENDL
            );
    static const string eachMethod(
            "    // ---------------------------------------" ENDL
            "    // "METHOD_NAME ENDL
            "    // ---------------------------------------" ENDL
            "    typedef void (*"METHOD_NAME"Callback)(int requestId, void* param," ENDL
            "            "METHOD_OUTPUT_TYPE_CPP"& response, RpcStatus& status);" ENDL
            ENDL
            "    /// Asynchronously call the RPC implemented by #"SERVICE_NAME"."METHOD_NAME"()." ENDL
            "    /// @return positive requestId if the request was successfully queued," ENDL
            "    ///     or a negative error code if we couldn't queue the request." ENDL
            "    int "METHOD_NAME"Async(const "METHOD_INPUT_TYPE_CPP"& request," ENDL
            "            "METHOD_NAME"Callback callback, void* callbackParam);" ENDL
            ENDL
            );
    static const string serviceAfterMethods(
            "};" ENDL
            ENDL
            );
    static const string afterServices(
            NAMESPACES_END
            ENDL
            "#endif /* "INCLUDE_GUARD" */" ENDL
            );
    
    string includeGuard;
    includeGuard.append("__").append(protoFilenameNoExtNoPath).append("_CLIENT_ASYNC_PB_H__");
    writeFileContents(asyncCliHFile, beforeServices, serviceBeforeMethods,
            eachMethod, serviceAfterMethods, afterServices, includeGuard);
}

void
ServiceGeneratorHelper::writeAsyncClientSource(void)
{
    static const string beforeServices(
            "// GENERATED FROM "PROTO_NAME_NO_PATH".proto, DO NOT EDIT" ENDL
            ENDL
            "#include \""PROTO_NAME_NO_PATH"-client-async.pb.h\"" ENDL
            ENDL
            "using std::string;" ENDL
            ENDL
            NAMESPACES_START
            ENDL
            );
    static const string serviceBeforeMethods(
            "// -----------------------------------------------------------------------" ENDL
            "// "SERVICE_NAME_FULL ENDL
            "// -----------------------------------------------------------------------" ENDL
            SERVICE_NAME_FULL"ClientAsync::"SERVICE_NAME"ClientAsync(proto_rpc::AsyncServiceClientState& asyncState, AcquireClientCallback acquireClient, ReleaseClientCallback releaseClient)" ENDL
            "    : ServiceClientAsync(asyncState, acquireClient, releaseClient)" ENDL
            "{}" ENDL
            ENDL
            "int" ENDL
            SERVICE_NAME_FULL"ClientAsync::ProcessAsyncResponse()" ENDL
            "{" ENDL
            "    return _serviceState.processResult();" ENDL
            "}" ENDL
            ENDL
            );
    static const string eachMethod(
            "// ---------------------------------------" ENDL
            "// "METHOD_NAME ENDL
            "// ---------------------------------------" ENDL
            "static void "SERVICE_NAME"_"METHOD_NAME"_deliverFunc(" ENDL
            "        proto_rpc::AsyncCallState* resultObj)" ENDL
            "{" ENDL
            "    "SERVICE_NAME_FULL"ClientAsync::"METHOD_NAME"Callback callback =" ENDL
            "            ("SERVICE_NAME_FULL"ClientAsync::"METHOD_NAME"Callback)(resultObj->actualCallback);" ENDL
            "    callback(resultObj->requestId, resultObj->actualCallbackParam," ENDL
            "            ("METHOD_OUTPUT_TYPE_CPP"&)(resultObj->getResponse()), resultObj->status);" ENDL
            "}" ENDL
            ENDL
            "typedef proto_rpc::AsyncCallStateImpl<"METHOD_INPUT_TYPE_CPP", "METHOD_OUTPUT_TYPE_CPP">" ENDL
            "        "METHOD_NAME"AsyncState;" ENDL
            ENDL
            "static bool "SERVICE_NAME"_"METHOD_NAME"_send(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)" ENDL
            "{" ENDL
            "    "METHOD_NAME"AsyncState* state = ("METHOD_NAME"AsyncState*)arg;" ENDL
            "    try {" ENDL
            "        static const string method(\""METHOD_NAME"\");" ENDL
            "        client.sendRpcRequest(method, state->getRequest(), state->status);" ENDL
            "    } catch (...) {" ENDL
            "        state->status.set_status(RpcStatus::INTERNAL_ERROR);" ENDL
            "        return false;" ENDL
            "    }" ENDL
            "    return true;" ENDL
            "}" ENDL
            ENDL
            "static void "SERVICE_NAME"_"METHOD_NAME"_recv(proto_rpc::AsyncCallState* arg, ProtoRpcClient& client)" ENDL
            "{" ENDL
            "    "METHOD_NAME"AsyncState* state = ("METHOD_NAME"AsyncState*)arg;" ENDL
            "    try {" ENDL
            "        client.recvRpcResponse(state->getResponse(), state->status);" ENDL
            "    } catch (...) {" ENDL
            "        state->status.set_status(RpcStatus::INTERNAL_ERROR);" ENDL
            "    }" ENDL
            "}" ENDL
            ENDL
            "int" ENDL
            SERVICE_NAME_FULL"ClientAsync::"METHOD_NAME"Async(const "METHOD_INPUT_TYPE_CPP"& request," ENDL
            "        "METHOD_NAME"Callback callback, void* callbackParam)" ENDL
            "{" ENDL
            "    int requestId = _serviceState.nextRequestId();" ENDL
            "    if (requestId >= 0) {" ENDL
            "        "METHOD_NAME"AsyncState* asyncState =" ENDL
            "                new "METHOD_NAME"AsyncState(requestId, this, request," ENDL
            "                        "SERVICE_NAME"_"METHOD_NAME"_send," ENDL
            "                        "SERVICE_NAME"_"METHOD_NAME"_recv," ENDL
            "                        "SERVICE_NAME"_"METHOD_NAME"_deliverFunc," ENDL
            "                        (proto_rpc::AsyncCallState::AnyFunc)callback, callbackParam);" ENDL
            "        int rv = _serviceState.enqueueRequest(asyncState);" ENDL
            "        if (rv < 0) {" ENDL
            "            requestId = rv;" ENDL
            "            delete asyncState;" ENDL
            "        }" ENDL
            "    }" ENDL
            "    return requestId;" ENDL
            "}" ENDL
            ENDL
            );
    static const string serviceAfterMethods;
    static const string afterServices(
            NAMESPACES_END
            ENDL
            );

    string unused;
    writeFileContents(asyncCliCcFile, beforeServices, serviceBeforeMethods,
            eachMethod, serviceAfterMethods, afterServices, unused);
}

void
ServiceGeneratorHelper::writeFileContents(ZeroCopyOutputStream* out,
        const string& beforeServices,
        const string& serviceBeforeMethods,
        const string& method,
        const string& serviceAfterMethods,
        const string& afterServices,
        const string& includeGuard)
{
    appendToFile(out, beforeServices, NULL, NULL, includeGuard);
    for (int s = 0; s < file->service_count(); s++) {
        const ServiceDescriptor* service = file->service(s);
        appendToFile(out, serviceBeforeMethods, service, NULL, includeGuard);
        for (int m = 0; m < service->method_count(); m++) {
            const MethodDescriptor* methodDesc = service->method(m);
            appendToFile(out, method, service, methodDesc, includeGuard);
        }
        appendToFile(out, serviceAfterMethods, service, NULL, includeGuard);
    }
    appendToFile(out, afterServices, NULL, NULL, includeGuard);
}

void
ServiceGeneratorHelper::appendToFile(ZeroCopyOutputStream* out,
        const string& contents, const ServiceDescriptor* service,
        const MethodDescriptor* method, const string& includeGuard)
{
    string remain = contents;
    while (!remain.empty()) {
        size_t nextIdent = remain.find(IDENT_PREFIX);
        if (nextIdent == string::npos) {
            writeExactlyOrThrow(out, remain.c_str(), remain.size());
            remain = "";
        } else {
            string pre = remain.substr(0, nextIdent);
            string ident = remain.substr(nextIdent, IDENT_LEN);
            remain = remain.substr(nextIdent + IDENT_LEN,
                    remain.size() - nextIdent - IDENT_LEN);
            writeExactlyOrThrow(out, pre.c_str(), pre.size());
            string replacement;
            getString(ident, service, method, includeGuard, replacement);
            writeExactlyOrThrow(out, replacement.c_str(), replacement.size());
        }
    }
}

void
ServiceGeneratorHelper::getString(const string& ident,
        const ServiceDescriptor* service,
        const MethodDescriptor* method, const string& includeGuard,
        string& value)
{
    if (ident.compare(PROTO_NAME_NO_PATH) == 0) {
        value = protoFilenameNoExtNoPath;
    } else if (ident.compare(INCLUDE_GUARD) == 0) {
        value = includeGuard;
    } else if (ident.compare(NAMESPACES_START) == 0) {
        value = openCppNamespaces(file);
    } else if (ident.compare(NAMESPACES_END) == 0) {
        value = closeCppNamespaces(file);
    } else if (ident.compare(JAVA_PACKAGE) == 0) {
        value = google::protobuf::compiler::java::FileJavaPackage(file);
    } else if (ident.compare(JAVA_OUTER_CLASS) == 0) {
        value = google::protobuf::compiler::java::FileClassName(file);
    } else if (ident.compare(SERVICE_NAME) == 0) {
        value = service->name();
    } else if (ident.compare(SERVICE_NAME_FULL) == 0) {
        value = scopeToCpp(service->full_name());
    } else if (ident.compare(METHOD_NAME) == 0) {
        value = method->name();
    } else if (ident.compare(METHOD_INPUT_TYPE_CPP) == 0) {
        value = scopeToCpp(method->input_type()->full_name());
    } else if (ident.compare(METHOD_OUTPUT_TYPE_CPP) == 0) {
        value = scopeToCpp(method->output_type()->full_name());
    } else if (ident.compare(METHOD_INPUT_TYPE_JAVA) == 0) {
        value = google::protobuf::compiler::java::ClassName(method->input_type());
    } else if (ident.compare(METHOD_OUTPUT_TYPE_JAVA) == 0) {
        value = google::protobuf::compiler::java::ClassName(method->output_type());
    } else if (ident.compare(METHOD_INPUT_TYPE_CSHARP) == 0) {
        value = method->input_type()->full_name();
    } else if (ident.compare(METHOD_OUTPUT_TYPE_CSHARP) == 0) {
        value = method->output_type()->full_name();
    } else {
        string error = "Unsupported ident: ";
        error.append(ident);
        throw error;
    }
}
