#ifndef __VPL__MONITOR_DIR_H__
#define __VPL__MONITOR_DIR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>

#define EVENT_SIZE ( sizeof( struct inotify_event ) )
#define EVENT_BUFF_LEN ( 1024 * ( EVENT_SIZE + 16 ) )
#define INOTIFY_MASK IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO
#define SYNC_INOTIFY_STOP 'S'
#define SYNC_INOTIFY_REFRESH 'R'

typedef struct fdset {
    fd_set fdset;
    int maxfd;
} vpl_monitor_fdset;

typedef struct wd {
    int wd;
    int parent_wd;
    char *dirname;
    char *pathname;
    struct wd *prev;
    struct wd *next;
} vpl_monitor_wd;

typedef struct handle {
    int fd;
    VPLFS_MonitorCallback callback;
    VPLFS_MonitorEvent *event_buff;
    vpl_monitor_wd *wd_list_head;
    struct handle *prev;
    struct handle *next;
} vpl_monitor_handle;

int vpl_monitor_add_watch( vpl_monitor_handle *, char *, char * );
int vpl_monitor_check_directory( const char * );
int vpl_monitor_delete_wd( vpl_monitor_wd * );
int vpl_monitor_find_handle( VPLFS_MonitorHandle, vpl_monitor_handle ** );
int vpl_monitor_find_wd( vpl_monitor_handle *, int, vpl_monitor_wd ** );
int vpl_monitor_find_wd_by_name( vpl_monitor_handle *, int, char *, vpl_monitor_wd ** );
int vpl_monitor_find_wd_by_path( vpl_monitor_handle *, const char *, vpl_monitor_wd ** );
int vpl_monitor_insert_handle( vpl_monitor_handle * );
int vpl_monitor_insert_wd( vpl_monitor_handle *, vpl_monitor_wd * );
int vpl_monitor_release_handle( vpl_monitor_handle * );
int vpl_monitor_rm_watch( vpl_monitor_handle *, vpl_monitor_wd * );
int vpl_monitor_walk_through( vpl_monitor_handle *, const char * );

static int _recurs_directory( char *, vpl_monitor_handle * );
static void * vpl_monitor_main_thread( void * );

#ifdef __cplusplus
}
#endif
#endif // __VPL__MONITOR_DIR_H__

