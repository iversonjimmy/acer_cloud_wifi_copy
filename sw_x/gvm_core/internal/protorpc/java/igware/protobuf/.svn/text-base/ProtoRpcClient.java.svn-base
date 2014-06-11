package igware.protobuf;

import java.io.IOException;

import igware.protobuf.pb.Rpc.RpcRequestHeader;
import igware.protobuf.pb.Rpc.RpcStatus;
import igware.protobuf.pb.Rpc.RpcStatus.Status;

import com.google.protobuf.MessageLite;
import com.google.protobuf.UninitializedMessageException;

public class ProtoRpcClient {
    
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
    public ProtoRpcClient(ProtoChannel channel, boolean throwOnAppError)
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
    protected int sendRpc(String methodName, MessageLite request, MessageLite.Builder responseBuilder)
    throws ProtoRpcException
    {
        sendRpcRequest(methodName, request);
        return recvRpcResponse(responseBuilder);
    }
    
    // sendRpc() split into two halves, for async support:
    
    protected void sendRpcRequest(String methodName, MessageLite request)
    throws RpcLayerException
    {
        try {
            // Send request header
            RpcRequestHeader reqHdr = RpcRequestHeader.newBuilder().
                    setMethodName(methodName).
                    build();
            _channel.writeMessage(reqHdr);
            
            // Send request body
            _channel.writeMessage(request);
            
            // Flush stream
            _channel.flushOutputStream();
            
        } catch (RpcLayerException e) {
            throw e;
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        } catch (UninitializedMessageException e) {
            throw new RpcLayerException(Status.BAD_REQUEST, e.getMessage());
        } catch (Exception e) {
            throw new RpcLayerException(Status.INTERNAL_ERROR, e);
        }
    }
    
    protected int recvRpcResponse(MessageLite.Builder responseBuilder)
    throws ProtoRpcException
    {
        try {
            RpcStatus.Builder statusBuilder = RpcStatus.newBuilder();
            _channel.extractMessage(statusBuilder);
            RpcStatus status = statusBuilder.build();
            if (status.getStatus() != Status.OK) {
                throw new RpcLayerException(status.getStatus(), status.getErrorDetail());
            }
            if (status.getAppStatus() >= 0) {
                _channel.extractMessage(responseBuilder);
            } else if (_throwOnAppError) {
                throw new AppLayerException(status.getAppStatus());
            }
            return status.getAppStatus();
            
        } catch (ProtoRpcException e) {
            throw e;
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        } catch (UninitializedMessageException e) {
            throw new RpcLayerException(Status.BAD_RESPONSE, e.getMessage());
        } catch (Exception e) {
            throw new RpcLayerException(Status.INTERNAL_ERROR, e);
        }
    }

    private ProtoChannel _channel;
    private final boolean _throwOnAppError;
}
