/*
 *               Copyright (C) 2009, BroadOn Communications Corp.
 *
 *  These coded instructions, statements, and computer programs contain
 *  unpublished  proprietary information of BroadOn Communications Corp.,
 *  and  are protected by Federal copyright law. They may not be disclosed
 *  to  third  parties or copied or duplicated in any form, in whole or in
 *  part, without the prior written consent of BroadOn Communications Corp.
 *
 */
#ifndef __SIG_H__
#define __SIG_H__

#include <vplu_types.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SIG_OK              0
#define SIG_ERROR_PIPE      -1
#define SIG_ERROR_SELECT    -2

pid_t SIGFork(void);
s32   SIGInit(void);
s32   SIGSelect(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exeptfds,
        struct timeval* timeout, int* signum);
s32   SIGWait(void);

#ifdef __cplusplus
}
#endif

#endif // include guard
