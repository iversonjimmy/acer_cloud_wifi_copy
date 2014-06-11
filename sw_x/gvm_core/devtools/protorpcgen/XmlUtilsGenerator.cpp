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

#include "XmlUtilsGenerator.h"

#include "GeneratorUtils.h"

#include <google/protobuf/compiler/cpp/cpp_helpers.h>

#include <iostream>
#include <sstream>

using std::ostringstream;
using std::endl;

static string writeMessageParseClassHdr(const Descriptor* desc, void* cppNsParam);
static string writeMessageWriterHdr(const Descriptor* desc, void* cppNsParam);
static string writeSourceDepInclude(const FileDescriptor* desc, void* cppNsParam);
static string writeEnumFnHdr(const EnumDescriptor* desc, void* cppNsParam);
static string writeMessageFnProtos(const Descriptor* desc, void* cppNsParam);
static string writeMessageParseClassCtr(const Descriptor* desc, void* cppNsParam);
static string writeRegisterFn(const FileDescriptor* desc, void* cppNsParam);
static string writeRegisterType(const Descriptor* desc, void* cppNsParam);
static string writeRegisterDep(const FileDescriptor* desc, void* cppNsParam);
static string writeEnumFn(const EnumDescriptor* desc, void* cppNsParam);
static string writeEnumFnCase(const EnumValueDescriptor* desc, void* cppNsParam);
static string writeMessageFns(const Descriptor* desc, void* cppNsParam);
static string writeMsgOpenFnCase(const FieldDescriptor* desc, void* cppNsParam);
static string writeMsgCloseFnCase(const FieldDescriptor* desc, void* cppNsParam);
static string writeEnumWriterHdr(const EnumDescriptor* desc, void* cppNsParam);
static string writeEnumWriterFn(const EnumDescriptor* desc, void* cppNsParam);
static string writeEnumWriterVal(const EnumValueDescriptor* desc, void* cppNsParam);
static string writeMsgWriterFn(const Descriptor* desc, void* cppNsParam);
static string writeFieldWriter(const FieldDescriptor* desc, void* cppNsParam);
static string writeNamespace(const FileDescriptor* desc);
static string writeCloseNamespace(const FileDescriptor* desc);

XmlUtilsGenerator::XmlUtilsGenerator(void)
{}

XmlUtilsGenerator::~XmlUtilsGenerator(void)
{}

bool
XmlUtilsGenerator::Generate(const FileDescriptor* file,
        const string& parameter,
        OutputDirectory* outputDirectory,
        string* error) const
{
    XmlUtilsGeneratorHelper helper(file, parameter, outputDirectory, error);
    return helper.generate();
}

XmlUtilsGeneratorHelper::XmlUtilsGeneratorHelper(const FileDescriptor* _proto,
        const string& _parameter,
        OutputDirectory* _outputDirectory,
        string* _error)
    : proto(_proto), parameter(_parameter), outputDirectory(_outputDirectory),
      error(_error), protoFilenameNoExt(), protoFilenameNoExtNoPath(),
      hFile(NULL), ccFile(NULL)
{}

XmlUtilsGeneratorHelper::~XmlUtilsGeneratorHelper(void)
{
    delete hFile;
    delete ccFile;
}

bool
XmlUtilsGeneratorHelper::generate(void)
{
    bool success = true;
    try {
        if (proto->enum_type_count() + proto->message_type_count() > 0) {
            createOutputFiles();
            writeHeaderContents();
            writeSourceContents();
        } else {
            success = false;
            *error = "No enum or message types in input";
        }
    } catch (string e) {
        *error = e;
        success = false;
    }
    return success;
}

void
XmlUtilsGeneratorHelper::createOutputFiles(void)
{
    protoFilenameNoExt = stripExtension(proto->name());
    protoFilenameNoExtNoPath = stripPath(protoFilenameNoExt);
    cppNs = scopeToCpp(proto->package());
    string hName = protoFilenameNoExt;
    hName.append("-xml.pb.h");
    hFile = outputDirectory->Open(hName);
    string ccName = protoFilenameNoExt;
    ccName.append("-xml.pb.cc");
    ccFile = outputDirectory->Open(ccName);
}

