/*
 *  Copyright 2012 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#define _XOPEN_SOURCE 600

#include <stdlib.h>

#include "vpl_conv.h"
#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_trace.h"
 #include "vplex_assert.h"

#include "vssi_error.h"
#include "vssi_types.h"
#include "vss_object.hpp"
#include "vss_file.hpp"

using namespace std;

//
// VSS flock class implements byte range locks
//

// vss_flock constructor
vss_flock::vss_flock(vss_object* object,
                     u64 origin, 
                     u64 mode, 
                     u64 start,
                     u64 end) 
:   object(object),
    origin(origin),
    mode(mode),
    start(start),
    end(end),
    next(NULL)
{
}

vss_flock::~vss_flock()
{
}

// Intervals are [start,end) meaning that "end" is first byte not covered
bool vss_flock::overlaps(u64 start2, u64 end2)
{
    if((start2 >= end) || (start >= end2)) return false;
    return true;
}

bool vss_flock::matches_object(vss_object* object2)
{
    return (object == object2);
}

bool vss_flock::matches_origin(u64 origin2)
{
    return (origin == origin2);
}

bool vss_flock::matches(u64 origin2, u64 mode2, u64 start2, u64 end2)
{
    return (origin == origin2) && (mode == mode2) && (start == start2) && (end == end2);
}

//
// Implement the rules for IO interacting with byte range locks.
// Note that these are different than the rules for setting locks.
//
bool vss_flock::io_conflicts(u64 origin2, u64 mode2, u64 start2, u64 end2)
{
    if(!overlaps(start2, end2)) return false;

    //
    // On the same descriptor, some conflicting accesses are allowed:
    //
    // A read is legal on a segment locked for WRITE_EXCL by the same handle,
    // but writes are not allowed on READ_SHARED segments even on the same handle.
    //
    if(origin == origin2) {
        // Only fail if writing on a read locked segment
        if((mode & VSSI_FILE_LOCK_READ_SHARED)
           && (mode2 & VSSI_FILE_LOCK_WRITE_EXCL)) return true;
        return false;
    }

    // Allow shared read locks, otherwise fail
    if((mode & VSSI_FILE_LOCK_WRITE_EXCL) != 0) return true;
    if((mode2 & VSSI_FILE_LOCK_WRITE_EXCL) != 0) return true;

    if(((mode & VSSI_FILE_LOCK_READ_SHARED) != 0) && ((mode2 & VSSI_FILE_LOCK_READ_SHARED) != 0))
        return false;

    return true;
}

//
// Implement the rules for conflicts on setting byte range locks.
//
// These are different than the rules for io interacting with byte range
// locks:  write locks are not allowed to overlap with any other lock,
// even on the same file handle.
//
bool vss_flock::lock_conflicts(u64 origin2, u64 mode2, u64 start2, u64 end2)
{
    if(!overlaps(start2, end2)) return false;

    //
    // On the same descriptor, a READ_SHARED lock can overlay a WRITE_EXCL,
    // but not the other way around.  In that overlay case, the READ_SHARED
    // cannot be further shared by other descriptors.
    //
    if(origin == origin2) {
        // Only allow the case if READ overlapping WRITE
        if((mode & VSSI_FILE_LOCK_WRITE_EXCL)
           && (mode2 & VSSI_FILE_LOCK_READ_SHARED)) return false;
    }

    // Write locks can not overlap even on the same descriptor
    if((mode & VSSI_FILE_LOCK_WRITE_EXCL) != 0) return true;
    if((mode2 & VSSI_FILE_LOCK_WRITE_EXCL) != 0) return true;

    // Read locks are the only ones that can overlap
    if(((mode & VSSI_FILE_LOCK_READ_SHARED) != 0) && ((mode2 & VSSI_FILE_LOCK_READ_SHARED) != 0))
        return false;

    return true;
}

//
// VSS Oplock Class implements cache state control locks
//
vss_oplock::vss_oplock(vss_object* object,
                       u64 mode)
:   object(object),
    mode(mode),
    next(NULL)
{
}

vss_oplock::~vss_oplock()
{
}

bool vss_oplock::matches(vss_object* object2)
{
    return (object == object2);
}

void vss_oplock::set_mode(u64 new_mode)
{
    mode = new_mode;
}

void vss_oplock::get_mode(u64& ret_mode)
{
    ret_mode = mode;
}

//
// Determine whether an IO or oplock request requires breaking this
// oplock and generate notification if so
//
bool vss_oplock::need_to_break(vss_file *file,
                               vss_object* object_in,
                               bool write_request,
                               bool send_break)
{
    bool conflict = false;

    //
    // If the oplock is for write, it conflicts with any request
    // from another client, but not with reads or writes from the
    // same client
    // 
    if(mode & VSSI_FILE_LOCK_CACHE_WRITE) {
        if(object != object_in) conflict = true;
    }
    //
    // If the oplock is for read, then it conflicts with a write
    // request from any other client
    //
    if(mode & VSSI_FILE_LOCK_CACHE_READ) {
        if(write_request && (object != object_in)) conflict = true;
    }
    //
    // Note that a break is sent only for a conflicting io request.
    // For conflicting lock requests, an error is returned to the caller.
    //
    if(conflict && send_break) {
        this->notify(file);
    }

    return conflict;
}

//
// Send an oplock break notification for the current oplock
//
void vss_oplock::notify(vss_file* file)
{
    VSSI_NotifyMask event = VSSI_NOTIFY_OPLOCK_BREAK_EVENT;
    char oplock_break_msg[VSS_OPLOCK_BREAK_SIZE];
    u32 file_id = file->get_file_id();

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                      "Object %p filehandle %p (file id %x) notified of break for mode "FMTx64,
                      object, file, file_id, mode);

    vss_oplock_break_set_mode(oplock_break_msg, mode);
    vss_oplock_break_set_handle(oplock_break_msg, file_id);

    file->get_dataset()->server->send_notify_event(object,
                                                   event,
                                                   oplock_break_msg,
                                                   sizeof(oplock_break_msg));
}

///
/// VSS File Handle Class
///
vss_file::vss_file(dataset* dset,
                   VPLFile_handle_t fd,
                   const std::string& component,
                   const std::string& filename,
                   u32 flags,
                   u32 attrs,
                   u64 size,
                   bool is_modified)
:   refcount(1),
    attrs_orig(attrs),
    attrs(attrs),
    size_orig(size),
    size(size),
    is_modified(is_modified),
    new_modify_time(is_modified),
    delete_pending(false),
    mydataset(dset),
    locks(NULL),
    numlocks(0),
    oplocks(NULL),
    numoplocks(0),
    fd(fd),
    component(component),
    filename(filename)
{
    VPLTime_t curtime = VPLTime_GetTime();

    access_mode = flags;

    // Calculate a unique file_id to be passed back to the client
    // as the server file handle
    // TODO: This should really be a UUID, but that would require
    // changing the protocol to pass more data.
    file_id = (u32) (curtime & 0xffffffff) ^ (u32)this;

    VPLMutex_Init(&flock_mutex);

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "make vss_file for component {%s}, filename {%s}: file_id %x access %x",
                      component.c_str(), filename.c_str(), file_id, access_mode);
}

vss_file::~vss_file()
{
    // In case there are any waiters, release them.
    // Of course they will fail, since this handle is going away.
    // This is also a race in that the waiters could run before
    // this completes.  We should be saved by the fact that there
    // is currently only one server thread per dataset, meaning
    // that there is no other thread to start processing the
    // requeued requests before this routine is completed.
    // TODO: come up with a less fragile solution.
    wake_all();

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "numlocks %d, numoplocks %d.", numlocks, numoplocks);

    // Close the actual FD if it is defined
    if(VPLFile_IsValidHandle(fd)) {
        int rc;
        rc = VPLFile_Close(fd);
        if(rc != VPL_OK) {
            VPLTRACE_LOG_WARN(TRACE_BVS, 0, "VPL Close fd %d returns %d.", fd, rc);
        }
        fd = -1;
    }

    // Free any file locks
    VPLMutex_Lock(&flock_mutex);
    vss_flock* flp = locks;
    vss_flock* next;

    // TODO: this is the last close by defn so there should be no locks, right?
    while (flp != NULL) {
        next = flp->next;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "lock removed: O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64,
                            flp->origin, flp->mode, flp->start, flp->end);
        delete flp;
        numlocks--;
        flp = next;
    }
    locks = NULL;

    // Free oplocks
    vss_oplock* olp = oplocks;
    vss_oplock* olnext;
    while (olp != NULL) {
        olnext = olp->next;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "oplock removed: %p.", olp);
        delete olp;
        numoplocks--;
        olp = olnext;
    }
    oplocks = NULL;

    // Free any remaining entries in the object map
    std::map<vss_object*,object_reference*>::iterator oref_it;
    vss_object* object;
    object_reference* orp;
 
    oref_it = object_refs.begin();
    while(oref_it != object_refs.end()) {
        object = oref_it->first;
        orp =  oref_it->second;
        object_refs.erase(object);
        delete orp;
        oref_it++;
    }

    VPLMutex_Unlock(&flock_mutex);

    if(numlocks != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "numlocks %d != 0 in vss_file destructor", numlocks);
    }
    if(numoplocks != 0) {
        VPLTRACE_LOG_WARN(TRACE_BVS, 0, "numoplocks %d != 0 in vss_file destructor", numoplocks);
    }

    VPLMutex_Destroy(&flock_mutex);
}

VPLFile_handle_t vss_file::get_fd()
{
    return fd;
}

bool vss_file::check_mode_conflict(u32 oldflags, u32 newflags)
{
    bool conflict = false;

    //
    // Check that new open mode does not conflict with the established access and sharing modes
    //
    // First check that what the new ref wants is shared by the previous references.
    // New access modes can be added (if they don't conflict), but not new sharing modes.
    //
    if (newflags & VSSI_FILE_OPEN_READ) {
        if (!(oldflags & VSSI_FILE_SHARE_READ)) {
            conflict = true;
            goto done;
        }
    }
    if (newflags & VSSI_FILE_OPEN_WRITE) {
        if (!(oldflags & VSSI_FILE_SHARE_WRITE)) {
            conflict = true;
            goto done;
        }
    }
    if (newflags & VSSI_FILE_OPEN_DELETE) {
        if (!(oldflags & VSSI_FILE_SHARE_DELETE)) {
            conflict = true;
            goto done;
        }
    }

    // Next check that the new ref is willing to share what the previous accessors are doing
    if (oldflags & VSSI_FILE_OPEN_READ) {
        if (!(newflags & VSSI_FILE_SHARE_READ)) {
            conflict = true;
            goto done;
        }
    }
    if (oldflags & VSSI_FILE_OPEN_WRITE) {
        if (!(newflags & VSSI_FILE_SHARE_WRITE)) {
            conflict = true;
            goto done;
        }
    }
    if (oldflags & VSSI_FILE_OPEN_DELETE) {
        if (!(newflags & VSSI_FILE_SHARE_DELETE)) {
            conflict = true;
            goto done;
        }
    }

 done:
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "new flags %x %s with old flags %x",
                        newflags, conflict ? "conflict" : "compatible", oldflags);
    return conflict;
}

s16 vss_file::add_reference(u32 flags)
{
    s16 rv = VSSI_SUCCESS;
    std::map<u64, u32>::iterator or_it;
    bool conflict = false;

    VPLMutex_Lock(&flock_mutex);

    //
    // Check for mode conflicts with all the existing references
    //
    or_it = origin_flags.begin();
    while (or_it != origin_flags.end()) {
        if (check_mode_conflict(or_it->second, flags)) {
            conflict = true;
            break;
        }
        or_it++;
    }

    VPLMutex_Unlock(&flock_mutex);

    if (conflict) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "add reference for {%s} fails with mode conflict: r%x/a%x vpl_fd %d",
                        component.c_str(), flags, access_mode, fd);
        rv = VSSI_ACCESS;
    }
    else {
        // No conflict, so add the new reference and access modes
        refcount++;

        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "add reference for {%s} count %d: r%x/a%x vpl_fd %d",
                        component.c_str(), refcount, flags, access_mode, fd);
    }

    return rv;
}


s16 vss_file::remove_reference(vss_object *object, u64 origin)
{
    s16 rv = VSSI_SUCCESS;
    std::map<vss_object*,object_reference*>::iterator oref_it;
    object_reference *orp;
    std::map<u64, u32>::iterator or_it;
    bool release_oplocks = false;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "numlocks %d, numoplocks %d, mapsize %d, vpl_fd %d, object %p",
                        numlocks, numoplocks, object_refs.size(), fd, object);

    VPLMutex_Lock(&flock_mutex);

    release_brlocks(origin);

    // Decrement the refcount on the object to vss_file link
    oref_it = object_refs.find(object);
    if(oref_it == object_refs.end()) {
        VPLTRACE_LOG_FINE(TRACE_BVS, 0, "object ref not found for %p on vss_file %p, mapsize %d",
                          object, this, object_refs.size());
    }
    else {
        orp = oref_it->second;
        orp->refcount--;
        if(orp->refcount == 0) {
            object_refs.erase(object);
            delete orp;
            release_oplocks = true;
        }
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "decr refcount for object %p to vss_file %p, mapsize %d",
                            object, this, object_refs.size());
    }

    or_it = origin_flags.find(origin);
    if(or_it != origin_flags.end()) {
        origin_flags.erase(or_it);
    }

    // If this was the last reference from the object, also delete any oplocks
    if(release_oplocks) {
        remove_oplocks(object);
    }

    VPLMutex_Unlock(&flock_mutex);

    // The refcount is protected by the dataset fd_mutex, so we can release the flock_mutex above
    if (refcount == 0) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "no references for {%s}",
                         component.c_str());
        rv = VSSI_INVALID;
    }
    else {
        refcount--;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "%d remaining references for {%s} vpl_fd %d",
                        refcount, component.c_str(), fd);
    return rv;
}

bool vss_file::referenced()
{
    return (refcount > 0);
}

void vss_file::set_delete_pending()
{
    component = "<gone>";
    filename = "<gone>";
    delete_pending = true;
    return;
}

s16 vss_file::add_object(vss_object* object)
{
    s16 rv = VSSI_SUCCESS;
    std::map<vss_object*,object_reference*>::iterator oref_it;
    object_reference *orp;

    VPLMutex_Lock(&flock_mutex);

    oref_it = object_refs.find(object);
    if(oref_it == object_refs.end()) {
        // Allocate a new object reference
        orp = new object_reference;
        orp->object = object;
        orp->refcount = 1;
        // Add to the map
        object_refs[object] = orp;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "added new object ref %p to vss_file %p, mapsize %d",
                            object, this, object_refs.size());
    }
    else {
        orp = oref_it->second;
        orp->refcount++;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "new refcount %d for object ref %p to vss_file %p, mapsize %d",
                            orp->refcount, object, this, object_refs.size());
    }

    VPLMutex_Unlock(&flock_mutex);

    return rv;
}

//
// Remove any active references to the vss_file from an object that is being deleted
//
bool vss_file::remove_object(vss_object* object)
{
    bool removed = false;
    std::map<vss_object*,object_reference*>::iterator oref_it;
    object_reference *orp;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "object %p, numlocks %d, numoplocks %d, mapsize %d",
                        object, numlocks, numoplocks, object_refs.size());

    VPLMutex_Lock(&flock_mutex);

    // Remove any oplocks created by the object (does its own wakeup)
    if(remove_oplocks(object)) {
        removed = true;
    }

    // Remove any byte range locks created by the object
    if(remove_brlocks(object)) {
        removed = true;
    }

    // Decrement the file handle ref count from this object
    oref_it = object_refs.find(object);
    if(oref_it == object_refs.end()) {
        // No reference from that object
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "no object ref from %p to vss_file %p, mapsize %d",
                            object, this, object_refs.size());
    }
    else {
        orp = oref_it->second;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "found refcount %d for object %p to vss_file %p, mapsize %d",
                            orp->refcount, object, this, object_refs.size());
        // This looks sketchy, but the caller already holds the dataset fd_mutex.  Honest.
        refcount -= orp->refcount;
        object_refs.erase(object);
        delete orp;
        removed = true;
    }

    VPLMutex_Unlock(&flock_mutex);

    // Rather than try to be clever, just wake all waiters and let the chips fall where they may
    if(removed) {
        wake_all();
    }

    return removed;
}


void vss_file::add_origin(u64 origin, u32 flags)
{
    std::map<u64, u32>::iterator or_it;

    VPLMutex_Lock(&flock_mutex);

    or_it = origin_flags.find(origin);
    if(or_it == origin_flags.end()) {
        // Add to the map
        origin_flags[origin] = flags;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "added new origin with flags %x to vss_file %p, mapsize %d",
                            flags, this, origin_flags.size());
    }
    else {
        or_it->second = flags;
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "replaced origin ref with flags %x to vss_file %p, mapsize %d",
                            flags, this, origin_flags.size());
    }

    access_mode |= flags;

    VPLMutex_Unlock(&flock_mutex);
}

// ReleaseFile has been called, so release brlocks and open modes
void vss_file::release_origin(u64 origin)
{
    std::map<u64, u32>::iterator or_it;

    VPLMutex_Lock(&flock_mutex);

    release_brlocks(origin);

    or_it = origin_flags.find(origin);
    if(or_it != origin_flags.end()) {
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "releasing origin with flags %u to vss_file %p, mapsize %d",
                            or_it->second, this, origin_flags.size());
        origin_flags.erase(origin);
    }

    // Recalculate overall access mode.  If the ReleaseFile was the last
    // reference, then set to allow and share everything.
    if (origin_flags.size() == 0) {
        access_mode = VSSI_FILE_ACCESS_MODES | VSSI_FILE_SHARING_MODES;
    }
    else {
        access_mode = 0;
        or_it = origin_flags.begin();
        while (or_it != origin_flags.end()) {
            access_mode |= or_it->second;
            or_it++;
        }
    }
    
    VPLMutex_Unlock(&flock_mutex);
}

bool vss_file::validate_origin(u64 origin) 
{
    bool rv = false;
    std::vector<u64>::iterator or_it;

    VPLMutex_Lock(&flock_mutex);

    for (or_it = open_origins.begin(); or_it != open_origins.end(); or_it++) {
        if (*or_it == origin) {
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "origin "FMTu64" exists for vss_file %p", origin, this);
            rv = true;
            break;
        }        
    }

    // Special check here because set_size_iw opens and closes a file within the managed_dataset code. 
    // It has to pass in 0 because the origin is only generated on the client side in VSSI_CloseFile.
    if (origin == 0) {
        rv = true;
    }

    VPLMutex_Unlock(&flock_mutex);

    return rv;
}

void vss_file::add_open_origin(u64 origin) 
{
    VPLMutex_Lock(&flock_mutex);

    open_origins.push_back(origin);
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, "origin "FMTu64" has opened vss_file %p", origin, this);

    VPLMutex_Unlock(&flock_mutex);
}

void vss_file::remove_open_origin(u64 origin) 
{ 
    std::vector<u64>::iterator or_it;

    VPLMutex_Lock(&flock_mutex);

    for (or_it = open_origins.begin(); or_it != open_origins.end(); or_it++) {
        if (*or_it == origin) {
            open_origins.erase(or_it);
            VPLTRACE_LOG_FINE(TRACE_BVS, 0, "origin "FMTu64" has closed vss_file %p", origin, this);
            break;
        }        
    }

    VPLMutex_Unlock(&flock_mutex);
}

dataset* vss_file::get_dataset()
{
    return mydataset;
}

void vss_file::set_file_id(u32 new_file_id)
{
    file_id = new_file_id;
}

u32 vss_file::get_file_id()
{
    return file_id;
}

u32 vss_file::get_access_mode()
{
    return access_mode;
}

bool vss_file::check_lock_conflict(u64 origin, u64 mode, u64 start, u64 end)
{
    bool conflict = false;

    VPLMutex_Lock(&flock_mutex);
    vss_flock* flp = locks;

    while (flp != NULL) {
        if(flp->io_conflicts(origin, mode, start, end)) {
            conflict = true;
            break;
        }
        flp = flp->next;
    }

    VPLMutex_Unlock(&flock_mutex);

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "lock conflict %s for O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64,
                        conflict ? "true":"false", origin, mode, start, end);
    return conflict;
}

s16 vss_file::set_lock_range(vss_object* object,
                             u64 origin,
                             u32 flags,
                             u64 mode,
                             u64 start,
                             u64 end)
{
    s16 rv = VSSI_SUCCESS;

    VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                      "%s lock range for file handle %p: "
                      "origin "FMTx64", range "FMTu64":"FMTu64".",
                      (flags & VSSI_RANGE_LOCK) ? "Add" : "Remove",
                      this, origin, start, end);

    if(end < start) {
        rv = VSSI_INVALID;
        goto done;
    }
    if((flags & VSSI_RANGE_UNLOCK) && (flags & VSSI_RANGE_LOCK)) {
        rv = VSSI_BADCMD;
        goto done;
    }

    if((flags & VSSI_RANGE_UNLOCK)) {
        if(!remove_range_lock(origin, mode, start, end)) {
            rv = VSSI_INVALID;
            goto done;
        }
    } else if((flags & VSSI_RANGE_LOCK)) {
        //
        // If adding a lock, we need to force all oplocks to be
        // released on the file first in order to get correct
        // behavior.  The rule is that byte range locks and
        // oplocks cannot be mixed on the same file.  Setting
        // a lock is considered a write operation, so that it
        // will conflict with any oplock.
        //
        if(check_oplock_conflict(object, true)) {
            rv = VSSI_RETRY;
            goto done;
        }
        //
        // Note that the retry error return is different in the case
        // that the request is blocked by a byte range lock in wait mode,
        // so that the higher layer can wait for the correct thing.
        //
        if(!add_range_lock(object, origin, mode, start, end)) {
            rv = (flags & VSSI_RANGE_WAIT) ? VSSI_WAITLOCK : VSSI_LOCKED;
            goto done;
        }
    } else {
        rv = VSSI_BADCMD;
    }

 done:
    return rv;
}

bool vss_file::add_range_lock(vss_object* object,
                              u64 origin,
                              u64 mode,
                              u64 start,
                              u64 end)
{
    bool success = false;

    VPLMutex_Lock(&flock_mutex);
    vss_flock* flp = locks;

    while (flp != NULL) {
        if(flp->lock_conflicts(origin, mode, start, end)) {
            break;
        }
        flp = flp->next;
    }

    // If no conflict found, add the lock
    // Note that the locks are not maintained in any order.  The search of the list
    // needs to check them all anyway.
    if (flp == NULL) {
        flp = new vss_flock(object, origin, mode, start, end);
        flp->next = locks;
        locks = flp;
        numlocks++;
        success = true;
    }
    VPLMutex_Unlock(&flock_mutex);

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "lock add %s for O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64,
                        success ? "true":"false", origin, mode, start, end);
    return success;
}

bool vss_file::remove_range_lock(u64 origin, u64 mode, u64 start, u64 end)
{
    bool success = false;

    VPLMutex_Lock(&flock_mutex);
    vss_flock* flp = locks;
    vss_flock** prevptr = &locks;

    while (flp != NULL) {
        if(flp->matches(origin, mode, start, end)) {
            *prevptr = flp->next;
            delete flp;
            numlocks--;
            flp = *prevptr;
            success = true;
            continue;
        }
        prevptr = &flp->next;
        flp = flp->next;
    }

    //
    // If the lock was freed, check whether there are any queued requests that
    // should be retried as a result.
    //
    if(success) {
        wake_range_lock(origin, mode, start, end);
    }

    VPLMutex_Unlock(&flock_mutex);

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "lock remove %s for O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64,
                        success ? "ok":"failed", origin, mode, start, end);
    return success;
}

//
// Add event to the byte range lock wait queue for this file
//
bool vss_file::wait_brlock(vss_req_proc_ctx* context,
                           u64 origin,
                           u64 mode,
                           u64 start,
                           u64 end)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                      "Queue request for brlock releases for file handle %p",
                      this);

    VPLMutex_Lock(&flock_mutex);

    brlock_wait *wait_event = new brlock_wait;
    wait_event->context = context;
    wait_event->origin = origin;
    wait_event->mode = mode;
    wait_event->start = start;
    wait_event->end = end;
    brlock_queue.push_back(wait_event);

    VPLMutex_Unlock(&flock_mutex);
    return true;
}

static bool range_overlap(u64 start1, u64 end1, u64 start2, u64 end2)
{
    if((start1 >= end2) || (start2 >= end1)) return false;
    return true;
}

//
// Wake up any queued range lock requests freed by a particular lock release
//
bool vss_file::wake_range_lock(u64 origin,
                               u64 mode,
                               u64 start,
                               u64 end)
{
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Wake up brlock requests after freeing O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64
                        " for file handle %p",
                        origin, mode, start, end, this);
    ///////////////////////////////////////////////////////////////////
    // NOTE: this must only be called with the flock_mutex already held
    ///////////////////////////////////////////////////////////////////

    if (!VPLMutex_LockedSelf(&flock_mutex)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Current thread does not hold flock_mutex in file %p", this);
        FAILED_ASSERT("wake_range_lock does not hold flock_mutex");
    }

    //
    // Examine each queued request in turn.  If it conflicts with the freed lock, retry it.
    //
    brlock_wait *waiter;
    u32 num_entries = brlock_queue.size();

    while(num_entries > 0) {
        waiter = brlock_queue.front();
        brlock_queue.pop_front();
        // If waiter conflicts with the lock that was removed, retry it
        if(range_overlap(start, end, waiter->start, waiter->end)) {
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "requeuing %p", waiter->context);
            mydataset->server->requeueRequest(waiter->context);
            delete waiter;
        }
        else {
            brlock_queue.push_back(waiter);
        }
        num_entries--;
    }

    return true;
}

//
// Release any byte range locks for a particular user descriptor (origin)
//
bool vss_file::release_brlocks(u64 origin)
{
    bool removed = false;

    ///////////////////////////////////////////////////////////////////
    // NOTE: this must only be called with the flock_mutex already held
    ///////////////////////////////////////////////////////////////////

    if (!VPLMutex_LockedSelf(&flock_mutex)) {           
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Current thread does not hold flock_mutex in file %p", this);
        FAILED_ASSERT("release_brlocks does not hold flock_mutex");
    }

    vss_flock* flp = locks;
    vss_flock** prevptr = &locks;

    // Remove any file locks created by this originator
    while(flp != NULL) {
        if(flp->matches_origin(origin)) {
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "lock removed: O"FMTx64"/M"FMTx64"/S"FMTu64"/E"FMTu64,
                                origin, flp->mode, flp->start, flp->end);
            // Wake any waiters for this lock
            wake_range_lock(flp->origin, flp->mode, flp->start, flp->end);
            *prevptr = flp->next;
            delete flp;
            numlocks--;
            flp = *prevptr;
            removed = true;
            continue;
        }
        prevptr = &flp->next;
        flp = flp->next;
    }

    return removed;
}

//
// Remove any byte range locks created by an object
//
bool vss_file::remove_brlocks(vss_object* object)
{
    bool removed = false;
    vss_flock* flp = locks;
    vss_flock** prevptr = &locks;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Remove any remaining brlocks (cnt %d) for object %p from file %p",
                        numlocks, object, this);

    ///////////////////////////////////////////////////////////////////
    // NOTE: this must only be called with the flock_mutex already held
    ///////////////////////////////////////////////////////////////////

    if (!VPLMutex_LockedSelf(&flock_mutex)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Current thread does not hold flock_mutex in file %p", this);
        FAILED_ASSERT("remove_brlocks does not hold flock_mutex");
    }

    while (flp != NULL) {
        if(flp->matches_object(object)) {
            *prevptr = flp->next;
            delete flp;
            numlocks--;
            flp = *prevptr;
            removed = true;
            continue;
        }
        prevptr = &flp->next;
        flp = flp->next;
    }

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "numlocks %d", numlocks);

    return removed;
}

//
// Remove any oplocks created by an object
//
bool vss_file::remove_oplocks(vss_object* object)
{
    bool removed = false;
    vss_oplock* olp;
    vss_oplock** prevptr;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Remove any remaining oplocks (cnt %d) for object %p from file %p",
                        numoplocks, object, this);
    ///////////////////////////////////////////////////////////////////
    // NOTE: this must only be called with the flock_mutex already held
    ///////////////////////////////////////////////////////////////////

    if (!VPLMutex_LockedSelf(&flock_mutex)) {
        VPLTRACE_LOG_ERR(TRACE_BVS, 0, "Current thread does not hold flock_mutex in file %p", this);
        FAILED_ASSERT("remove_oplocks does not hold flock_mutex");
    }
    
    olp = oplocks;
    prevptr = &oplocks;

    //
    // For a particular file handle, there should be only one oplock for
    // a given originating object, but the code traverses the whole list.
    //
    while (olp != NULL) {
        if(olp->matches(object)) {
            *prevptr = olp->next;
            delete olp;
            numoplocks--;
            olp = *prevptr;
            removed = true;
            continue;
        }
        prevptr = &olp->next;
        olp = olp->next;
    }
    // If any oplocks released, check for waiters that can be started
    if(removed) {
        // TODO: make this more selective, but start them all for now
        while(!oplock_queue.empty()) {
            struct oplock_wait* waiter = oplock_queue.front();
            oplock_queue.pop_front();
            VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                      "requeued %p", waiter->context);
            mydataset->server->requeueRequest(waiter->context);
            delete waiter;
        }
    }
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "numoplocks %d", numoplocks);

    return removed;
}

//
// Set oplock state on the file to control the ownership of cache state
//
s16 vss_file::set_oplock(vss_object* object,
                         u64 mode)
{
    s16 rv = VSSI_SUCCESS;
    bool success = false;
    bool conflict = false;
    vss_oplock* olp;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Set cache lock state for file handle %p: "
                        "object %p, mode "FMTx64".",
                        this, object, mode);

    if(mode & ~(VSSI_FILE_LOCK_CACHE_READ|VSSI_FILE_LOCK_CACHE_WRITE)) {
        rv = VSSI_INVALID;
        goto done;
    }

    //
    // Code for releasing oplocks is shared with the close logic
    //
    if(mode == VSSI_FILE_LOCK_NONE) {
        VPLMutex_Lock(&flock_mutex);
        success = remove_oplocks(object);
        VPLMutex_Unlock(&flock_mutex);
        // Don't return error if no locks found
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "oplock remove %s",
                            success ? "succeeded":"skipped (no locks found)");
        goto done;
    }

    VPLMutex_Lock(&flock_mutex);
    //
    // The current rules of engagement are that clients may not cache files
    // that have any byte range locks.  It is too complicated to make that
    // combination work correctly, so it is prevented from happening at all.
    //
    if(locks != NULL) {
        conflict = true;
        goto unlock;
    }
    
    //
    // Setting a new oplock requires checking whether the request conflicts
    // with any existing oplocks.  If so, the request is rejected synchronously.
    //
    olp = oplocks;
    while (olp != NULL) {
        // Check for conflict
        if(olp->need_to_break(this, object, ((mode & VSSI_FILE_LOCK_CACHE_WRITE) != 0), false)) {
            conflict = true;
            break;
        }
        olp = olp->next;
    }
    //
    // If no conflict found, add the lock, replacing an existing lock for the
    // originating object. Note that the locks are not maintained in any order.
    // The search of the list needs to check them all anyway.
    //
    if (!conflict) {
        // Replace an existing oplock from this object if there is one, so that
        // the new mode overrides.
        olp = oplocks;
        while (olp != NULL) {
            if (olp->matches(object)) {
                olp->set_mode(mode);
                break;
            }
            olp = olp->next;
        }
        // No pre-existing oplock, so add a new one
        if (olp == NULL) {
            olp = new vss_oplock(object, mode);
            olp->next = oplocks;
            oplocks = olp;
            numoplocks++;
        }
    }
unlock:
    VPLMutex_Unlock(&flock_mutex);

    if(conflict) {
        rv = VSSI_LOCKED;
    }
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "oplock add %s",
                        conflict ? "rejected":"done");
 done:
    return rv;
}

//
// Return the current oplock state on the file for a particular object
//
s16 vss_file::get_oplock(vss_object* object,
                         u64& mode)
{
    s16 rv = VSSI_SUCCESS;
    bool found = false;
    vss_oplock* olp;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Get cache lock state for file handle %p: "
                        "object %p.",
                        this, object);
    
    mode = 0ULL;
    VPLMutex_Lock(&flock_mutex);
    olp = oplocks;
    while (olp != NULL) {
        // Check for conflict
        if(olp->matches(object)) {
            found = true;
            olp->get_mode(mode);
            // Indicate whether anyone is waiting
            if(oplock_queue.size() != 0) {
                mode |= VSSI_FILE_LOCK_CACHE_BREAK;
            }
            break;
        }
        olp = olp->next;
    }
    VPLMutex_Unlock(&flock_mutex);

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "oplock %sfound: "FMTx64".",
                        found ? "":"not ", mode);
    return rv;
}

//
// Check whether an IO request conflicts with oplocks.
// In the process, oplock break notifications are generated.
//
// If they were, then the triggering IO request needs to
// wait for the clients to respond by clearing the conflicting
// locks (after invalidating or flushing their caches as appropriate).
//
// The queuing will happen several layers up, if this method returns true.
//
bool vss_file::check_oplock_conflict(vss_object* object,
                                     bool write_request)
{
    bool notify_done = false;
    vss_oplock* olp;

    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Check %s request against oplocks for file handle %p, "
                        "object %p.",
                        write_request ? "write" : "read", this, object);

    VPLMutex_Lock(&flock_mutex);

    // Traverse the list of oplocks and generate notifications as required
    olp = oplocks;
    while (olp != NULL) {
        if(olp->need_to_break(this, object, write_request, true)) {
            notify_done = true;
        }
        olp = olp->next;
    }

    VPLMutex_Unlock(&flock_mutex);
    return notify_done;
}

bool vss_file::sync_attrs(u32& ret_attrs)
{
    bool changed;

    changed = (attrs != attrs_orig);
    ret_attrs = attrs_orig = attrs;

    return changed;
}

bool vss_file::sync_size(u64& ret_size)
{
    bool changed;

    changed = (size != size_orig);
    ret_size = size_orig = size;

    return changed;
}

//
// Add event to the oplock wait queue for this file
//
bool vss_file::wait_oplock(vss_req_proc_ctx* context,
                           vss_object* object,
                           bool write_request)
{
    VPLTRACE_LOG_FINE(TRACE_BVS, 0, 
                      "Queue %s request to wait for oplock releases for file handle %p, "
                      "object %p.",
                      write_request ? "write" : "read", this, object);

    VPLMutex_Lock(&flock_mutex);

    oplock_wait *wait_event = new oplock_wait;
    wait_event->context = context;
    wait_event->object = object;
    wait_event->write_request = write_request;
    oplock_queue.push_back(wait_event);

    VPLMutex_Unlock(&flock_mutex);
    return true;
}

//
// Sledgehammer to use when a vss_object that had a reference to the given vss_file is
// being destroyed, wake all waiters and let them fail, succeed or requeue as appropriate.
//
void vss_file::wake_all()
{
    VPLTRACE_LOG_FINEST(TRACE_BVS, 0, 
                        "Wake up all waiters for file handle %p (br %d, op %d)", 
                        this, brlock_queue.size(), oplock_queue.size());

    VPLMutex_Lock(&flock_mutex);

    brlock_wait *waiter;
    u32 num_entries = brlock_queue.size();

    while(num_entries > 0) {
        waiter = brlock_queue.front();
        brlock_queue.pop_front();
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "requeuing %p", waiter->context);
        mydataset->server->requeueRequest(waiter->context);
        delete waiter;
        num_entries--;
    }

    num_entries = oplock_queue.size();
    oplock_wait *opwaiter;

    while(num_entries > 0) {
        opwaiter = oplock_queue.front();
        oplock_queue.pop_front();
        VPLTRACE_LOG_FINEST(TRACE_BVS, 0, "requeuing %p", opwaiter->context);
        mydataset->server->requeueRequest(opwaiter->context);
        delete opwaiter;
        num_entries--;
    }

    VPLMutex_Unlock(&flock_mutex);
}

void vss_file::set_size(u64 new_size)
{
    size = new_size;
    is_modified = true;
    new_modify_time = true;
}

void vss_file::set_size_if_larger(u64 new_size)
{
    if ( new_size > size ) {
        size = new_size;
    }
    is_modified = true;
    new_modify_time = true;
}

void vss_file::set_attrs(u32 new_attrs)
{
    attrs = new_attrs;
    is_modified = true;
}
