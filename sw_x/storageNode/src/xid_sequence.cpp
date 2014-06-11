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

#include <iostream>

#include <string.h> // memset

#include "xid_sequence.hpp"

using namespace std;

xid_sequence::xid_sequence() :
    last_received(0),
    fresh_state(true)
{}

xid_sequence::~xid_sequence()
{}


bool xid_sequence::is_fresh()
{
    return fresh_state;
}

u32 xid_sequence::next_valid()
{
    u32 rv = 0;
    // Next acceptable is 0 if fresh, last_received + 1 otherwise.
    if(!fresh_state) {
        rv = last_received + 1;
    }

    return rv;
}

bool xid_sequence::is_valid(u32 seqval)
{
    bool rv = false;
    u32 increase;
    u32 decrease;

    // Always accept the first XID.
    if(fresh_state) {
        return true;
    }

    increase = seqval - last_received;
    decrease = last_received - seqval;
    if(increase == 0) {
        cout << "Repeat seqval " << seqval << " seen. Last received:"
             << last_received << endl;
            // No good. Matches a received value.        
    }
    else if(increase <= MAX_JUMP) {
        // OK to accept forward seqval value within jump range.
        rv = true;
    }
    else if(decrease <= WINDOW_SIZE) {
        u32 test_bit = (seqval & (WINDOW_SIZE-1));
        if((window[test_bit >> 5] &
            (1 << (test_bit & 31))) == 0) {
            // OK to accept past seqval not previously seen within window.
            rv = true;
        }
    } 
    else {
        // else, seqval matches past value or is out of range.
        cout << "Seqval "<< seqval << " is a duplicate or out-of-range." << 
            endl;
    }

    return rv;

}

void xid_sequence::accept(u32 seqval)
{
    u32 increase;
    u32 decrease;
    u32 i = 0;

    if(fresh_state) {
        last_received = seqval;
        fresh_state = false;
    }
    
    increase = seqval - last_received;
    decrease = last_received - seqval;

    if(increase == 0) {
        // It's a duplicate value. No change.
    }
    else if(increase <= MAX_JUMP) {
        if(increase < WINDOW_SIZE) {
            // Shift window bits by N(=increase) so the most recent ones are 0.

            // Set current "0" bit, to represent last_received.
            window[(last_received & (WINDOW_SIZE-1)) >> 5] |=
                (1 << (last_received & 31));

            // Clear the most-recent N bits.
            for(i = 0; i < increase; i++) {
                u32 clear_bit = ((seqval - i) & (WINDOW_SIZE-1));
                window[clear_bit >> 5] &= ~(1 << (clear_bit & 31));
            }
        }
        else {
            // Wiped out the history window with a long jump.
            memset(&(window), 0, sizeof(window));
        }

        last_received = seqval;
    }
    else if(decrease <= WINDOW_SIZE) {
        u32 test_bit = (seqval % WINDOW_SIZE);

        if(window[test_bit >> 5] & (1 << (test_bit & 31))) {
            // It's a duplicate. No change.
        }
        else {
            // Set the appropriate window bit true.
            window[test_bit >> 5] |= (1 << (test_bit & 31));
        }
    }
    // else seqval is out of update range.
}

void xid_sequence::reset()
{
    last_received = 0;
    memset(&(window), 0, sizeof(window));
    fresh_state = true;
}