void
XmlUtilsGeneratorHelper::writeHeaderContents(void)
{
    string incGuard;
    incGuard.append("__").append(protoFilenameNoExtNoPath)
            .append("_xml_pb_h__");
    string contents;
    contents.append(
            "// GENERATED FROM ").append(protoFilenameNoExtNoPath).append(
                ".proto, DO NOT EDIT\n" \
            "#ifndef ").append(incGuard).append("\n" \
            "#define ").append(incGuard).append("\n" \
            "\n" \
            "#include <vplex_xml_writer.h>\n" \
            "#include <ProtoXmlParseState.h>\n" \
            "#include \"").append(protoFilenameNoExtNoPath).append(".pb.h\"\n" \
            "\n").append(
            writeNamespace(proto)).append(
            "\n").append(
            foreachMessage(writeMessageParseClassHdr, proto, NULL)).append(
            "void RegisterHandlers_").append(protoFilenameNoExtNoPath).append(
                "(ProtoXmlParseStateImpl*);\n" \
            "\n").append(
            foreachEnum(writeEnumFnHdr, proto, NULL)).append(
            foreachEnum(writeEnumWriterHdr, proto, NULL)).append(
            "\n").append(
            foreachMessage(writeMessageWriterHdr, proto, NULL)).append(
            "\n").append(
            writeCloseNamespace(proto)).append(
            "#endif /* ").append(incGuard).append(" */\n");
    if (!writeExactly(hFile, contents.c_str(), contents.size())) {
        string error("writeExactly failed");
        throw error;
    }
}

string
writeMessageParseClassHdr(const Descriptor* desc, void*)
{
    string out;
    out.append(
            "class ParseState").append(desc->name()).append(
                " : public ProtoXmlParseState\n" \
            "{\n" \
            "public:\n" \
            "    ParseState").append(desc->name()).append(
                "(const char* outerTag, ").append(desc->name()).append(
                "* message);\n" \
            "};\n" \
            "\n");
    return out;
}

string
writeMessageWriterHdr(const Descriptor* desc, void*)
{
    string out;
    out.append("void write").append(desc->name())
            .append("(VPLXmlWriter* writer, const ").append(desc->name())
            .append("& message);\n");
    return out;
}

void
XmlUtilsGeneratorHelper::writeSourceContents(void)
{
    string contents;
    contents.append(
            "// GENERATED FROM ").append(protoFilenameNoExtNoPath).append(
                ".proto, DO NOT EDIT\n" \
            "\n" \
            "#include \"").append(protoFilenameNoExtNoPath).append(
                "-xml.pb.h\"\n" \
            "\n" \
            "#include <string.h>\n" \
            "\n")
            .append(foreachDependency(writeSourceDepInclude, proto, NULL)).append(
            "#include <ProtoXmlParseStateImpl.h>\n" \
            "\n")
            .append(foreachMessage(writeMessageFnProtos, proto, NULL)).append(
            "\n")
            .append(foreachMessage(writeMessageParseClassCtr, proto, &cppNs))
            .append(writeRegisterFn(proto, &cppNs))
            .append(foreachEnum(writeEnumFn, proto, &cppNs))
            .append(foreachMessage(writeMessageFns, proto, &cppNs))
            .append(foreachEnum(writeEnumWriterFn, proto, &cppNs))
            .append(foreachMessage(writeMsgWriterFn, proto, &cppNs));
    if (!writeExactly(ccFile, contents.c_str(), contents.size())) {
        string error("writeExactly failed");
        throw error;
    }
}

string
writeSourceDepInclude(const FileDescriptor* desc, void*)
{
    string out;
    out.append("#include \"").append(stripPandE(desc->name())).append(
                "-xml.pb.h\"\n");
    return out;
}

