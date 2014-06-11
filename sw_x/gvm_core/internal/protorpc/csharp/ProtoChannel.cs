//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//
using System;

namespace protorpc
{
    abstract public class ProtoChannel
    {
        /**
         * Serializes and writes the message to the channel.
         */
        abstract public void writeMessage(object message);
        
        /**
         * Reads and deserializes the message from the channel.
         */
        abstract public void extractMessage(Type type, ref object message);
        
        /** Flushes the output stream. */
        abstract public bool flushOutputStream();
    }
}
