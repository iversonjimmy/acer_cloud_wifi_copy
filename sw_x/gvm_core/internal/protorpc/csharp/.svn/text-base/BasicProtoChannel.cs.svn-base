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
using System.IO;
using rpc;

namespace protorpc
{
    public class BasicProtoChannel : ProtoChannel
    {
        protected MemoryStream inStream;
        protected MemoryStream outStream;

        public BasicProtoChannel(MemoryStream inStream, MemoryStream outStream)
        {
            this.inStream = inStream;
            this.outStream = outStream;
        }
        
        /**
         * @see igware.protobuf.ProtoChannel#writeMessage(com.google.protobuf.MessageLite)
         */
        public override void writeMessage(object message)
        {
            try {
                ProtoBuf.Serializer.NonGeneric.SerializeWithLengthPrefix(outStream, message, ProtoBuf.PrefixStyle.Base128, 0);
            } catch (IOException e) {
                throw new RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            }
        }

        /**
         * @see igware.protobuf.ProtoChannel#extractMessage(com.google.protobuf.MessageLite.Builder)
         */
        public override void extractMessage(Type type, ref object message)
        {
            try {
                int msgLen = 0;
                bool success = ProtoBuf.Serializer.TryReadLengthPrefix(inStream, ProtoBuf.PrefixStyle.Base128, out msgLen);
                if (success == false) {
                    throw new RpcLayerException(RpcStatus.Status.IO_ERROR, "Failed to read length prefix: " + msgLen);
                }
                byte[] buffer = inStream.GetBuffer();
                MemoryStream tempInStream = new MemoryStream(buffer, (int)inStream.Position, msgLen);
                inStream.Position += msgLen;
                message = ProtoBuf.Serializer.NonGeneric.Deserialize(type, tempInStream);
            } catch (IOException e) {
                throw new RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            }
        }
        
        /**
         * @see igware.protobuf.ProtoChannel#flushOutputStream()
         */
        public override bool flushOutputStream()
        {
            try {
                outStream.Flush();
            } catch {//(IOException e) {
                // TODO LOG
                return false;
            }
            return true;
        }
    }
}
