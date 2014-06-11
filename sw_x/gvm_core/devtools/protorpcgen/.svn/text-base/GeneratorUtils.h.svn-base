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

#ifndef __GENERATOR_UTILS_H__
#define __GENERATOR_UTILS_H__

#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/message.h>

using google::protobuf::Descriptor;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::MethodDescriptor;
using google::protobuf::ServiceDescriptor;
using google::protobuf::io::ZeroCopyInputStream;
using google::protobuf::io::ZeroCopyOutputStream;

using std::string;

class GeneratorHelper;

/// Special-case field name that is used to indicate that protobuf composition should be mapped
/// to inheritance.  This is currently only used when generating XSDs from .protos, and when
/// generating C++ code to convert between XML strings and C++ protobuf objects.
static const string INHERITED_KEY = "_inherited";

// All of these string methods may throw string to indicate error.

typedef string (*DependencyProcessor)(const FileDescriptor* desc, void* param);
typedef string (*EnumProcessor)(const EnumDescriptor* desc, void* param);
typedef string (*EnumValProcessor)(const EnumValueDescriptor* desc, void* param);
typedef string (*MessageProcessor)(const Descriptor* desc, void* param);
typedef string (*FieldProcessor)(const FieldDescriptor* desc, void* param);
typedef string (*ServiceProcessor)(const ServiceDescriptor* desc, void* param);
typedef string (*MethodProcessor)(const MethodDescriptor* desc, void* param);

string foreachDependency(DependencyProcessor fn, const FileDescriptor* desc, void* param);
string foreachEnum(EnumProcessor fn, const FileDescriptor* desc, void* param);
string foreachEnumVal(EnumValProcessor fn, const EnumDescriptor* desc, void* param);
string foreachMessage(MessageProcessor fn, const FileDescriptor* desc, void* param);
string foreachField(FieldProcessor fn, const Descriptor* desc, void* param);
string foreachService(ServiceProcessor fn, const FileDescriptor* desc, void* param);
string foreachMethod(MethodProcessor fn, const ServiceDescriptor* desc, void* param);

string stripExtension(const string& fileName);
string stripPath(const string& fileName);
string stripPandE(const string& fileName);
string scopeToCpp(const string& name);
string toLower(const string& name);

string openCppNamespaces(const FileDescriptor* fileDesc);
string closeCppNamespaces(const FileDescriptor* fileDesc);

string getJavaPackagePath(const FileDescriptor* fileDesc);

void writeExactlyOrThrow(ZeroCopyOutputStream* stream, const void* bytes, int numBytes);

// TODO the below is copied from code in protorpc lib, should use that

/// Helper to write a buffer to a zero copy stream and handle excess buffer
/// space provided by the stream.
bool writeExactly(ZeroCopyOutputStream* stream, const void* bytes, int numBytes);

/// Helper to read an exact number of bytes from a zero copy stream and
/// handle any excess read-ahead performed by the stream.
bool readExactly(ZeroCopyInputStream* stream, void* bytes, int numBytes);

#endif /* __GENERATOR_UTILS_H__ */
