//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include <ccdi.hpp>
#include <log.h>
#include <vpl_th.h>
#include <vpl_time.h>

#include <dirent.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <cerrno>
#include <csignal>
#include <string>
#include <vector>

#define PATH_MAX_LENGTH         1024

static int closeFds()
{
    int fd = -1;
    int rv = 0;
    DIR* directory = NULL;
    struct dirent* entry = NULL;
    char path[PATH_MAX_LENGTH];

    snprintf(path, sizeof(path), "/proc/%d/fd", (int)getpid());

    directory = opendir(path);
    if (directory == NULL) {
        LOG_ERROR("opendir %s failed: %s", path, strerror(errno));
        goto out;
    }

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(".",  entry->d_name, 2) == 0 ||
	    strncmp("..", entry->d_name, 3) == 0) {
            continue;
        }

        fd = atoi(entry->d_name);
        if (fd > KMSG_FILENO) {
            close(fd);
        }
    }

    closedir(directory);
out:
    directory = NULL;
    entry = NULL;

    return rv;
}

static pid_t launchCcd(uid_t procUid)
{
    LOG_INFO("start");
    char*  argv[] = { (char *)"/bin/ccd", (char *)NULL };

    pid_t pid;
    pid = fork();
    if (pid == 0) {
        closeFds();
        setgid(procUid);
        setuid(procUid);
        LOG_INFO("executing /bin/ccd");
        execv("/bin/ccd", argv);
        abort();
    }

    LOG_INFO("CCD[pid %d] started", pid);

    return pid;
}


int main(int argc, char ** argv)
{
    LOG_DISABLE_LEVEL(LOG_LEVEL_DEBUG);

    std::string machine;
    int procUid;

    if(argc != 3) {
        LOG_INFO("Usage: %s <machine> <uid>\n", argv[0]);
        LOG_INFO("Example: %s C1 1000", argv[0]);
        return -1;
    }

    machine.assign(argv[1]);
    procUid = atoi(argv[2]);

    setgid(procUid);
    setuid(procUid);

    // Launch ccd process
    launchCcd(procUid);

    sleep(1);

    return 0;
}
