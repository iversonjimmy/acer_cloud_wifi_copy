/*
 *  Copyright 2013 Acer Cloud Technology, Inc.
 *  All Rights Reserved.
 *
 *  This software contains confidential information and
 *  trade secrets of Acer Cloud Technology, Inc.
 *  Use, disclosure or reproduction is prohibited without
 *  the prior express written permission of Acer Cloud
 *  Technology, Inc.
 */

#include "vplTest.h"

#include "vpl_shm.h"

#define VPLTEST_SHM_OBJECT_NAME     "/VPLTest_Shm"
#define VPLTEST_SHM_OBJECT_SIZE     (10 * 1024 * 1024)

void
testVPLShm(void)
{
    int fd, i;
    void *addr;
    u8 *p;

    // Positive tests to exercise each API
    fd = VPLShm_Open(VPLTEST_SHM_OBJECT_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        VPLTEST_FAIL("VPLShm_Open() failed with rv=%d", fd);
        goto exit;
    }
    VPLTEST_LOG("Open shared memory object with fd=%d", fd);

    VPLTEST_CALL_AND_CHK_RV(VPL_Ftruncate(fd, VPLTEST_SHM_OBJECT_SIZE), VPL_OK);
    VPLTEST_LOG("Truncated shared memory object");

    VPLTEST_CALL_AND_CHK_RV(VPL_Mmap(NULL, VPLTEST_SHM_OBJECT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0, &addr), VPL_OK);
    VPLTEST_LOG("Mapped shared memory object at addr=%p", addr);

    p = (u8 *) addr;
    for (i = 0; i < VPLTEST_SHM_OBJECT_SIZE; i++) {
        p[i] = (i & 0xff);
    }
    for (i = 0; i < VPLTEST_SHM_OBJECT_SIZE; i++) {
        if (p[i] != (i & 0xff)) {
            VPLTEST_FAIL("Shared memory read-back check failed at pos=%d", i);
            goto exit;
        }
    }
    VPLTEST_LOG("Check shared memory write and read-back");

    VPLTEST_CALL_AND_CHK_RV(VPL_Munmap(addr, VPLTEST_SHM_OBJECT_SIZE), VPL_OK);
    VPLTEST_LOG("Unmapped shared memory object");

    VPLTEST_CALL_AND_CHK_RV(VPL_Close(fd), VPL_OK);
    VPLTEST_LOG("Closed shared memory fd");

    VPLTEST_CALL_AND_CHK_RV(VPLShm_Unlink(VPLTEST_SHM_OBJECT_NAME), VPL_OK);
    VPLTEST_LOG("Unlinked shared memory object");

    // A few negative test cases
    VPLTEST_CALL_AND_CHK_RV(VPLShm_Open(VPLTEST_SHM_OBJECT_NAME, 0, 0), VPL_ERR_NOENT);

    VPLTEST_CALL_AND_CHK_RV(VPL_Ftruncate(-1, 0), VPL_ERR_BADF);

    VPLTEST_CALL_AND_CHK_RV(VPLShm_Unlink(VPLTEST_SHM_OBJECT_NAME), VPL_ERR_NOENT);

    VPLTEST_CALL_AND_CHK_RV(VPL_Close(-1), VPL_ERR_BADF);

    VPLTEST_CALL_AND_CHK_RV(VPL_Mmap(NULL, VPLTEST_SHM_OBJECT_SIZE, PROT_READ | PROT_WRITE, 0, -1, 0, &addr), VPL_ERR_BADF);

    VPLTEST_CALL_AND_CHK_RV(VPL_Munmap((void *) -1, VPLTEST_SHM_OBJECT_SIZE), VPL_ERR_INVALID);

exit:
    NO_OP();
}
