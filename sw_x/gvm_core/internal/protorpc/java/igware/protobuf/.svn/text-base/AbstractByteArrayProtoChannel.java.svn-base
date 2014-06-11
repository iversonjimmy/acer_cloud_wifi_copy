/*
 * Copyright (C) 2011, BroadOn Communications Corp.
 *
 * These coded instructions, statements, and computer programs
 * contain unpublished proprietary information of BroadOn
 * Communications Corp., and are protected by Federal copyright
 * law. They may not be disclosed to third parties or copied or
 * duplicated in any form, in whole or in part, without the prior
 * written consent of BroadOn Communications Corp.
 */
package igware.protobuf;

import igware.protobuf.pb.Rpc.RpcStatus.Status;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

import com.google.protobuf.MessageLite;

public abstract class AbstractByteArrayProtoChannel implements ProtoChannel {
    
    protected ByteArrayOutputStream mRequest = new ByteArrayOutputStream(512);
    protected ByteArrayInputStream mResponse = null;
    
    /**
     * @see igware.protobuf.ProtoChannel#writeMessage(com.google.protobuf.MessageLite)
     */
    public void writeMessage(MessageLite message) throws RpcLayerException {

        try {
            message.writeDelimitedTo(mRequest);
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        }
    }
    
    /**
     * @see igware.protobuf.ProtoChannel#extractMessage(com.google.protobuf.MessageLite.Builder)
     */
    public void extractMessage(MessageLite.Builder messageBuilder)
            throws RpcLayerException {

        if (mResponse == null) {
            throw new RpcLayerException(Status.IO_ERROR,
                    "No response received, make sure to call flushOutputStream()");
        }
        try {
            messageBuilder.mergeDelimitedFrom(mResponse);
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        }
    }
    
    /**
     * @see igware.protobuf.ProtoChannel#flushOutputStream()
     */
    @Override
    public boolean flushOutputStream() throws Exception {

        byte[] requestBuf = mRequest.toByteArray();
        mRequest.reset();
        mResponse = new ByteArrayInputStream(perform(requestBuf));
        return true;
    }
    
    protected abstract byte[] perform(byte[] requestBuf) throws Exception;
}
