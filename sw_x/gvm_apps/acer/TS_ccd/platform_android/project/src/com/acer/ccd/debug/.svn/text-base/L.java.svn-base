package com.acer.ccd.debug;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;

public class L {

    public static final int VERBOSE = 2;
    public static final int DEBUG = 3;
    public static final int INFO = 4;
    public static final int WARN = 5;
    public static final int ERROR = 6;
    public static final int ASSERT = 7;

    private static void out(int LEVEL, String TAG, String fmt, Object... args)
    {
        if (fmt == null)
            fmt = "";
        String s0 = String.format(fmt, args);
        String s;

        if (Debug.DEBUG == true) {
            StackTraceElement st = new RuntimeException().getStackTrace()[2];
            s = st.getMethodName() + "():" + st.getLineNumber() + ": " + s0;
        } else
            s = s0;

        switch (LEVEL) {
        case VERBOSE:
            Log.v(TAG, s); return;
        case DEBUG:
            Log.d(TAG, s); return;
        case INFO:
            Log.i(TAG, s); return;
        case WARN:
            Log.w(TAG, s); return;
        case ERROR:
            Log.e(TAG, s); return;
        case ASSERT:
            Log.wtf(TAG, s); return;
        default:
            break;
        }
    }
    public static void v(String TAG)
    { out(VERBOSE, TAG, null); }
    public static void v(String TAG, String fmt, Object... args)
    { out(VERBOSE, TAG, fmt, args); }
    public static void d(String TAG)
    { out(DEBUG, TAG, null); }
    public static void d(String TAG, String fmt, Object... args)
    { out(DEBUG, TAG, fmt, args); }
    public static void i(String TAG)
    { out(INFO, TAG, null); }
    public static void i(String TAG, String fmt, Object... args)
    { out(INFO, TAG, fmt, args); }
    public static void w(String TAG)
    { out(WARN, TAG, null); }
    public static void w(String TAG, String fmt, Object... args)
    { out(WARN, TAG, fmt, args); }
    public static void e(String TAG)
    { out(ERROR, TAG, null); }
    public static void e(String TAG, String fmt, Object... args)
    { out(ERROR, TAG, fmt, args); }
    public static void wtf(String TAG)
    { out(ASSERT, TAG, null); }
    public static void wtf(String TAG, String fmt, Object... args)
    { out(ASSERT, TAG, fmt, args); }

    // used in test phase build
    public static void t(String TAG)
    { if (Debug.DEBUG == true) out(ERROR, TAG, null); }
    public static void t(String TAG, String fmt, Object... args)
    { if (Debug.DEBUG == true) out(ERROR, TAG, fmt, args); }

    /** L.t() plus toast */
    public static void tt(String TAG, Context context, String fmt, Object... args) {
        if (Debug.DEBUG == false) return;
        L.t(TAG, fmt, args);
        Toast.makeText(context, String.format(fmt, args), Toast.LENGTH_SHORT).show();
    }

    public static void printException(String tag, Exception ex) {
        out(ERROR, tag, ex.toString(), null);
        StackTraceElement elements[] = ex.getStackTrace();
        for (StackTraceElement e : elements)
            out(ERROR, tag, e.toString(), null);
    }
}
