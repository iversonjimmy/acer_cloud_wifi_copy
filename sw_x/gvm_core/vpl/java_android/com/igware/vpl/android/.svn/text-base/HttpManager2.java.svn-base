package com.igware.vpl.android;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.atomic.AtomicBoolean;

import javax.net.ssl.SSLException;
import javax.net.ssl.SSLPeerUnverifiedException;

import org.apache.http.Header;
import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.client.ResponseHandler;
import org.apache.http.client.methods.HttpDelete;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.client.methods.HttpPut;
import org.apache.http.client.methods.HttpUriRequest;
import org.apache.http.conn.ClientConnectionManager;
import org.apache.http.conn.HttpHostConnectException;
import org.apache.http.conn.scheme.PlainSocketFactory;
import org.apache.http.conn.scheme.Scheme;
import org.apache.http.conn.scheme.SchemeRegistry;
import org.apache.http.conn.ssl.SSLSocketFactory;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.impl.conn.tsccm.ThreadSafeClientConnManager;
import org.apache.http.params.HttpConnectionParams;
import org.apache.http.params.HttpParams;
import org.apache.http.protocol.HTTP;

import android.util.Log;

public class HttpManager2
{
    protected static final String logTag = "HttpManager2";

    // MyHttpClient is the same as DefaultHttpClient,
    // except that it uses ThreadSafeClientConnManager.
    public static class MyHttpClient extends DefaultHttpClient
    {
        public MyHttpClient()
        {
        }

        @Override
        protected ClientConnectionManager createClientConnectionManager()
        {
            SchemeRegistry registry = new SchemeRegistry();
            registry.register(new Scheme("http", PlainSocketFactory.getSocketFactory(), 80));
            registry.register(new Scheme("https", SSLSocketFactory.getSocketFactory(), 443));
            ThreadSafeClientConnManager cm = new ThreadSafeClientConnManager(getParams(), registry);
            return cm;
        }
    }

    public final static int VPL_ERR_CANCELED = -9087;
    public final static int VPL_ERR_FAIL = -9099;
    public final static int VPL_ERR_RESPONSE_TRUNCATED = -9612;
    public final static int VPL_ERR_IN_RECV_CALLBACK = -9613;
    
    private final static int READ_BUFFER_SIZE = 1024 * 16;
    private final static int DOWNLOAD_BUFFER_SIZE = 4096;
    
    // This is safe to use this from multiple threads, since we use a ThreadSafeClientConnManager.
    private static MyHttpClient mClient;

    private static class RequestState
    {
     // Marked volatile to ensure that it is assigned only after being fully constructed.
        public volatile HttpUriRequest request;
        public String httpMethod;
        public boolean cancel = false;
        
        public int cancel()
        {
            int rv = 0;
            cancel = true;
            try {
                // This is only safe because request is volatile.
                if (request != null) {
                    request.abort();
                }
            }
            catch (UnsupportedOperationException e) {
                Log.e(logTag, "cancel exception:", e);
                rv = exceptionToVplexError(e);
            }
            return rv;
        }
    }

    private static Map<Long, RequestState> requests = new HashMap<Long, RequestState>();
    /** Lock requests before accessing this field. */
    private static long nextRequestHandle = 1;

    private static AtomicBoolean initialized = new AtomicBoolean(false);

    private HttpManager2()
    {
    }

    /**
     * NOTE: this function is called from the native code.
     */
    public static void init()
    {
        synchronized (initialized) {
            if (!initialized.get()) {
                try {
                    mClient = new MyHttpClient();
                    initialized.set(true);
                }
                catch (Exception e) {
                    Log.e(logTag, "init failed", e);
                }
            }
        }
    }

    /**
     * NOTE: this function is called from the native code.
     */
    public static long createRequest()
    {
        long newRequestHandle;
        synchronized (requests) {
            RequestState newRequest = new RequestState();
            newRequestHandle = nextRequestHandle++;
            requests.put(Long.valueOf(newRequestHandle), newRequest);
        }
        Log.d(logTag, "Created HTTP request " + newRequestHandle);
        return newRequestHandle;
    }