string
writeEnumFnHdr(const EnumDescriptor* desc, void*)
{
    string out;
    out.append(scopeToCpp(desc->full_name())).append(" parse")
            .append(desc->name())
            .append("(const std::string& data, ProtoXmlParseStateImpl* state);\n");
    return out;
}

string
writeMessageFnProtos(const Descriptor* desc, void*)
{
    ostringstream out;
    out << "static void processOpen"  << desc->name() << "(" << scopeToCpp(desc->full_name()) <<
               "* msg, const char* tag, ProtoXmlParseStateImpl* state);\n"
           "static void processOpen"  << desc->name() << "Cb(const char* tag, ProtoXmlParseStateImpl* state);\n"
           "static void processClose" << desc->name() << "(" << scopeToCpp(desc->full_name()) <<
               "* msg, const char* tag, ProtoXmlParseStateImpl* state);\n"
           "static void processClose" << desc->name() << "Cb(const char* tag, ProtoXmlParseStateImpl* state);\n";
    return out.str();
}

string
writeMessageParseClassCtr(const Descriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    string className;
    className.append("ParseState").append(desc->name());
    string out;
    out.append(
            *cppNs).append("::").append(className).append("::")
                .append(className).append("(const char* outerTag, ")
                .append(desc->name()).append("* message)\n" \
            "    : ProtoXmlParseState(outerTag, message)\n" \
            "{\n" \
            "    RegisterHandlers_").append(stripPandE(desc->file()->name()))
                .append("(impl);\n").append(
            "}\n" \
            "\n");
    return out;
}

string
writeRegisterFn(const FileDescriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    string out;
    out.append(
            "void\n")
            .append(*cppNs).append("::RegisterHandlers_")
                .append(stripPandE(desc->name())).append(
                "(ProtoXmlParseStateImpl* impl)\n" \
            "{\n");
    if (desc->message_type_count() > 0) {
        out.append(
                "    if (impl->openHandlers.find(\"")
                    .append(desc->message_type(0)->full_name()).append(
                    "\") == impl->openHandlers.end()) {\n")
                .append(foreachMessage(writeRegisterType, desc, NULL))
                .append(foreachDependency(writeRegisterDep, desc, NULL)).append(
                "    }\n");
    }
    out.append(
            "}\n" \
            "\n");
    return out;
}

string
writeRegisterType(const Descriptor* desc, void*)
{
    string out;
    out.append(
            "        impl->openHandlers[\"").append(desc->full_name()).append(
                "\"] = processOpen").append(desc->name()).append("Cb;\n" \
            "        impl->closeHandlers[\"").append(desc->full_name()).append(
                "\"] = processClose").append(desc->name()).append("Cb;\n");
    return out;
}

string
writeRegisterDep(const FileDescriptor* desc, void*)
{
    string out;
    out.append("        ").append(scopeToCpp(desc->package()))
            .append("::RegisterHandlers_")
            .append(stripPandE(desc->name())).append("(impl);\n");
    return out;
}

string
writeEnumFn(const EnumDescriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    string out;
    out.append(scopeToCpp(desc->full_name())).append("\n")
            .append(*cppNs).append("::parse").append(desc->name()).append(
                "(const std::string& data, ProtoXmlParseStateImpl* state)\n" \
            "{\n" \
            "    ").append(scopeToCpp(desc->full_name())).append(" out;\n" \
            "    ").append(
                foreachEnumVal(writeEnumFnCase, desc, NULL)).append("{\n" \
            "        state->error = \"Invalid ").append(desc->full_name())
                .append(": \";\n" \
            "        state->error.append(data);\n" \
            "        out = ").append(scopeToCpp(desc->value(0)->full_name()))
                .append(";\n" \
            "    }\n" \
            "    return out;\n" \
            "}\n" \
            "\n");
    return out;
}

