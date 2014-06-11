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

#ifndef __SERVICECLIENTGENERATOR_H__
#define __SERVICECLIENTGENERATOR_H__

#include <string>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>

using google::protobuf::compiler::CodeGenerator;
using google::protobuf::compiler::OutputDirectory;
using google::protobuf::FileDescriptor;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using std::string;

typedef unsigned char ServiceGeneratorTarget;
static const ServiceGeneratorTarget SERVICE_GENERATOR_TARGET_CLIENT = 0x01;
static const ServiceGeneratorTarget SERVICE_GENERATOR_TARGET_SERVER = 0x02;
static const ServiceGeneratorTarget SERVICE_GENERATOR_TARGET_ASYNC_CLIENT = 0x04;
static const ServiceGeneratorTarget SERVICE_GENERATOR_TARGET_CLIENT_JAVA =  0x08;
static const ServiceGeneratorTarget SERVICE_GENERATOR_TARGET_CLIENT_CSHARP =  0x10;

class ServiceGenerator : public CodeGenerator
{
public:
    ServiceGenerator(ServiceGeneratorTarget targets);
    virtual ~ServiceGenerator(void);
    virtual bool Generate(const FileDescriptor* file,
            const string& parameter,
            OutputDirectory* outputDirectory,
            string* error) const;
private:
    ServiceGeneratorTarget targets;
};

class ServiceGeneratorHelper
{
public:
    ServiceGeneratorHelper(const FileDescriptor* file,
            const string& parameter,
            ServiceGeneratorTarget targets,
            OutputDirectory* outputDirectory,
            string& error);
    ~ServiceGeneratorHelper(void);
    bool generate(void);

private:
    void createOutputFiles(void);
    void writeClientHeader(void);
    void writeClientSource(void);
    void writeClientJava(void);
    void writeClientCSharp(void);
    void writeAsyncClientHeader(void);
    void writeAsyncClientSource(void);
    void writeServerHeader(void);
    void writeServerSource(void);
    void writeFileContents(ZeroCopyOutputStream* out,
            const string& beforeServices,
            const string& serviceBeforeMethods,
            const string& method,
            const string& serviceAfterMethods,
            const string& afterServices,
            const string& includeGuard);
    void appendToFile(ZeroCopyOutputStream* out, const string& contents,
            const ServiceDescriptor* service, const MethodDescriptor* method,
            const string& includeGuard);
    void getString(const string& ident, const ServiceDescriptor* service,
            const MethodDescriptor* method, const string& includeGuard,
            string& value);

    const FileDescriptor* file;
    const string& parameter;
    OutputDirectory* outputDirectory;
    string& error;
    string protoFilenameNoExt;
    string protoFilenameNoExtNoPath;
    ServiceGeneratorTarget targets;
    ZeroCopyOutputStream* cliHFile;
    ZeroCopyOutputStream* cliCcFile;
    ZeroCopyOutputStream* cliJavaFile;
    ZeroCopyOutputStream* cliCSharpFile;
    ZeroCopyOutputStream* asyncCliHFile;
    ZeroCopyOutputStream* asyncCliCcFile;
    ZeroCopyOutputStream* srvHFile;
    ZeroCopyOutputStream* srvCcFile;
};

#endif // include guard
