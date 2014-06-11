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
    public class ProtoRpcClient
    {
        private ProtoChannel _channel;
        private bool _throwOnAppError;

        /**
         * Allows communicating with a ProtoRPC-based service over a {@link ProtoChannel}.
         * @param channel Provide an implementation of {@link ProtoChannel} that connects
         *          to the target service.
         * @param throwOnAppError If <code>true</code>, then
         *          {@link #sendRpc(String, MessageLite, com.google.protobuf.MessageLite.Builder)}
         *          and {@link #recvRpcResponse(com.google.protobuf.MessageLite.Builder)}
         *          will throw {@link AppLayerException} instead of returning a negative
         *          app-layer error code (since the response will be empty in this case anyway).
         */
        public ProtoRpcClient(ProtoChannel channel, bool throwOnAppError)
        {
            _channel = channel;
            _throwOnAppError = throwOnAppError;
        }
        
        /**
         * Sends an RPC over the channel.  The RPC is dispatched to the named
         * method.  Upon return, a valid status will be filled in, and the
         * response may or may not be filled in.  The caller should check the
         * status to see if the call was successful.  The response will only be
         * filled in when (RpcStatus.getStatus() == Status.OK and RpcStatus.getAppStatus() >= 0).
         * @throws ProtoRpcException 
         */
        protected int sendRpc(string methodName, object request, Type type, ref object response)
        {
            sendRpcRequest(methodName, request);
            return recvRpcResponse(type, ref response);
        }
        
        // sendRpc() split into two halves, for async support:
        
        protected void sendRpcRequest(string methodName, object request)
        {
            try {
                // Send request header
                RpcRequestHeader reqHdr = new RpcRequestHeader();
                reqHdr.methodName = methodName;
                _channel.writeMessage(reqHdr);
                
                // Send request body
                _channel.writeMessage(request);
                
                // Flush stream
                _channel.flushOutputStream();
                
            } catch (RpcLayerException e) {
                throw e;
            } catch (IOException e) {
                throw new RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            } catch (ProtoBuf.ProtoException e) {
                throw new RpcLayerException(RpcStatus.Status.BAD_REQUEST, e.Message);
            } catch (Exception e) {
                throw new RpcLayerException(RpcStatus.Status.INTERNAL_ERROR, e.Message);
            }
        }

        protected int recvRpcResponse(Type type, ref object response)
        {
            try {
                RpcStatus status = new RpcStatus();
                object reference = status;
                _channel.extractMessage(status.GetType(), ref reference);
                status = (RpcStatus)reference;
                if (status.status != RpcStatus.Status.OK)
                {
                    throw new RpcLayerException(status.status, status.errorDetail);
                }
                if (status.appStatus >= 0) {
                    _channel.extractMessage(type, ref response);
                } else if (_throwOnAppError) {
                    throw new AppLayerException(status.appStatus);
                }
                return status.appStatus;
                
            } catch (ProtoRpcException e) {
                throw e;
            } catch (IOException e) {
                throw new RpcLayerException(RpcStatus.Status.IO_ERROR, e.Message);
            } catch (ProtoBuf.ProtoException e) {
                throw new RpcLayerException(RpcStatus.Status.BAD_RESPONSE, e.Message);
            } catch (Exception e) {
                throw new RpcLayerException(RpcStatus.Status.INTERNAL_ERROR, e.Message);
            }
        }
    }
}
