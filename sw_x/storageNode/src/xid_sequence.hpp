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

#ifndef STORAGE_NODE__XID_SEQUENCE_HPP__
#define STORAGE_NODE__XID_SEQUENCE_HPP__

#include <vplu_types.h>
#include <vplu_common.h>

/// Object that verifies sequence numbers received from a client.
/// A sequence number is considered good if it is within the current
/// acceptance window and has not been seen before.
/// The window may only advance forward (wrapping around) within the range
/// of sequence numbers. The window may jump forward, but only by a limited
/// degree.
/// The window size and max jump size are fixed.
class xid_sequence {
private:
    // TODO: we really shouldn't need to make copies of this
    //VPL_DISABLE_COPY_AND_ASSIGN(xid_sequence);

    static const u32 WINDOW_SIZE  = 512;
    static const u32 WINDOW_WORDS = 16; // WINDOWS_SIZE / 32;
    static const u32 MAX_JUMP     = 4096; // arbitrary

    /// bitset for sequence window
    u32 window[WINDOW_WORDS];

    /// Current offset of sequence window
    u32 last_received;

    /// Flag indicating if no XIDs have yet been accepted.
    bool fresh_state;

public:
    xid_sequence();
    ~xid_sequence();

    // Return if the sequence state is fresh (new or reset)
    bool is_fresh();

    /// Return next acceptable in-sequence XID
    u32 next_valid();

    /// Determine if a sequence value is currently valid.
    bool is_valid(u32 seqval);

    /// Accept a sequence value. 
    void accept(u32 seqval);

    /// Reset sequence value to initial state.
    void reset();
};

#endif //include guard
