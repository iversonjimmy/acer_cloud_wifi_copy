//
//  Copyright (C) 2007-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vplexTest.h"

const char* vplexErrToString(int errCode)
{
    switch(errCode) {
#define ROW(code, name) \
    case name: return #name;
    VPLEX_ERR_CODES
#undef ROW
    }
    return vplErrToString(errCode);
}