string
writeEnumFnCase(const EnumValueDescriptor* desc, void*)
{
    string out;
    out.append("if (data.compare(\"").append(desc->name()).append(
                "\") == 0) {\n" \
            "        out = ").append(scopeToCpp(desc->full_name())).append(";\n" \
            "    } else ");
    return out;
}

string
writeMessageFns(const Descriptor* desc, void* cppNsParam)
{
    ostringstream out;
    const string* inheritedTypeName = NULL;
    for (int i = 0; i < desc->field_count(); i++) {
        if (desc->field(i)->name() == INHERITED_KEY) {
            if (desc->field(i)->type() != FieldDescriptor::TYPE_MESSAGE) {
                string error(INHERITED_KEY);
                error.append(" is only supported for FieldDescriptor::TYPE_MESSAGE");
                throw error;
            }
            inheritedTypeName = &(desc->field(i)->message_type()->name());
        }
    }
    out <<
           "void\n"
           "processOpen" << desc->name() << "(" << scopeToCpp(desc->full_name()) <<
               "* msg, const char* tag, ProtoXmlParseStateImpl* state)\n"
           "{\n"
           "    " << foreachField(writeMsgOpenFnCase, desc, NULL) <<
               "{\n"
           "        (void)msg;\n"
           "    }\n"
           "}\n"
           "void\n"
           "processOpen" << desc->name() << "Cb(const char* tag, ProtoXmlParseStateImpl* state)\n"
           "{\n"
           "    " << scopeToCpp(desc->full_name()) << "* msg = static_cast<" <<
               scopeToCpp(desc->full_name()) << "*>(state->protoStack.top());\n"
           "    processOpen" << desc->name() << "(msg, tag, state);\n"
           "}\n"
           "\n"
           "void\n"
           "processClose" << desc->name() << "(" << scopeToCpp(desc->full_name()) <<
               "* msg, const char* tag, ProtoXmlParseStateImpl* state)\n"
           "{\n"
           "    " <<  foreachField(writeMsgCloseFnCase, desc, cppNsParam) <<
               "{\n"
           "        ";
    if (inheritedTypeName != NULL) {
        out << "processClose" << *inheritedTypeName << "(msg->mutable__inherited(), tag, state);\n";
    } else {
        out << "(void)msg;\n";
    }
    out << "    }\n"
           "}\n"
           "void\n" \
           "processClose" << desc->name() << "Cb(const char* tag, ProtoXmlParseStateImpl* state)\n" \
           "{\n" \
           "    " << scopeToCpp(desc->full_name()) << "* msg = static_cast<" <<
               scopeToCpp(desc->full_name()) << "*>(state->protoStack.top());\n"
           "    processClose" << desc->name() << "(msg, tag, state);\n"
           "}\n"
           "\n";
    return out.str();
}

static string
writeInheritedMsgOpenFnCase(const FieldDescriptor* desc, void* param)
{
    string out;
    if (desc->type() == FieldDescriptor::TYPE_MESSAGE) {
        // TODO: This should actually write openers for any message fields in the inherited
        //   type.  However, we don't need this currently (for IAS).
        string error("Currently unsupported: field type 'message' within an inherited message type");
        throw error;
    }
    return out;
}

string
writeMsgOpenFnCase(const FieldDescriptor* desc, void* cppNsParam)
{
    string out;
    if (desc->type() == FieldDescriptor::TYPE_MESSAGE) {
        if (desc->name() == INHERITED_KEY) {
            foreachField(writeInheritedMsgOpenFnCase, desc->message_type(), cppNsParam);
        } else {
            out.append("if (strcmp(tag, \"").append(desc->name()).append(
                    "\") == 0) {\n");
            string indent;
            if (desc->is_repeated()) {
                indent = "        ";
            } else {
                indent = "            ";
                out.append("        if (msg->has_").append(google::protobuf::compiler::cpp::FieldName(desc))
                            .append("()) {\n" \
                        "            state->error = \"Duplicate of non-repeated field ")
                            .append(desc->name()).append("\";\n" \
                        "        } else {\n");
            }
            out.append(indent).append(scopeToCpp(desc->message_type()->full_name())).append(
                    "* proto = msg->").append(
                    desc->is_repeated() ? "add_" : "mutable_").append(
                    google::protobuf::compiler::cpp::FieldName(desc)).append("();\n");
            out.append(indent).append("state->protoStack.push(proto);\n");
            out.append(indent).append("state->protoStartDepth.push(state->tagDepth);\n");
            if (!desc->is_repeated()) {
                out.append("        }\n");
            }
            out.append("    } else ");
        }
    }
    return out;
}

