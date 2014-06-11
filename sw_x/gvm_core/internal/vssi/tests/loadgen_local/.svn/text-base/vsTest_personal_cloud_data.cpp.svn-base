/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "vpl_th.h"
#include "vplex_trace.h"
#include "vplex_time.h"
#include "vssi.h"
#include "vssi_error.h"

#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <glob.h>


#include "vsTest_personal_cloud_data.hpp"

using namespace std;

extern int loadgen_round_size(u32 *size);
extern const char* vsTest_curTestName;

static void check_io_fill(char *buf, int offset, int xfer_size, const char* marker, int marklen);
static int check_io_check(char *buf, int offset, int xfer_size, const char* marker, int marklen);
#define CHECK_BLOCKSIZE 4096
static void check_io_fill(char *buf, int offset, int xfer_size, const char* marker, int marklen)
{
    int i;
    int hlen;
    union {
        int u_i;
        char u_c[4];
    } u;
    hlen = sizeof(u);

    for (i = 0; i < xfer_size; i += CHECK_BLOCKSIZE) {
        u.u_i = offset + i;
        memcpy(buf + i, u.u_c, hlen);
        memcpy(buf + i + hlen, marker, marklen);
    }
}

static int check_io_check(char *buf, int offset, int xfer_size, const char* marker, int marklen)
{
    int rv = 0;
    int i;
    union {
        int u_i;
        char u_c[4];
    } u, v;
    const int hlen = sizeof(u);

    for (i = 0; i < xfer_size; i += CHECK_BLOCKSIZE) {
        u.u_i = offset + i;
        memcpy(v.u_c, buf + i, hlen);
        if (v.u_i != u.u_i) {
            rv++;
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: block mismatch at offset 0x%x found 0x%x %s",
                u.u_i, v.u_i, buf + i + sizeof(u));
        }
        if (memcmp(buf  + i + hlen, marker, marklen) != 0) {
            rv++;
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:" "marker mismatch at offset 0x%x found %.*s not %.*s",
                u.u_i, marklen, buf + hlen, marklen, marker);
        }
    }
    return(rv);
}



static void
pctest_idle(void *v)
{
}

static int
local_rw_file(int fd, u32 offset, u32 xfer_size, char *buf, bool do_read, bool do_seek, const char *marker, const int marklen)
{
  int rv = -1;

  if (fd < 0) { goto out;}

  if (do_seek) {
      rv = lseek(fd, offset, SEEK_SET);
      if (-1 == rv) {
          goto out;
      }
  }

  if ((!do_read) && check_io) {
        check_io_fill(buf, offset, xfer_size, marker, marklen);
  }

  rv = (do_read)? read(fd, buf, xfer_size): write(fd, buf, xfer_size);
  if (-1 == rv || (u32)rv < xfer_size) {
      printf("io failed: %d\n", errno);
      goto out;
  }

  if (do_read && check_io) {
      rv = check_io_check(buf, offset, xfer_size, marker, marklen);
  } else {
      rv = 0; /* caller expects 0 on success */
  }
out:
  return rv;
}

static void
local_close_file(int fd)
{
    if (fd >= 0) { close(fd); }
}

static int
local_open_file(const char *path, const char *file, bool do_read, bool do_direct)
{
    int fd;
    char fname[PATH_MAX + 1];
    int flag;

    fname[PATH_MAX] = '\0';
    if (strcmp(path, "")) {
        snprintf(fname, sizeof(fname) - 1, "%s/%s", path, file);
    } else {
        snprintf(fname, sizeof(fname) - 1, "%s", file);
    }

    if (do_read) {
        flag = O_RDONLY;
        if (do_direct) {
            flag |= O_DIRECT;
        }
        fd = open(fname, flag);
        if (fd < 0) {
//            printf("XXX failed to open %s: %d\n", fname, errno);
        }
        goto out;
    }

    flag = O_WRONLY|O_CREAT|O_TRUNC;
    if (do_direct) {
        flag |= O_DIRECT;
    }
    fd = open(fname, flag, S_IRWXU | S_IRWXG | S_IRWXO);
out:
    return fd;
}

