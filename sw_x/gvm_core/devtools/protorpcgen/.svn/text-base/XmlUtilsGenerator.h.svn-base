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

#ifndef __XML_UTILS_GENERATOR_H__
#define __XML_UTILS_GENERATOR_H__

#include <string>

#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>

using google::protobuf::compiler::CodeGenerator;
using google::protobuf::compiler::OutputDirectory;
using google::protobuf::Descriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;
using std::string;

class XmlUtilsGenerator : public CodeGenerator
{
public:
    XmlUtilsGenerator(void);
    virtual ~XmlUtilsGenerator(void);
    virtual bool Generate(const FileDescriptor* file,
            const string& parameter,
            OutputDirectory* outputDirectory,
            string* error) const;
};

class XmlUtilsGeneratorHelper
{
public:
    XmlUtilsGeneratorHelper(const FileDescriptor* proto,
            const string& parameter,
            OutputDirectory* outputDirectory,
            string* error);
    ~XmlUtilsGeneratorHelper(void);
    bool generate(void);

private:
    void createOutputFiles(void);
    void writeHeaderContents(void);
    void writeSourceContents(void);

    const FileDescriptor* proto;
    const string& parameter;
    OutputDirectory* outputDirectory;
    string* error;
    string protoFilenameNoExt;
    string protoFilenameNoExtNoPath;
    ZeroCopyOutputStream* hFile;
    ZeroCopyOutputStream* ccFile;
    string cppNs;
};

#endif /* __XML_UTILS_GENERATOR_H__ */