string
writeMsgCloseFnCase(const FieldDescriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    bool processField = false;
    string protoArg;
    switch (desc->type()) {
    case FieldDescriptor::TYPE_BOOL:
        processField = true;
        protoArg = "parseBool(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_BYTES:
        processField = true;
        protoArg = "parseBytes(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_DOUBLE:
        processField = true;
        protoArg = "parseDouble(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_ENUM:
        processField = true;
        protoArg.append(*cppNs).append("::parse").append(desc->enum_type()->name())
                .append("(state->lastData, state)");
        break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
        processField = true;
        protoArg = "(u32)parseInt32(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
        processField = true;
        protoArg = "(u64)parseInt64(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_FLOAT:
        processField = true;
        protoArg = "parseFloat(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_GROUP:
        throw string("Groups not supported");
        break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
        processField = true;
        protoArg = "parseInt32(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
        processField = true;
        protoArg = "parseInt64(state->lastData, state)";
        break;
    case FieldDescriptor::TYPE_MESSAGE:
        // Nothing extra to do here.
        break;
    case FieldDescriptor::TYPE_STRING:
        processField = true;
        protoArg = "state->lastData";
        break;
    }
    string out;
    if (processField) {
        out.append("if (strcmp(tag, \"").append(desc->name())
                .append("\") == 0) {\n");
        if (desc->is_repeated()) {
            out.append("        msg->add_").append(google::protobuf::compiler::cpp::FieldName(desc))
                    .append("(").append(protoArg).append(");\n");
        } else {
            out.append("        if (msg->has_").append(google::protobuf::compiler::cpp::FieldName(desc))
                        .append("()) {\n" \
                    "            state->error = \"Duplicate of non-repeated field ")
                        .append(desc->name()).append("\";\n" \
                    "        } else {\n" \
                    "            msg->set_").append(google::protobuf::compiler::cpp::FieldName(desc))
                        .append("(").append(protoArg).append(");\n" \
                    "        }\n");
        }
        out.append("    } else ");
    }
    return out;
}

string
writeEnumWriterHdr(const EnumDescriptor* desc, void*)
{
    string out;
    out.append("const char* write").append(desc->name())
            .append("(").append(scopeToCpp(desc->full_name())).append(" val);\n");
    return out;
}

string
writeEnumWriterFn(const EnumDescriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    string out;
    out.append(
            "const char*\n")
            .append(*cppNs).append("::write").append(desc->name()).append("(")
                .append(scopeToCpp(desc->full_name())).append(" val)\n" \
            "{\n" \
            "    const char* out;\n" \
            "    ").append(
                foreachEnumVal(writeEnumWriterVal, desc, NULL)).append("{\n" \
            "        out = \"\";\n" \
            "    }\n"
            "    return out;\n" \
            "}\n" \
            "\n");
    return out;
}

string
writeEnumWriterVal(const EnumValueDescriptor* desc, void*)
{
    string out;
    out.append("if (val == ").append(scopeToCpp(desc->full_name()))
                .append(") {\n" \
            "        out = \"").append(desc->name()).append("\";\n" \
            "    } else ");
    return out;
}