#define LOADGEN_LOCAL_DIR_MODE 0777
#define SLASH 1
#define PATH  2
int
local_create_dirs(const char *path1, const char *path2)
{
  int rv;
  char tmp[1024];
  int state;
  int i = 1, len = 1;
  char path[PATH_MAX + 1];
  
  path[0] = path[PATH_MAX] = '\0';
  if (NULL != path1 && strcmp(path1, "")) {
      snprintf(path, sizeof(path) - 1, "%s/", path1);
  }
  if (NULL != path2) {
//      printf("Before: Path = %s path2 = %s\n", path, path2);
     if (path[0] != '\0') {
         strncat(path, path2, sizeof(path) - 1);
     } else {
         snprintf(path, sizeof(path) - 1, "%s/", path2);
     }
//      printf("After:Path = %s path2 = %s\n", path, path2);
  }
  if (path[0] == '\0') {
      rv = -1;
      goto out;
  }

//  printf("Will create all components in path: %s\n", path);

  state = (path[0] == '/')? SLASH : PATH;
  tmp[0] = path[0];
  i = 1;
  len = 1;
  while (path[i] != '\0') {
      if (state == PATH) {
          if (path[i] != '/') {
              tmp[len] = path[i];
              ++len;
          } else {
              tmp[len] = path[i];
              ++len;
              tmp[len] = '\0';
              rv = mkdir(tmp, LOADGEN_LOCAL_DIR_MODE);
              if (rv && errno != EEXIST) {
                  printf("Creating path %s failed: %d\n", tmp, errno);
                  goto out;
              }
//              printf("Created path: %s\n", tmp);
              state = SLASH;
          } 
      } else { /* state == SLASH */
          if (path[i] != '/') {
              state = PATH;
              tmp[len] = path[i];
              ++len;
          } 
          /* Else: skip extra slashes  */
      }
      ++i;
  }
  tmp[len]= '\0';
//  printf("end: Skipping file: %s\n", tmp);
  rv = 0;

out:
  return rv;
}

static int 
local_rdwrt_file(pctest_context_t &test_context,
                             const char *file,
                             u32 file_size,
                             tpc_thread_t *thread,
                             tpc_parms_t *parms, bool do_read,
                             clock_t *op_clocks)
{
    int rv = 0;
    int fd = -1;
    u32 xfer_tot = 0;
    u32 xfer_cur;
    clock_t st_time;
    clock_t en_time;
    struct tms st_cpu;
    struct tms en_cpu;
    bool do_seek = false; /* XXX sequential rw for now */
    char path[PATH_MAX + 1];
    path[PATH_MAX] = '\0';
    int marklen;
 

    generate_path_prefix(path, sizeof(path), parms->path_prefix, thread->id);
    marklen = strlen(path);
    fd = local_open_file(path, file, do_read, parms->direct_xfer);
    if (do_read) {
       VPLTRACE_LOG_FINE(TRACE_APP, 0, "Reading from file: %s/%s\n", path, file);
    } else {
        if (fd < 0 && errno == ENOENT) {
            // Create directories in path and try open() again
            local_create_dirs(path, file);
            fd = local_open_file(path, file, do_read, parms->direct_xfer);
            if (fd < 0) {
                printf("XXX after path creation: open(%s/%s) failed\n", path, file);
            }
        }

       VPLTRACE_LOG_FINE(TRACE_APP, 0, "Writing to file: %s/%s\n", path, file);
    }
       

    if (fd < 0) {
        rv = 1;
        goto out;
    }

    st_time = times(&st_cpu);
    if ( test_context.io_xfered == 0 ) {
        test_context.io_start = VPLTime_GetTimeStamp();
    }
    while (xfer_tot < file_size) {
        xfer_cur = MIN(file_size - xfer_tot, parms->xfer_size);
        if ( parms->xfer_delay_time ) {
            xfer_cur = MIN(xfer_cur, parms->xfer_delay_size - test_context.io_xfered);
        }
        rv = local_rw_file(fd, xfer_tot, xfer_cur, thread->xfer_buf, do_read, do_seek, path, marklen);
        if (rv != 0) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL:"
                             "%s file /%s: %d, expected %d.",
                             do_read ? "Read" : "Write",
                             file, rv, 0);
            rv = 1;
            goto out;
        }
    
        xfer_tot += xfer_cur;

        // try to throttle here.
        if ( parms->xfer_delay_time ) {
            VPLTime_t t1, tdelta;
            test_context.io_xfered += xfer_cur;
            if ( test_context.io_xfered < parms->xfer_delay_size ) {
                continue;
            }
            test_context.io_xfered = 0;
            t1 = VPLTime_GetTimeStamp();
            tdelta = t1 - test_context.io_start;
            if(tdelta < parms->xfer_delay_time) {
                tdelta = parms->xfer_delay_time - tdelta;
                VPLThread_Sleep(tdelta);
            }
            else {
                thread->rate_underrun_cnt++;
            }
            test_context.io_start += parms->xfer_delay_time;
        }
    }
    en_time = times(&en_cpu);
    *op_clocks = en_time - st_time;
out:
    local_close_file(fd);
    return rv;
}

