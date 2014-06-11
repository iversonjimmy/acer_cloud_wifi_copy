package com.igware.vpl.android;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import org.apache.http.entity.AbstractHttpEntity;

import android.util.Log;

public class InputStreamEntityWithProgress extends AbstractHttpEntity
{
    private final static int BUFFER_SIZE = 4096;
    
    private final InputStream content;
    private final long length;
    private boolean consumed = false;
    
    private long transferredBytes;
    private HttpManager2 httpManager;
    private long http2ImplPtr;
    private static final String logTag = "InputStreamEntityWithProgress";

    public InputStreamEntityWithProgress(final InputStream instream, long length, long http2ImplPtr) {
        super();
        if (instream == null) {
            throw new IllegalArgumentException("Source input stream may not be null");
        }
        this.content = instream;
        this.length = length;
        this.http2ImplPtr = http2ImplPtr;
    }

    @Override
    public boolean isRepeatable() {
        return false;
    }

    @Override
    public long getContentLength() {
        return this.length;
    }

    @Override
    public InputStream getContent() {
        return this.content;
    }
    
    @Override
    public void writeTo(final OutputStream outstream) throws IOException {
        if (outstream == null) {
            throw new IllegalArgumentException("Output stream may not be null");
        }
        InputStream instream = this.content;
        byte[] buffer = new byte[BUFFER_SIZE];
        int l;
        if (this.length < 0) {
            Log.w(logTag, "Unsupported: Consume until EOF");
            while ((l = instream.read(buffer)) != -1) {
                outstream.write(buffer, 0, l);
                transferredBytes += l;
                Log.d(logTag, "Call progress callback, length=" + length + ", transferredBytes=" + transferredBytes);
                int cbReturn = httpManager.callCProgressCallback(http2ImplPtr, 0, 0, length, transferredBytes);
                if (cbReturn < 0) {
                    Log.d(logTag, "Progress callback returned " + cbReturn + "; aborting");
                    break;
                }
            }
        } else {
            // consume no more than length
            long remaining = this.length;
            Log.d(logTag, "Will read " + remaining);
            while (remaining > 0) {
                l = instream.read(buffer, 0, (int)Math.min(BUFFER_SIZE, remaining));
                if (l == -1) {
                    break;
                }
                outstream.write(buffer, 0, l);
                remaining -= l;
                transferredBytes += l;
                Log.d(logTag, "Call progress callback, length=" + length + ", transferredBytes=" + transferredBytes);
                int cbReturn = httpManager.callCProgressCallback(http2ImplPtr, 0, 0, length, transferredBytes);
                if (cbReturn < 0) {
                    Log.d(logTag, "Progress callback returned " + cbReturn + "; aborting");
                    break;
                }
            }
        }
        this.consumed = true;
    }

    @Override
    public boolean isStreaming() {
        return !this.consumed;
    }

    @Override
    public void consumeContent() throws IOException {
        this.consumed = true;
        // If the input stream is from a connection, closing it will read to
        // the end of the content. Otherwise, we don't care what it does.
        this.content.close();
    }

}