string
writeMsgWriterFn(const Descriptor* desc, void* cppNsParam)
{
    string* cppNs = (string*)cppNsParam;
    string out;
    out.append(
            "void\n")
            .append(*cppNs).append("::write").append(desc->name())
                .append("(VPLXmlWriter* writer, const ")
                .append(scopeToCpp(desc->full_name())).append("& message)\n" \
            "{\n").append(
            foreachField(writeFieldWriter, desc, NULL)).append(
            "}\n" \
            "\n");
    return out;
}

string
writeFieldWriter(const FieldDescriptor* desc, void*)
{
    string out;
    string instance;
    instance.append("message.").append(google::protobuf::compiler::cpp::FieldName(desc))
            .append(desc->is_repeated() ? "(i)" : "()");
    if (desc->is_repeated()) {
        out.append("    for (int i = 0; i < message.")
                .append(google::protobuf::compiler::cpp::FieldName(desc)).append("_size(); i++) {\n");
    } else {
        out.append("    if (message.has_").append(google::protobuf::compiler::cpp::FieldName(desc))
                .append("()) {\n");
    }
    switch (desc->type()) {
    case FieldDescriptor::TYPE_BOOL:
        out.append(
                "        const char* val = (").append(instance)
                    .append(" ? \"true\" : \"false\");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_BYTES:
        out.append(
                "        std::string val = ")
                    .append("writeBytes(").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val.c_str());\n");
        break;
    case FieldDescriptor::TYPE_DOUBLE:
        out.append(
                "        char val[64];\n" \
                "        sprintf(val, \"%.24lf\", ").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_ENUM:
        out.append(
                "        const char* val = ")
                    .append(scopeToCpp(desc->enum_type()->file()->package()))
                    .append("::write").append(desc->enum_type()->name()).append("(")
                    .append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
        // Unsigned values masquerade as signed while in XML/XSD form,
        // to avoid confusing Axis.
        out.append(
                "        char val[15];\n" \
                "        sprintf(val, FMTs32, (s32)").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
        // Unsigned values masquerade as signed while in XML/XSD form,
        // to avoid confusing Axis.
        out.append(
                "        char val[30];\n" \
                "        sprintf(val, FMTs64, (s64)").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_FLOAT:
        out.append(
                "        char val[32];\n" \
                "        sprintf(val, \"%.12f\", ").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
        out.append(
                "        char val[15];\n" \
                "        sprintf(val, FMTs32, ").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
        out.append(
                "        char val[30];\n" \
                "        sprintf(val, FMTs64, ").append(instance).append(");\n" \
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", val);\n");
        break;
    case FieldDescriptor::TYPE_MESSAGE:
        if (desc->name() != INHERITED_KEY) {
            out.append("        VPLXmlWriter_OpenTagV(writer, \"").append(desc->name()).append("\", 0);\n");
        }
        out.append("        ").append(scopeToCpp(desc->message_type()->file()->package()))
                    .append("::write").append(desc->message_type()->name())
                    .append("(writer, ").append(instance).append(");\n");
        if (desc->name() != INHERITED_KEY) {
            out.append("        VPLXmlWriter_CloseTag(writer);\n");
        }
        break;
    case FieldDescriptor::TYPE_STRING:
        out.append(
                "        VPLXmlWriter_InsertSimpleElement(writer, \"")
                    .append(desc->name()).append("\", ").append(instance)
                    .append(".c_str());\n");
        break;
    case FieldDescriptor::TYPE_GROUP:
    default:
        // Shouldn't happen
        throw string("Unsupported type in writeFieldWriter");
    }
    out.append("    }\n");
    return out;
}

string
writeNamespace(const FileDescriptor* desc)
{
    string out;
    const string& pkg = desc->package();
    size_t i = 0;
    while (i != string::npos) {
        size_t j = pkg.find('.', i);
        string ns = pkg.substr(i, j);
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
writeCloseNamespace(const FileDescriptor* desc)
{
    string out;
    const string& pkg = desc->package();
    size_t i = 0;
    while (i != string::npos) {
        size_t j = pkg.find('.', i);
        string ns = pkg.substr(i, j);
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