static const char vsTest_personal_cloud_data [] = "Personal Cloud Data Test";
void test_personal_cloud(tpc_thread_t *thread)
{
    int rv = 0;
   
    tpc_parms_t *parms;
    u32 i, j;
    pctest_context_t test_context;
    bool do_read[10];
    bool is_read;
    bool thread_ok = true;
    clock_t op_clocks;
    char tmp[24];
    const char *file;
    u32 file_size;

    parms = thread->parms;
    thread->failed = false;
    thread->rv = -1;
    // Set up a read/write pattern
    thread->seed = thread->id;
    
    for( i = 0 ; i < 10 ; i++ ) {
        if ( parms->rw_only ) {
            do_read[i] = thread->rw_reader;
        }
        else {
            do_read[i] = ( i < ((parms->rw_ratio+5)/10) );
        }
    }
    // mix it up
    for( i = 0 ; i < 10 ; i++ ) {
        j = rand_r(&thread->seed) % 10;
        if ( i == j ) {
            continue;
        }
        is_read = do_read[i];
        do_read[i] = do_read[j];
        do_read[j] = is_read;
    }

    vsTest_curTestName = vsTest_personal_cloud_data;

    memset(&test_context, 0, sizeof(test_context));

    // Tell main thread we're ready to go.

    if (thrd_ready_wait(thread_ok,
        &test_context, ((thread->id == 0) ? pctest_idle : NULL))) {
        rv++;
        goto fail_open;
    }

    thread->start = time(NULL);
    // Create file_cnt files
    for( i = 0 ; ; i++ ) {
        // See if we're done - We must write out our files
        if (i >= parms->file_cnt) {
            // Stop if we're a writer thread
            if ( ! thread->rw_reader ) {
                break;
            }
            // now see if we need to stick around
            if ( !parms->rw_only || !parms->writer_wait || writers_are_done ) {
                break;
            }
        }
        if ( pct_threads_stop ) {
            rv++;
            break;
        }
        if ( parms->manifest ) {
            file = parms->manifest->array[i % parms->file_cnt]->path; 
            file_size = parms->manifest->array[i % parms->file_cnt]->size;
            if (parms->direct_xfer) {
                loadgen_round_size(&file_size);
            }
        } else {
            // We mod the file_cnt for the case where readers are waiting
            // for writers to finish.
            snprintf(tmp, sizeof(tmp) - 1, "file%05d-%04d", parms->file_size / 1024, i % parms->file_cnt);
            file = tmp;
            file_size = parms->file_size;
        }
        // file size already rounded up in main()
        is_read = do_read[i % 10];
        if ( is_read ) {
            rv += local_rdwrt_file(test_context, file, file_size, thread, parms, true, &op_clocks);
            if ( rv == 0 ) {
                thread->read_time += op_clocks;
                thread->reads++;
                thread->data_read_k += file_size / 1024;
            }
        } else {
            rv += local_rdwrt_file(test_context, file, file_size, thread, parms, false, &op_clocks);
            if ( rv == 0 ) {
                thread->write_time += op_clocks;
                thread->writes++;
                thread->data_write_k += file_size / 1024;
            }
        }

        if ( rv ) {
            VPLTRACE_LOG_ERR(TRACE_APP, 0, "FAIL: thread %p %d user_id %X ticks %d %s file %s\n", 
                            thread, thread->id, (int) thread->user_id, (int) op_clocks,
                             is_read ? "read" : "write", file);
            thread->failed = true;
            pct_threads_stop = true;
            break;
        }
    }
    thread->stop = time(NULL);

 fail_open:

    // tell the world that this writer has completed.
    if ( parms->rw_only && !thread->rw_reader ) {
        thrd_writer_done();
    }

    thread->rv = rv;
    VPLThread_Exit(&thread->dontcare);
}

void
generate_path_prefix(char *path, int pathmax, const char *prefix, int threadnum)
{
    static glob_t subdirs;
    static unsigned int nsubdirs = 0;
    if (0 == nsubdirs && NULL != prefix) {
        int rv;
        char **sp;
        if ((rv = glob(prefix, GLOB_MARK | GLOB_BRACE | GLOB_ONLYDIR, NULL, &subdirs)) != 0) {
            printf("Glob on prefix %s failed, %d", prefix, rv);
            exit(1);
        }
        // Check that we only return directories
        for (sp = subdirs.gl_pathv; NULL != *sp; sp++) {
            if (sp[0][strlen(*sp) - 1] == '/') {
                VPLTRACE_LOG_ALWAYS(TRACE_APP, 0, "Prefix dir %d: %s", nsubdirs, *sp);
                nsubdirs++;
            }
        }
        if (nsubdirs != subdirs.gl_pathc) {
            printf("Unexpected diretories matched!");
            for (sp = subdirs.gl_pathv; NULL != *sp; sp++) {
                printf("\t%s", *sp);
            }
            exit(1);
        }
    }
    if (NULL != prefix) {
        int dirchoice = threadnum % subdirs.gl_pathc;
        snprintf(path, pathmax, "%s%04d/", subdirs.gl_pathv[dirchoice], threadnum);
    } else {
        snprintf(path, pathmax, "%04d/", threadnum);
    }
}
