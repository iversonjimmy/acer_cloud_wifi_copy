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
#include <sig.h>
#include <log.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int __sigPipe[2] = { -1, -1 };

// Simply write the signal number to a pipe,
// assumes that all other signals are blocked
// It's not safe to do anything much in this signal handler, see signal(7).
static void
SIGSafeHandler(int signum)
{
    char sig[] = { signum };
    s32 rv = 0;

    rv = write(__sigPipe[1], sig, sizeof(sig));
    if (rv < sizeof(sig)) {
        abort();
    }
}

static void
SIGSetHandler(int signum, void (*handler)(int signum))
{
    LOG_DEBUG("SIGSetHandler");

    struct sigaction sa;

    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;

    sigaction(signum, &sa, NULL);
}

static void
SIGRestore(void)
{
    LOG_DEBUG("SIGRestore");

    SIGSetHandler(SIGHUP,  SIG_DFL);
    SIGSetHandler(SIGINT,  SIG_DFL);
    SIGSetHandler(SIGTERM, SIG_DFL);
    SIGSetHandler(SIGALRM, SIG_DFL);
    SIGSetHandler(SIGCHLD, SIG_DFL);
    SIGSetHandler(SIGUSR1, SIG_DFL);
    SIGSetHandler(SIGUSR2, SIG_DFL);

    SIGSetHandler(SIGPIPE, SIG_DFL);

    close(__sigPipe[1]);
    close(__sigPipe[0]);

    __sigPipe[1] = __sigPipe[0] = -1;
}

s32
SIGInit(void)
{
    LOG_DEBUG("SIGInit");

    s32 rv = SIG_OK;

    if (pipe(__sigPipe) < 0) {
        LOG_ERROR("pipe failed: %s", strerror(errno));
        rv = SIG_ERROR_PIPE;
        goto out;
    }

    // All asynchronous signals need safe handling
    SIGSetHandler(SIGHUP,  SIGSafeHandler);
    SIGSetHandler(SIGINT,  SIGSafeHandler);
    SIGSetHandler(SIGTERM, SIGSafeHandler);
    SIGSetHandler(SIGALRM, SIGSafeHandler);
    SIGSetHandler(SIGCHLD, SIGSafeHandler);
    SIGSetHandler(SIGUSR1, SIGSafeHandler);
    SIGSetHandler(SIGUSR2, SIGSafeHandler);

    // Ignore these annoying synchronous signals
    SIGSetHandler(SIGPIPE, SIG_IGN);

out:
    return rv;
}

pid_t
SIGFork(void)
{
    LOG_DEBUG("SIGFork");

    sigset_t orig_sigset;
    sigset_t blocking_sigset;
    pid_t pid;
    int errno_saved;

    // Block all signals during fork, to make sure child doesn't handle a
    // signal by writing to the parent's signal_pipe

    sigfillset(&blocking_sigset);
    sigprocmask(SIG_BLOCK, &blocking_sigset, &orig_sigset);

    pid = fork();
    errno_saved = errno;

    if (pid == 0) {
        SIGRestore();
    }

    sigprocmask(SIG_SETMASK, &orig_sigset, NULL);

    errno = errno_saved;

    return pid;
}

s32
SIGSelect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exeptfds,
        struct timeval *timeout, int *signum)
{
    LOG_DEBUG("SIGSelect");

    s32 rv = SIG_OK;
    fd_set def_read_fds;

    if (readfds == NULL) {
        readfds = &def_read_fds;
        FD_ZERO(readfds);
    }

    FD_SET(__sigPipe[0], readfds);
    if (__sigPipe[0] >= nfds) {
        nfds = __sigPipe[0] + 1;
    }

    *signum = -1;

    while (1) {
        rv = select(nfds, readfds, writefds, exeptfds, timeout);
        if (rv < 0 && errno == EINTR) {
            continue;
        } else if (rv < 0) {
            LOG_ERROR("select failed: %s", strerror(errno));
            rv = SIG_ERROR_SELECT;
            goto out;
        }

        if (FD_ISSET(__sigPipe[0], readfds)) {
            char sig[1] = { -1 };
            ssize_t cnt;

            cnt = read(__sigPipe[0], sig, sizeof(sig));
            *signum = sig[0];

            FD_CLR(__sigPipe[0], readfds);
            rv--;
        }

        goto out;
    }

out:
    return rv;
}

s32
SIGWait(void)
{
    LOG_DEBUG("SIGWait");

    s32 rv = SIG_OK;
    s32 signum = -1;

    rv = SIGSelect(0, NULL, NULL, NULL, NULL, &signum);
    if (rv < 0) {
        goto out;
    }

    rv = signum;

out:
    return rv;
}
