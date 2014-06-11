//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

using System;
using System.IO;
using rpc;
using csharp_ccd_named_pipe_interop;

namespace ccd
{
    public class CCDProtoChannel : protorpc.ProtoChannel
    {
        CCDNamedPipeInterop namedPipeWrapper;
        MemoryStream response;

        public CCDProtoChannel()
        {
            namedPipeWrapper = null;
            response = null;
        }

        /**
         * @see igware.protobuf.ProtoChannel#writeMessage(com.google.protobuf.MessageLite)
         */
        public override void writeMessage(object message)
        {
            MemoryStream outStream = new MemoryStream(4096);
            try
            {
                ProtoBuf.Serializer.NonGeneric.SerializeWithLengthPrefix(outStream, message, ProtoBuf.PrefixStyle.Base128, 0);
            }
            catch (IOException e)
            {
                throw new protorpc.RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            }
            byte[] buffer = outStream.ToArray();
            if (namedPipeWrapper == null)
            {
                namedPipeWrapper = new CCDNamedPipeInterop(); // Also opens the Named Pipe.
                response = null; // Reset the response in case we are reusing this ProtoChannel for multiple RPCs.
            }
            namedPipeWrapper.writeBufferToNamedPipe(buffer);
        }

        /**
         * @see igware.protobuf.ProtoChannel#extractMessage(com.google.protobuf.MessageLite.Builder)
         */
        public override void extractMessage(Type type, ref object message)
        {
            if (response == null)
            {
                response = new MemoryStream(4096);
                namedPipeWrapper.readBufferFromNamedPipe(response);
                namedPipeWrapper = null; // Now that we've read the entire response, reset the namedPipeWrapper.
                response.Position = 0; // Done writing to it.  Next, we will read from the beginning.
            }
            try
            { // TODO: we really want this to just be TryDeserializeWithLengthPrefix, but Pedela reported problems when he tried to use it.
                int msgLen = 0;
                bool success = ProtoBuf.Serializer.TryReadLengthPrefix(response, ProtoBuf.PrefixStyle.Base128, out msgLen);
                if (success == false)
                {
                    throw new protorpc.RpcLayerException(RpcStatus.Status.IO_ERROR, "Failed to read length prefix.");
                }
                // Deserialize reads the whole stream.  Since 'response' can contain multiple protobufs, we need to
                // separate out a copy of just the next protobuf.
                MemoryStream tempInStream = new MemoryStream(response.GetBuffer(), (int)response.Position, msgLen);
                response.Position += msgLen;
                message = ProtoBuf.Serializer.NonGeneric.Deserialize(type, tempInStream);
            }
            catch (IOException e)
            {
                throw new protorpc.RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            }
        }

        /**
         * @see igware.protobuf.ProtoChannel#flushOutputStream()
         */
        public override bool flushOutputStream()
        {
            return true;
        }
    }
}
