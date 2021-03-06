// Generated by the protocol buffer compiler.  DO NOT EDIT!

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "vplex_community_service_types.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace vplex {
namespace community {

namespace {

const ::google::protobuf::Descriptor* SessionInfo_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  SessionInfo_reflection_ = NULL;

}  // namespace


void protobuf_AssignDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto() {
  protobuf_AddDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "vplex_community_service_types.proto");
  GOOGLE_CHECK(file != NULL);
  SessionInfo_descriptor_ = file->message_type(0);
  static const int SessionInfo_offsets_[2] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(SessionInfo, sessionhandle_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(SessionInfo, serviceticket_),
  };
  SessionInfo_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      SessionInfo_descriptor_,
      SessionInfo::default_instance_,
      SessionInfo_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(SessionInfo, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(SessionInfo, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(SessionInfo));
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    SessionInfo_descriptor_, &SessionInfo::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_vplex_5fcommunity_5fservice_5ftypes_2eproto() {
  delete SessionInfo::default_instance_;
  delete SessionInfo_reflection_;
}

void protobuf_AddDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n#vplex_community_service_types.proto\022\017v"
    "plex.community\";\n\013SessionInfo\022\025\n\rsession"
    "Handle\030\001 \002(\006\022\025\n\rserviceTicket\030\002 \002(\014B(\n\017i"
    "gware.vplex.pbB\025CommunityServiceTypes", 157);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "vplex_community_service_types.proto", &protobuf_RegisterTypes);
  SessionInfo::default_instance_ = new SessionInfo();
  SessionInfo::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_vplex_5fcommunity_5fservice_5ftypes_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_vplex_5fcommunity_5fservice_5ftypes_2eproto {
  StaticDescriptorInitializer_vplex_5fcommunity_5fservice_5ftypes_2eproto() {
    protobuf_AddDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto();
  }
} static_descriptor_initializer_vplex_5fcommunity_5fservice_5ftypes_2eproto_;


// ===================================================================

#ifndef _MSC_VER
const int SessionInfo::kSessionHandleFieldNumber;
const int SessionInfo::kServiceTicketFieldNumber;
#endif  // !_MSC_VER

SessionInfo::SessionInfo()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void SessionInfo::InitAsDefaultInstance() {
}

SessionInfo::SessionInfo(const SessionInfo& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void SessionInfo::SharedCtor() {
  _cached_size_ = 0;
  sessionhandle_ = GOOGLE_ULONGLONG(0);
  serviceticket_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

SessionInfo::~SessionInfo() {
  SharedDtor();
}

void SessionInfo::SharedDtor() {
  if (serviceticket_ != &::google::protobuf::internal::kEmptyString) {
    delete serviceticket_;
  }
  if (this != default_instance_) {
  }
}

void SessionInfo::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* SessionInfo::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return SessionInfo_descriptor_;
}

const SessionInfo& SessionInfo::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_vplex_5fcommunity_5fservice_5ftypes_2eproto();  return *default_instance_;
}

SessionInfo* SessionInfo::default_instance_ = NULL;

SessionInfo* SessionInfo::New() const {
  return new SessionInfo;
}

void SessionInfo::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    sessionhandle_ = GOOGLE_ULONGLONG(0);
    if (has_serviceticket()) {
      if (serviceticket_ != &::google::protobuf::internal::kEmptyString) {
        serviceticket_->clear();
      }
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool SessionInfo::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required fixed64 sessionHandle = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_FIXED64) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint64, ::google::protobuf::internal::WireFormatLite::TYPE_FIXED64>(
                 input, &sessionhandle_)));
          set_has_sessionhandle();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(18)) goto parse_serviceTicket;
        break;
      }
      
      // required bytes serviceTicket = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_serviceTicket:
          DO_(::google::protobuf::internal::WireFormatLite::ReadBytes(
                input, this->mutable_serviceticket()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }
      
      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void SessionInfo::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required fixed64 sessionHandle = 1;
  if (has_sessionhandle()) {
    ::google::protobuf::internal::WireFormatLite::WriteFixed64(1, this->sessionhandle(), output);
  }
  
  // required bytes serviceTicket = 2;
  if (has_serviceticket()) {
    ::google::protobuf::internal::WireFormatLite::WriteBytes(
      2, this->serviceticket(), output);
  }
  
  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* SessionInfo::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required fixed64 sessionHandle = 1;
  if (has_sessionhandle()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteFixed64ToArray(1, this->sessionhandle(), target);
  }
  
  // required bytes serviceTicket = 2;
  if (has_serviceticket()) {
    target =
      ::google::protobuf::internal::WireFormatLite::WriteBytesToArray(
        2, this->serviceticket(), target);
  }
  
  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int SessionInfo::ByteSize() const {
  int total_size = 0;
  
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required fixed64 sessionHandle = 1;
    if (has_sessionhandle()) {
      total_size += 1 + 8;
    }
    
    // required bytes serviceTicket = 2;
    if (has_serviceticket()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::BytesSize(
          this->serviceticket());
    }
    
  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void SessionInfo::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const SessionInfo* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const SessionInfo*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void SessionInfo::MergeFrom(const SessionInfo& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_sessionhandle()) {
      set_sessionhandle(from.sessionhandle());
    }
    if (from.has_serviceticket()) {
      set_serviceticket(from.serviceticket());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void SessionInfo::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void SessionInfo::CopyFrom(const SessionInfo& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool SessionInfo::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000003) != 0x00000003) return false;
  
  return true;
}

void SessionInfo::Swap(SessionInfo* other) {
  if (other != this) {
    std::swap(sessionhandle_, other->sessionhandle_);
    std::swap(serviceticket_, other->serviceticket_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata SessionInfo::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = SessionInfo_descriptor_;
  metadata.reflection = SessionInfo_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace community
}  // namespace vplex

// @@protoc_insertion_point(global_scope)
