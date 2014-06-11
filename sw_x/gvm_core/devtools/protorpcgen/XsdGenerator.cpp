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

#include "XsdGenerator.h"

#include "GeneratorUtils.h"

static string writeDependency(const FileDescriptor* desc, void* param);
static string writeEnum(const EnumDescriptor* desc, void* param);
static string writeEnumVal(const EnumValueDescriptor* desc, void* param);
static string writeMessage(const Descriptor* desc, void* param);
static string writeField(const FieldDescriptor* desc, void* param);
static string getFieldType(const FieldDescriptor* desc);
static string getFieldModifiers(const FieldDescriptor* desc);

XsdGenerator::XsdGenerator(const char* _targetNamespace)
    : targetNamespace(_targetNamespace)
{}

XsdGenerator::~XsdGenerator(void)
{}

bool
XsdGenerator::Generate(const FileDescriptor* file,
        const string& parameter,
        OutputDirectory* outputDirectory,
        string* error) const
{
    bool success = false;
    if (targetNamespace == NULL) {
        *error = "Error: no xsd namespace specified (use --xsd_ns)";
    } else {
        XsdGeneratorHelper helper(file, targetNamespace,
                outputDirectory, error);
        success = helper.generate();
    }
    return success;
}

XsdGeneratorHelper::XsdGeneratorHelper(const FileDescriptor* _proto,
        const char* _targetNamespace,
        OutputDirectory* _outputDirectory,
        string* _error)
    : proto(_proto), targetNamespace(_targetNamespace),
      outputDirectory(_outputDirectory), error(_error), protoFilenameNoExt(),
      protoFilenameNoExtNoPath(), xsd(NULL)
{}

XsdGeneratorHelper::~XsdGeneratorHelper(void)
{
    delete xsd;
}

bool
XsdGeneratorHelper::generate(void)
{
    bool success = true;
    if (proto->enum_type_count() + proto->message_type_count() > 0) {
        if ((!createOutputFile()) || (!writeFileContents())) {
            success = false;
        }
    }
    return success;
}

bool
XsdGeneratorHelper::createOutputFile(void)
{
    bool success = true;
    try {
        protoFilenameNoExt = stripExtension(proto->name());
        protoFilenameNoExtNoPath = stripPath(protoFilenameNoExt);
        string xsdName = protoFilenameNoExt;
        xsdName.append(".xsd");
        xsd = outputDirectory->Open(xsdName);
    } catch (string& e) {
        *error = e;
        success = false;
    }
    return success;
}

bool
XsdGeneratorHelper::writeFileContents(void)
{
    bool success = true;
    try {
        string contents;
        contents.append(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
                "<schema xmlns=\"http://www.w3.org/2001/XMLSchema\"\n" \
                "\t\txmlns:tns=\"").append(targetNamespace).append("\"\n" \
                "\t\ttargetNamespace=\"").append(targetNamespace).append("\"\n" \
                "\t\telementFormDefault=\"qualified\"\n" \
                "\t\tattributeFormDefault=\"unqualified\">\n")
                .append(foreachDependency(writeDependency, proto, NULL)).append(
                "\n" \
                "\t<!-- GENERATED FROM ").append(protoFilenameNoExtNoPath).append(".proto DO NOT EDIT -->\n")
                .append(foreachEnum(writeEnum, proto, NULL))
                .append(foreachMessage(writeMessage, proto, NULL)).append(
                "\n" \
                "</schema>\n");
        success = writeExactly(xsd, contents.c_str(), contents.size());
    } catch (string& s) {
        success = false;
        *error = s;
    }
    return success;
}

string
writeDependency(const FileDescriptor* desc, void* param)
{
    string out;
    out.append("\t<include schemaLocation=\"").append(stripExtension(desc->name())).append(".xsd\"/>\n");
    return out;
}

string
writeEnum(const EnumDescriptor* desc, void* param)
{
    string out;
    out.append(
            "\n" \
            "\t<simpleType name=\"").append(desc->name()).append("\">\n" \
            "\t\t<restriction base=\"string\">\n")
            .append(foreachEnumVal(writeEnumVal, desc, NULL)).append(
            "\t\t</restriction>\n" \
            "\t</simpleType>\n");
    return out;
}

