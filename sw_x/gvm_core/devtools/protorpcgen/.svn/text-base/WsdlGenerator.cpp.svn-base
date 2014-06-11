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

#include "WsdlGenerator.h"

#include "GeneratorUtils.h"

static string writeDependency(const FileDescriptor* desc, void* param);
static string writeServiceElements(const ServiceDescriptor* desc, void* param);
static string writeElements(const MethodDescriptor* desc, void* param);
static string writeServiceMessages(const ServiceDescriptor* desc, void* param);
static string writeMessages(const MethodDescriptor* desc, void* param);
static string writeServicePortType(const ServiceDescriptor* desc, void* param);
static string writeOperation(const MethodDescriptor* desc, void* param);
static string writeServiceBinding(const ServiceDescriptor* desc, void* param);
static string writeBinding(const MethodDescriptor* desc, void* param);
static string writeServicePort(const ServiceDescriptor* desc, void* param);

struct ServiceParams {
    const char* serverName;
    const char* port;
};

WsdlGenerator::WsdlGenerator(const char* _targetNamespace,
        const char* _serverName, const char* _port)
    : targetNamespace(_targetNamespace), serverName(_serverName), port(_port)
{}

WsdlGenerator::~WsdlGenerator(void)
{}

bool
WsdlGenerator::Generate(const FileDescriptor* file,
        const string& parameter,
        OutputDirectory* outputDirectory,
        string* error) const
{
    bool success = false;
    if (targetNamespace == NULL) {
        *error = "Error: no xsd namespace specified (use --xsd_ns)";
    } else if (serverName == NULL) {
        *error = "Error: no server name specified (use --wsdl_server)";
    } else if (port == NULL) {
        *error = "Error: no port specified (use --wsdl_port)";
    } else {
        WsdlGeneratorHelper helper(file, targetNamespace, serverName, port,
                outputDirectory, error);
        success = helper.generate();
    }
    return success;
}

WsdlGeneratorHelper::WsdlGeneratorHelper(const FileDescriptor* _proto,
        const char* _targetNamespace,
        const char* _serverName,
        const char* _port,
        OutputDirectory* _outputDirectory,
        string* _error)
    : proto(_proto), targetNamespace(_targetNamespace),
      serverName(_serverName), port(_port), outputDirectory(_outputDirectory),
      error(_error), protoFilenameNoExt(), protoFilenameNoExtNoPath(),
      wsdl(NULL)
{}

WsdlGeneratorHelper::~WsdlGeneratorHelper(void)
{
    delete wsdl;
}

bool
WsdlGeneratorHelper::generate(void)
{
    bool success = createOutputFile();
    return success;
}

bool
WsdlGeneratorHelper::createOutputFile(void)
{
    bool success = true;
    try {
        protoFilenameNoExt = stripExtension(proto->name());
        protoFilenameNoExtNoPath = stripPath(protoFilenameNoExt);
        string wsdlName = protoFilenameNoExtNoPath;
        wsdlName.append(".wsdl");
        wsdl = outputDirectory->Open(wsdlName);
        success = writeFileContents();
    } catch (string s) {
        success = false;
        *error = s;
    }
    return success;
}

bool
WsdlGeneratorHelper::writeFileContents(void)
{
    bool success = true;
    try {
        string contents;
        ServiceParams params;
        params.serverName = serverName;
        params.port = port;
        contents.append(
                "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" \
                "<!-- GENERATED FROM ").append(protoFilenameNoExtNoPath).append(".proto DO NOT EDIT -->\n" \
                "<wsdl:definitions name=\"").append(protoFilenameNoExtNoPath).append("\"\n" \
                "\t\ttargetNamespace=\"").append(targetNamespace).append("\"\n" \
                "\t\txmlns:soap=\"http://schemas.xmlsoap.org/wsdl/soap/\"\n" \
                "\t\txmlns:tns=\"").append(targetNamespace).append("\"\n" \
                "\t\txmlns:wsdl=\"http://schemas.xmlsoap.org/wsdl/\"\n" \
                "\t\txmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">\n" \
                "\t<wsdl:types>\n" \
                "\t\t<schema xmlns=\"http://www.w3.org/2001/XMLSchema\"\n" \
                "\t\t\t\txmlns:tns=\"").append(targetNamespace).append("\"\n" \
                "\t\t\t\ttargetNamespace=\"").append(targetNamespace).append("\"\n" \
                "\t\t\t\telementFormDefault=\"qualified\"\n" \
                "\t\t\t\tattributeFormDefault=\"unqualified\">\n")
                .append(foreachDependency(writeDependency, proto, NULL))
                .append(foreachService(writeServiceElements, proto, NULL)).append(
                "\t\t</schema>\n" \
                "\t</wsdl:types>\n" \
                "\n" \
                "\t<wsdl:message name=\"GetHealthRequest\">\n" \
                "\t</wsdl:message>\n" \
                "\t<wsdl:message name=\"GetHealthResponse\">\n" \
                "\t\t<wsdl:part name=\"GetHealthResponse\" type=\"xsd:string\"/>\n" \
                "\t</wsdl:message>\n")
                .append(foreachService(writeServiceMessages, proto, NULL)).append(
                "\n")
                .append(foreachService(writeServicePortType, proto, NULL))
                .append(foreachService(writeServiceBinding, proto, const_cast<char*>(targetNamespace))).append(
                "\t<wsdl:service name=\"").append(protoFilenameNoExtNoPath).append("Service\">\n")
                .append(foreachService(writeServicePort, proto, &params)).append(
                "\t</wsdl:service>\n" \
                "</wsdl:definitions>\n");
        success = writeExactly(wsdl, contents.c_str(), contents.size());
    } catch (string s) {
        success = false;
        *error = s;
    }
    return success;
}

