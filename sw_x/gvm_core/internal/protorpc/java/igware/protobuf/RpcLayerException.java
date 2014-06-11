package igware.protobuf;

import igware.protobuf.pb.Rpc.RpcStatus.Status;

public class RpcLayerException extends ProtoRpcException {

    private static final long serialVersionUID = -3264267950883345984L;
    private final Status status;

    public RpcLayerException(Status status, String message)
    {
        super("Status = " + status.toString() + ": " + message);
        this.status = status;
    }

    public RpcLayerException(Status status, Throwable t)
    {
        super("Status = " + status.toString(), t);
        this.status = status;
    }
    
    public Status getStatus()
    {
        return status;
    }
}