    /**
     * NOTE: this function is called from the native code.
     */
    public static void destroyRequest(long requestHandle)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.remove(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
        } else {
            state.cancel();
            Log.d(logTag, "Destroyed HTTP request " + requestHandle);
        }
    }

    /**
     * NOTE: this function is called from the native code.
     * @return VPL error code.
     */
    public static int cancel(long requestHandle)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        } else {
            Log.d(logTag, "Canceling request " + requestHandle);
            return state.cancel();
        }
    }

    /**
     * NOTE: this function is called from the native code.
     * @param headers HTTP headers, separated by newlines
     */
    public static int setCommonParams(long requestHandle, String url, String headers, String method, int timeoutMs)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        }
        int rv = 0;
        try {
            boolean skipContentLengthHeader = false;
            state.httpMethod = method;
            if (method.equals("POST")) {
                state.request = new HttpPost(url);
                skipContentLengthHeader = true;
            }
            else if (method.equals("PUT")) {
                state.request = new HttpPut(url);
                skipContentLengthHeader = true;
            }
            else if (method.equals("DELETE")) {
                state.request = new HttpDelete(url);
            }
            else {
                state.request = new HttpGet(url);
            }
            if (headers.length() > 0) {
                String[] headersArray = headers.split("\n", 0);
                for (int i = 0; i < headersArray.length; i++) {
                    String[] hdr = headersArray[i].split(":", 2);
                    if (hdr.length != 2) {
                        throw new IllegalArgumentException("Invalid header: " + headersArray[i]);
                    }
                    // Bug 12368: Calling setEntity() implicitly sets "Content-Length".
                    //   Setting "Content-Length" twice would cause an exception, so skip it here.
                    if (skipContentLengthHeader && hdr[0].trim().equals("Content-Length"))
                        continue;
                    state.request.addHeader(hdr[0].trim(), hdr[1].trim());
                }
            }
            // Set "Connection: Keep-Alive":
            state.request.setHeader(HTTP.CONN_DIRECTIVE, HTTP.CONN_KEEP_ALIVE);
            // Set "User-Agent: vplex-android":
            state.request.setHeader(HTTP.USER_AGENT, "vplex-android");
            HttpParams params = state.request.getParams();
            HttpConnectionParams.setConnectionTimeout(params, timeoutMs);
            HttpConnectionParams.setSoTimeout(params, timeoutMs);
            // Probably not needed, but possibly causes problems on 2.2 without
            // this.
            HttpConnectionParams.setSocketBufferSize(params, 8192);
            state.request.setParams(params);
        }
        catch (Exception e) {
            if (state.cancel) {
                return VPL_ERR_CANCELED;
            }
            Log.e(logTag, "connect exception:", e);
            rv = exceptionToVplexError(e);
        }
        return rv;
    }

    /**
     * NOTE: this function is called from the native code.
     * @param content send http content from string
     */
    public static int sendHttpContentFromString(long requestHandle, String content)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        }
        int rv = 0;
        try {
            if (state.httpMethod.equals("POST")) {
                ((HttpPost)state.request).setEntity(new StringEntity(content, HTTP.UTF_8));
            }
            else if (state.httpMethod.equals("PUT")) {
                ((HttpPut)state.request).setEntity(new StringEntity(content, HTTP.UTF_8));
            } else {
                Log.e(logTag, "Unexpected for " + state.httpMethod);
            }
        } catch (Exception e) {
            if (state.cancel) {
                return VPL_ERR_CANCELED;
            }
            Log.e(logTag, "sendHttpContentFromString exception:", e);
            rv = exceptionToVplexError(e);
        }
        return rv;
    }

    /**
     * NOTE: this function is called from the native code.
     * @param filepath send http content from file
     */
    public static int sendHttpContentFromFile(long requestHandle,
            long cHttp2ImplPtr,
            final String filepath)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        }
        int rv = 0;
        try {
            File file = new File(filepath);
            long totalSize = file.length();
            InputStreamEntityWithProgress reqEntity =
                    new InputStreamEntityWithProgress(
                        new FileInputStream(file), totalSize, cHttp2ImplPtr);
            if (state.httpMethod.equals("POST")) {
                ((HttpPost)state.request).setEntity(reqEntity);
            }
            else if (state.httpMethod.equals("PUT")) {
                ((HttpPut)state.request).setEntity(reqEntity);
            } else {
                Log.e(logTag, "Unexpected for " + state.httpMethod);
            }
        } catch (Exception e) {
            if (state.cancel) {
                return VPL_ERR_CANCELED;
            }
            Log.e(logTag, "sendHttpContentFromFile exception:", e);
            rv = exceptionToVplexError(e);
        }
        return rv;
    }

    /**
     * NOTE: this function is called from the native code.
     */
    public static int sendHttpContentFromCallback(long requestHandle,
            final long cHttp2ImplPtr,
            final long cSendCallbackPtr,
            final long cSendSize)
    {
        RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        }
        InputStream in = new CSendCallbackInputStream(cHttp2ImplPtr, cSendCallbackPtr, cSendSize);
        InputStreamEntityWithProgress requestEntity =
                new InputStreamEntityWithProgress(in, cSendSize, cHttp2ImplPtr);
        if (state.httpMethod.equals("POST")) {
            ((HttpPost)state.request).setEntity(requestEntity);
        }
        else if (state.httpMethod.equals("PUT")) {
            ((HttpPut)state.request).setEntity(requestEntity);
        } else {
            Log.e(logTag, "Unexpected for " + state.httpMethod);
        }
        return 0;
    }
    
    /**
     * NOTE: this function is called from the native code.
     * @return Positive HTTP status code on success, or negative VPL error code on failure.
     */
    public static int connectAndRecvResponse(long requestHandle,
            final long http2ImplPtr,
            final long recvCbPtr)
    {
        final RequestState state;
        synchronized (requests) {
            state = requests.get(Long.valueOf(requestHandle));
        }
        if (state == null) {
            Log.e(logTag, "Request " + requestHandle + " no longer exists.");
            return VPL_ERR_FAIL;
        }
        try {
            ResponseHandler<Integer> responseHandler = new ResponseHandler<Integer>() {
                public Integer handleResponse(HttpResponse response) throws IOException
                {
                    Log.d(logTag, "Handling response");
                    int status = response.getStatusLine().getStatusCode();
                    callCResponseStatusCallback(http2ImplPtr, status);
                    Header[] headers = response.getAllHeaders();
                    String responseHeaders = new String();
                    for (int i = 0; i < headers.length; i++) {
                        responseHeaders += headers[i].getName();
                        responseHeaders += ": ";
                        responseHeaders += headers[i].getValue();
                        responseHeaders += "\n";
                    }
                    byte[] res = responseHeaders.getBytes();
                    callCResponseHeadersCallback(http2ImplPtr, res);
                    HttpEntity entity = response.getEntity();
                    if (entity != null) {
                        long dlTotal = entity.getContentLength();
                        long dlProgress = 0;
                        InputStream in = entity.getContent();
                        if (in != null) {
                            try {
                                byte[] tmp = new byte[DOWNLOAD_BUFFER_SIZE];
                                do {
                                    int l = in.read(tmp);
                                    if (l == -1) {
                                        break;
                                    }
                                    int bytesProcessed = callCRecvCallback(recvCbPtr, http2ImplPtr, tmp, l);
                                    if (bytesProcessed != l) {
                                        Log.w(logTag, "Recv callback returned " + bytesProcessed + ", expected " + l);
                                        return VPL_ERR_IN_RECV_CALLBACK;
                                    }
                                    dlProgress += l;
                                    callCProgressCallback(http2ImplPtr, dlTotal, dlProgress, 0, 0);
                                    if (state.cancel) {
                                        Log.d(logTag, "Http request canceled.");
                                        return VPL_ERR_CANCELED;
                                    }
                                } while (true);
                            }
                            finally {
                                in.close();
                            }
                        }
                        if ((dlTotal >= 0) && (dlTotal != dlProgress)) {
                            Log.w(logTag, "Http response appears to have been truncated.");
                            return VPL_ERR_RESPONSE_TRUNCATED;
                        }
                    }
                    return status;
                }
            };
            int tempTimeout = HttpConnectionParams.getSoTimeout(state.request.getParams());
            if (state.cancel) {
                Log.d(logTag, "Http request canceled.");
                return VPL_ERR_CANCELED;
            }
            Log.d(logTag, "Executing HTTP request, timeout=" + tempTimeout);
            int status = mClient.execute(state.request, responseHandler);
            Log.d(logTag, "HTTP response: " + status);
            return status;
        }
        catch (Exception e) {
            if (state.cancel) {
                return VPL_ERR_CANCELED;
            }
            Log.e(logTag, "recvHttpResponse exception:", e);
            return exceptionToVplexError(e);
        }
    }

    /**
     * Convert a Java exception to a VPL or VPLex error code.
     * Sort of a best effort.
     */
    public static int exceptionToVplexError(Throwable throwable)
    {
        // By using "try, throw, catch" here, we have the compiler make sure that
        // all listed cases are reachable (since some exceptions are subclasses of others).
        try {
            throw throwable;
        }
        catch (IllegalStateException e) {
            return -9090; // VPL_ERR_NOT_INIT
        }
        catch (UnknownHostException e) {
            return -9032; // VPL_ERR_UNREACH
        }
        catch (SSLPeerUnverifiedException e) { // this catch must appear before the catch for SSLException
            // TODO: bug 17908: This logic is not completely correct:
            // VPL_ERR_SSL_DATE_INVALID is only one of many possible reasons for SSLPeerUnverifiedException to be thrown.
            return -9611; //VPL_ERR_SSL_DATE_INVALID
        }
        catch (SSLException e) { // this catch must appear after the catch for SSLPeerUnverifiedException
            return -9610; // VPL_ERR_SSL
        }
        catch (HttpHostConnectException e) { // this catch must appear before the catch for IOException
            return -9039; // VPL_ERR_CONNREFUSED
        }
        catch (IOException e) {
            return -9003; // VPL_ERR_IO
        }
        catch (Throwable t) {
            return -9601; // VPL_ERR_HTTP_ENGINE
        }
    }

    protected static native int callCSendCallback(
            long sendCbPtr,
            long http2ImplPtr,
            byte[] buffer);
    private static native int callCRecvCallback(
            long recvCbPtr,
            long http2ImplPtr,
            byte[] buffer,
            long bytesInBuffer);
    private static native int callCResponseStatusCallback(
            long http2ImplPtr,
            int status);
    private static native int callCResponseHeadersCallback(
            long http2ImplPtr,
            byte[] data);
    public static native int callCProgressCallback(
            long http2ImplPtr,
            long dltotal,
            long dlnow,
            long ultotal,
            long ulnow);
}