string
writeDependency(const FileDescriptor* desc, void* param)
{
    string out;
    out.append("\t\t\t<include schemaLocation=\"").append(stripExtension(desc->name())).append(".xsd\"/>\n");
    return out;
}

string
writeServiceElements(const ServiceDescriptor* desc, void* param)
{
    return foreachMethod(writeElements, desc, param);
}

string
writeElements(const MethodDescriptor* desc, void* param)
{
    string out;
    out.append(
            "\t\t\t<element name=\"").append(desc->name()).append("Input\" type=\"tns:")
                .append(desc->input_type()->name()).append("\"/>\n" \
            "\t\t\t<element name=\"").append(desc->name()).append("Output\" type=\"tns:")
                .append(desc->output_type()->name()).append("\"/>\n");
    return out;
}

string
writeServiceMessages(const ServiceDescriptor* desc, void* param)
{
    return foreachMethod(writeMessages, desc, param);
}

string
writeMessages(const MethodDescriptor* desc, void* param)
{
    string out;
    out.append(
            "\n" \
            "\t<wsdl:message name=\"").append(desc->name()).append("Request\">\n" \
            "\t\t<wsdl:part name=\"in\" element=\"tns:").append(desc->name()).append("Input\"/>\n" \
            "\t</wsdl:message>\n" \
            "\t<wsdl:message name=\"").append(desc->name()).append("Response\">\n" \
            "\t\t<wsdl:part name=\"out\" element=\"tns:").append(desc->name()).append("Output\"/>\n"
            "\t</wsdl:message>\n");
    return out;
}

string
writeServicePortType(const ServiceDescriptor* service, void* param)
{
    string out;
    out.append(
            "\t<wsdl:portType name=\"").append(service->name()).append("PortType\">\n" \
            "\t\t<wsdl:operation name=\"GetHealth\">\n" \
            "\t\t\t<wsdl:input message=\"tns:GetHealthRequest\"/>\n" \
            "\t\t\t<wsdl:output message=\"tns:GetHealthResponse\"/>\n" \
            "\t\t</wsdl:operation>\n" )
            .append(foreachMethod(writeOperation, service, NULL)).append(
            "\t</wsdl:portType>\n" \
            "\n");
    return out;
}

string
writeOperation(const MethodDescriptor* desc, void* param)
{
    string out;
    out.append(
            "\t\t<wsdl:operation name=\"").append(desc->name()).append("\">\n" \
            "\t\t\t<wsdl:input message=\"tns:").append(desc->name()).append("Request\"/>\n" \
            "\t\t\t<wsdl:output message=\"tns:").append(desc->name()).append("Response\"/>\n" \
            "\t\t</wsdl:operation>\n");
    return out;
}

string
writeServiceBinding(const ServiceDescriptor* service, void* param)
{
    string out;
    out.append(
            "\t<wsdl:binding name=\"").append(service->name()).append("SOAPBinding\" type=\"tns:")
            .append(service->name()).append("PortType\">\n" \
            "\t\t<soap:binding style=\"document\" transport=\"http://schemas.xmlsoap.org/soap/http\"/>\n" \
            "\n" \
            "\t\t<wsdl:operation name=\"GetHealth\">\n" \
            "\t\t\t<soap:operation soapAction=\"").append((char*)param).append("/GetHealth\"/>\n" \
            "\t\t\t<wsdl:input>\n" \
            "\t\t\t\t<soap:body use=\"literal\"/>\n" \
            "\t\t\t</wsdl:input>\n" \
            "\t\t\t<wsdl:output>\n" \
            "\t\t\t\t<soap:body use=\"literal\"/>\n" \
            "\t\t\t</wsdl:output>\n" \
            "\t\t</wsdl:operation>\n")
            .append(foreachMethod(writeBinding, service, param)).append(
            "\n" \
            "\t</wsdl:binding>\n" \
            "\n");
    return out;
}

string
writeBinding(const MethodDescriptor* desc, void* param)
{
    string out;
    out.append(
            "\n" \
            "\t\t<wsdl:operation name=\"").append(desc->name()).append("\">\n" \
            "\t\t\t<soap:operation soapAction=\"").append((char*)param).append("/")
                .append(desc->name()).append("\"/>\n" \
            "\t\t\t<wsdl:input>\n" \
            "\t\t\t\t<soap:body use=\"literal\"/>\n" \
            "\t\t\t</wsdl:input>\n" \
            "\t\t\t<wsdl:output>\n" \
            "\t\t\t\t<soap:body use=\"literal\"/>\n" \
            "\t\t\t</wsdl:output>\n" \
            "\t\t</wsdl:operation>\n");
    return out;
}

string
writeServicePort(const ServiceDescriptor* service, void* param)
{
    ServiceParams* params = (ServiceParams*)param;
    string out;
    out.append(
            "\t\t<wsdl:port binding=\"tns:").append(service->name()).append("SOAPBinding\" name=\"")
                .append(service->name()).append("SOAP\">\n" \
            "\t\t\t<soap:address location=\"http://").append(params->serverName).append(":")
                .append(params->port).append("/").append(params->serverName).append("/services/")
                .append(service->name()).append("SOAP\"/>\n" \
            "\t\t</wsdl:port>\n");
    return out;
}
