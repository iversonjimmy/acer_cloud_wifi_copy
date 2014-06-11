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

#include "GeneratorUtils.h"

#include <google/protobuf/compiler/java/java_helpers.h>

#include <cctype>

string
foreachDependency(DependencyProcessor fn, const FileDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->dependency_count(); i++) {
        out.append(fn(desc->dependency(i), param));
    }
    return out;
}

string
foreachEnum(EnumProcessor fn, const FileDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->enum_type_count(); i++) {
        out.append(fn(desc->enum_type(i), param));
    }
    return out;
}

string
foreachEnumVal(EnumValProcessor fn, const EnumDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->value_count(); i++) {
        out.append(fn(desc->value(i), param));
    }
    return out;
}

string
foreachMessage(MessageProcessor fn, const FileDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->message_type_count(); i++) {
        out.append(fn(desc->message_type(i), param));
    }
    return out;
}

string
foreachField(FieldProcessor fn, const Descriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->field_count(); i++) {
        out.append(fn(desc->field(i), param));
    }
    return out;
}

string
foreachService(ServiceProcessor fn, const FileDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->service_count(); i++) {
        out.append(fn(desc->service(i), param));
    }
    return out;
}

string
foreachMethod(MethodProcessor fn, const ServiceDescriptor* desc, void* param)
{
    string out;
    for (int i = 0; i < desc->method_count(); i++) {
        out.append(fn(desc->method(i), param));
    }
    return out;
}

string
stripExtension(const string& fileName)
{
    string out;
    const string& fileEnd = fileName.substr(fileName.size() - 6, 6);
    if (fileEnd.compare(".proto") != 0) {
        string error = "File does not end in .proto: ";
        error.append(fileName);
        throw error;
    } else {
        out = fileName.substr(0, fileName.size() - 6);
    }
    return out;
}

string
stripPath(const string& fileName)
{
    string out;
    size_t slashPos = fileName.find_last_of('/');
    if (slashPos == string::npos) {
        out = fileName;
    } else {
        out = fileName.substr(slashPos + 1, fileName.size() - slashPos - 1);
    }
    return out;
}

string
stripPandE(const string& fileName)
{
    return stripPath(stripExtension(fileName));
}

string
scopeToCpp(const string& name)
{
    string out;
    for (int i = 0; i < name.size(); i++) {
        if (name[i] == '.') {
            out.append("::");
        } else {
            out.push_back(name[i]);
        }
    }
    return out;
}

string
toLower(const string& name)
{
    string out;
    for (int i = 0; i < name.size(); i++) {
        out.push_back(tolower(name[i]));
    }
    return out;
}

string
openCppNamespaces(const FileDescriptor* desc)
{
    string out;
    const string& pkg = desc->package();
    size_t i = 0;
    while (i != string::npos) {
        size_t j = pkg.find('.', i);
        string ns = pkg.substr(i, (j - i));
        if (!ns.empty()) {
            out.append("namespace ").append(ns).append(" {\n");
        }
        if (j != string::npos) {
            j++;
        }
        i = j;
    }
    return out;
}

string
closeCppNamespaces(const FileDescriptor* desc)
{
    string out;
    const string& pkg = desc->package();
    size_t i = 0;
    while (i != string::npos) {
        size_t j = pkg.find('.', i);
        string ns = pkg.substr(i, (j - i));
        if (!ns.empty()) {
            out.append("} // namespace ").append(ns).append("\n");
        }
        if (j != string::npos) {
            j++;
        }
        i = j;
    }
    return out;
}

string
getJavaPackagePath(const FileDescriptor* fileDesc)
{
    string out;
    const string pkg = google::protobuf::compiler::java::FileJavaPackage(fileDesc);
    size_t i = 0;
    for (;;) {
        size_t j = pkg.find('.', i);
        string ns = pkg.substr(i, (j - i));
        if (!ns.empty()) {
            out.append(ns).append("/");
        }
        if (j == string::npos) {
            break;
        }
        j++;
        i = j;
    }
    return out;
}

void
writeExactlyOrThrow(ZeroCopyOutputStream* stream, const void* bytes, int numBytes)
{
    if (!writeExactly(stream, bytes, numBytes)) {
        string error = "Failed to write to stream";
        throw error;
    }
}

bool
writeExactly(ZeroCopyOutputStream* stream, const void* bytes, int numBytes)
{
    bool success = true;

    while (numBytes > 0) {
        void* buffer = NULL;
        int bufferLen = 0;
        success &= stream->Next(&buffer, &bufferLen);
        if (!success) {
            break;
        }
        int numToWrite = (bufferLen < numBytes ? bufferLen : numBytes);
        memcpy(buffer, bytes, numToWrite);
        bytes = (const void*)(((const int8_t*)bytes) + numToWrite);
        numBytes -= numToWrite;
        if (numToWrite < bufferLen) {
            // Got too much buffer, back up
            stream->BackUp(bufferLen - numToWrite);
        }
    }

    return success;
}

bool
readExactly(ZeroCopyInputStream* stream, void* bytes, int numBytes)
{
    bool success = true;

    while (numBytes > 0) {
        const void* buffer = NULL;
        int bufferLen = 0;
        success &= stream->Next(&buffer, &bufferLen);
        if (!success) {
            break;
        }
        int numToRead = (bufferLen < numBytes ? bufferLen : numBytes);
        memcpy(bytes, buffer, numToRead);
        bytes = (void*)(((int8_t*)bytes) + numToRead);
        numBytes -= numToRead;
        if (numToRead < bufferLen) {
            // Got too many bytes, back up
            stream->BackUp(bufferLen - numToRead);
        }
    }

    return success;
}
