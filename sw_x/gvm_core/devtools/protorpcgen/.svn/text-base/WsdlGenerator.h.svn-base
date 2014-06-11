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

#ifndef __WSDL_GENERATOR_H__
#define __WSDL_GENERATOR_H__

#include <string>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>

using google::protobuf::compiler::CodeGenerator;
using google::protobuf::compiler::OutputDirectory;
using google::protobuf::FileDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using std::string;

class WsdlGenerator : public CodeGenerator
{
public:
    WsdlGenerator(const char* targetNamespace, const char* serverName,
            const char* port);
    virtual ~WsdlGenerator(void);
    virtual bool Generate(const FileDescriptor* file,
            const string& parameter,
            OutputDirectory* outputDirectory,
            string* error) const;

private:
    const char* targetNamespace;
    const char* serverName;
    const char* port;
};

class WsdlGeneratorHelper
{
public:
    WsdlGeneratorHelper(const FileDescriptor* proto,
            const char* targetNamespace,
            const char* serverName,
            const char* port,
            OutputDirectory* outputDirectory,
            string* error);
    ~WsdlGeneratorHelper(void);
    bool generate(void);

private:
    bool createOutputFile(void);
    bool writeFileContents(void);

    const FileDescriptor* proto;
    const char* targetNamespace;
    const char* serverName;
    const char* port;
    OutputDirectory* outputDirectory;
    string* error;
    string protoFilenameNoExt;
    string protoFilenameNoExtNoPath;
    ZeroCopyOutputStream* wsdl;
};

#endif /* __WSDL_GENERATOR_H__ */
