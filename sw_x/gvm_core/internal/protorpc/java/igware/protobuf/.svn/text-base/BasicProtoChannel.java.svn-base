package igware.protobuf;

import igware.protobuf.pb.Rpc.RpcStatus.Status;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import com.google.protobuf.MessageLite;

public class BasicProtoChannel implements ProtoChannel {
    
    private InputStream inStream;
    private OutputStream outStream;

    public BasicProtoChannel(InputStream inStream, OutputStream outStream)
    {
        this.inStream = inStream;
        this.outStream = outStream;
    }
    
    /**
     * @see igware.protobuf.ProtoChannel#writeMessage(com.google.protobuf.MessageLite)
     */
    public void writeMessage(MessageLite message) throws RpcLayerException
    {
        try {
            message.writeDelimitedTo(outStream);
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        }
    }

    /**
     * @see igware.protobuf.ProtoChannel#extractMessage(com.google.protobuf.MessageLite.Builder)
     */
    public void extractMessage(MessageLite.Builder messageBuilder) throws RpcLayerException
    {
        try {
            messageBuilder.mergeDelimitedFrom(inStream);
        } catch (IOException e) {
            throw new RpcLayerException(Status.IO_ERROR, e);
        }
    }
    
    /**
     * @see igware.protobuf.ProtoChannel#flushOutputStream()
     */
    public boolean flushOutputStream()
    {
        try {
            outStream.flush();
        } catch (IOException e) {
            // TODO LOG
            return false;
        }
        return true;
    }
}
