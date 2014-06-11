protoxml

For some of our SOAP-based infrastructure WebServices, we define the service as Protocol Buffer messages.
(For an example, see gvm_core/vplex/proto/vplex_vs_directory_service_types.proto.)

We use the Protocol Buffer definitions (along with protorpcgen) to generate the WSDL/XSD files for infra.

We also use the Protocol Buffer definitions (along with protorpcgen) to generate boilerplate SOAP clients (*-xml.pb.h).

This protoxml module provides part of the XML parsing logic to support mapping from XML strings (within SOAP responses) to C++ protobuf objects.
This module should in general only be referenced directly by the generated boilerplate code.
