//
//  Copyright (C) 2005-2008, BroadOn Communications Corp.
//
//  These coded instructions, statements, and computer programs contain
//  unpublished  proprietary information of BroadOn Communications Corp.,
//  and are protected by Federal copyright law. They may not be disclosed
//  to third parties or copied or duplicated in any form, in whole or in
//  part, without the prior written consent of BroadOn Communications Corp.
//

#include "vpl_th.h"

#include "vplex_time.h"

void VPLTime_SleepSec(unsigned int seconds)
{
    VPLTime_t timeout = seconds * 1000000;
    VPLThread_Sleep(timeout);
}


void VPLTime_SleepMs(unsigned int milliseconds)
{
    VPLTime_t timeout = milliseconds * 1000;
    VPLThread_Sleep(timeout);
}