class CSendCallbackInputStream extends InputStream {
    final private long cHttp2ImplPtr;
    final private long cSendCallbackPtr;
    final private long cSendSize;
    private long remaining;

    public CSendCallbackInputStream(final long cHttp2ImplPtr, final long cSendCallbackPtr, final long cSendSize) {
        long r = cSendSize;
        this.cHttp2ImplPtr = cHttp2ImplPtr;
        this.cSendCallbackPtr = cSendCallbackPtr;
        this.cSendSize = cSendSize;
        this.remaining = cSendSize;
    }

    public int read() throws IOException {
        byte[] tmp = new byte[1];
        int read;
        int value;
        if (this.remaining <= 0) {
            Log.i(HttpManager2.logTag, "EOF");
            return -1;
        }
        read = HttpManager2.callCSendCallback(this.cSendCallbackPtr, this.cHttp2ImplPtr, tmp);
        if ((read <= 0) || (read > 1)) {
            // Negative is an error.
            // 0 is also unexpected, since we were told the size beforehand.
            Log.e(HttpManager2.logTag, "The VPLHttp2_SendCb callback returned " + read);
            return -1;
        }
        this.remaining--;
        value = tmp[0] >= 0 ? tmp[0] : tmp[0] + 256;
        return value;
    }
}
