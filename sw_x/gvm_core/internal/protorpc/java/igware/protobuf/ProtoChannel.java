package igware.protobuf;

import com.google.protobuf.MessageLite;

public interface ProtoChannel {
    
    /**
     * Serializes and writes the message to the channel.
     */
    public void writeMessage(MessageLite message) throws Exception;
    
    /**
     * Reads and deserializes the message from the channel.
     */
    public void extractMessage(MessageLite.Builder messageBuilder) throws Exception;
    
    /** Flushes the output stream. */
    public boolean flushOutputStream() throws Exception;
}
