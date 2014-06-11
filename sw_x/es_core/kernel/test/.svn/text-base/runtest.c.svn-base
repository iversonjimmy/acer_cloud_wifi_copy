#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <km_types.h>
#include <esc_ioctl.h>

int main(int argc, char *argv[])
{
    int fd = open("/dev/esc", O_RDWR);
    if (fd < 0) {
        perror("ERROR: failed to open /dev/esc");
        exit(1);
    }

    struct esc_devtest arg;
    memset(&arg, 0, sizeof(arg));
    arg.arg1 = 1;
    arg.arg2 = 2;
    arg.arg3 = (u8*)"abcdefg123";
    arg.arg3len = sizeof("abcdefg123");
    int err = ioctl(fd, ESC_IOC_DEVTEST, &arg);
    if (err < 0) {
        perror("ERROR: failed ioctl");
        exit(1);
    }

    printf("INFO: arg1=%d arg2=%d arg3=%s ans1=%d ans2=%d ans3=%s error=%d\n",
           arg.arg1, arg.arg2, arg.arg3, arg.ans1, arg.ans2, arg.ans3, arg.error);

    close(fd);

    exit(0);
}