string
writeEnumVal(const EnumValueDescriptor* desc, void* param)
{
    string out;
    out.append("\t\t\t<enumeration value=\"").append(desc->name()).append("\"/>\n");
    return out;
}

string
writeMessage(const Descriptor* desc, void* param)
{
    // If the message has a "_inherited" field, extract the type name from it.
    string xsdExtension;
    for (int i = 0; i < desc->field_count(); i++) {
        if (desc->field(i)->name() == INHERITED_KEY) {
            if (desc->field(i)->type() == FieldDescriptor::TYPE_MESSAGE) {
                xsdExtension = desc->field(i)->message_type()->name();
            }
        }
    }
    
    string out;
    out.append(
            "\n"
            "\t<complexType name=\"").append(desc->name()).append("\">\n");
    if (xsdExtension.size() > 0) {
        out.append("\t  <xsd:complexContent><xsd:extension base=\"").append(xsdExtension).append("\">\n");
    }
    out.append(
            "\t\t<sequence>\n")
            .append(foreachField(writeField, desc, NULL)).append(
            // If there is only one field in the message, add a second so that Axis wsdl2java
            // will not collapse the message type to the field type.  If this happens with a
            // service response type it will affect the encoding and vplex will be unable to
            // parse it.  If this happens with optional or repeated types, we lose the ability
            // to distinguish null from empty.
            (desc->field_count() == 1 ?
                    "\t\t\t<element name=\"unused-field\" type=\"boolean\" minOccurs=\"0\"/>\n" :
                    "")).append(
            "\t\t</sequence>\n");
    if (xsdExtension.size() > 0) {
        out.append("\t  </xsd:extension></xsd:complexContent>\n");
    }
    out.append("\t</complexType>\n");
    return out;
}

string
writeField(const FieldDescriptor* desc, void* param)
{
    string out;
    if (desc->name() != INHERITED_KEY) {
        out.append("\t\t\t<element name=\"").append(desc->name()).append("\" type=\"")
                .append(getFieldType(desc)).append("\"")
                .append(getFieldModifiers(desc)).append("/>\n");
    }
    return out;
}

string
getFieldType(const FieldDescriptor* desc)
{
    string value;
    switch (desc->type()) {
    case FieldDescriptor::TYPE_BOOL:
        value = "boolean";
        break;
    case FieldDescriptor::TYPE_BYTES:
        value = "string";
        break;
    case FieldDescriptor::TYPE_DOUBLE:
        value = "double";
        break;
    case FieldDescriptor::TYPE_ENUM:
        value = "tns:";
        value.append(desc->enum_type()->name());
        break;
    case FieldDescriptor::TYPE_FIXED32:
    case FieldDescriptor::TYPE_UINT32:
        // Unsigned values masquerade as signed while in XML/XSD form,
        // to avoid confusing Axis.
        value = "int";
        break;
    case FieldDescriptor::TYPE_FIXED64:
    case FieldDescriptor::TYPE_UINT64:
        // Unsigned values masquerade as signed while in XML/XSD form,
        // to avoid confusing Axis.
        value = "long";
        break;
    case FieldDescriptor::TYPE_FLOAT:
        value = "float";
        break;
    case FieldDescriptor::TYPE_GROUP:
        throw string("Groups not supported");
        break;
    case FieldDescriptor::TYPE_INT32:
    case FieldDescriptor::TYPE_SFIXED32:
    case FieldDescriptor::TYPE_SINT32:
        value = "int";
        break;
    case FieldDescriptor::TYPE_INT64:
    case FieldDescriptor::TYPE_SFIXED64:
    case FieldDescriptor::TYPE_SINT64:
        value = "long";
        break;
    case FieldDescriptor::TYPE_MESSAGE:
        value = "tns:";
        value.append(desc->message_type()->name());
        break;
    case FieldDescriptor::TYPE_STRING:
        value = "string";
        break;
    }
    return value;
}

string
getFieldModifiers(const FieldDescriptor* field)
{
    string value;
    if (field->is_optional()) {
        value = " minOccurs=\"0\"";
    } else if (field->is_repeated()) {
        value = " minOccurs=\"0\" maxOccurs=\"unbounded\"";
    }
    return value;
}
