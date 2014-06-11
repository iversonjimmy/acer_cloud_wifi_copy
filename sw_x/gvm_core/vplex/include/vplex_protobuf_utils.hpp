//
//  Copyright (C) 2005-2010, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#ifndef __VPLEX_PROTOBUF_UTILS_HPP__
#define __VPLEX_PROTOBUF_UTILS_HPP__

#include <google/protobuf/repeated_field.h>

/// Adds better compile-time type-checking to RepeatedPtrField<Element>::MergeFrom.
/// Normally, it would accept any type of RepeatedPtrField, but we almost always want
/// it to be the same exact element type too.
template <typename Element>
void MergeRepeatedPtrField(
        google::protobuf::RepeatedPtrField<Element>& to,
        const google::protobuf::RepeatedPtrField<Element>& from)
{
    to.MergeFrom(from);
}
template <typename Element>
void MergeRepeatedPtrField(
        google::protobuf::RepeatedPtrField<Element>* to,
        const google::protobuf::RepeatedPtrField<Element>& from)
{
    to->MergeFrom(from);
}

#endif // include guard
