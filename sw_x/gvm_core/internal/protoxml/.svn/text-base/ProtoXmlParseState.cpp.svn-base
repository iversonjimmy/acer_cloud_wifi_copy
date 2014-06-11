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

#include "ProtoXmlParseState.h"

#include "ProtoXmlParseStateImpl.h"

ProtoXmlParseState::ProtoXmlParseState(const char* outerTag,
        ProtoMessage* message)
    : impl(new ProtoXmlParseStateImpl(outerTag, message))
{}

ProtoXmlParseState::~ProtoXmlParseState(void)
{
    delete impl;
}

_VPLXmlReader&
ProtoXmlParseState::reader(void)
{
    return impl->reader();
}

bool
ProtoXmlParseState::hasError(void)
{
    return impl->hasError();
}

const std::string&
ProtoXmlParseState::errorDetail(void)
{
    return impl->errorDetail();
}
