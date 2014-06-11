package com.igware.vplex.test;


public class VplexNative {
    static final String TAG = "VplexTestNatives";
    
    static {
        System.loadLibrary("vplextest-jni");
    }
    
    // Test cases
    native static int testVPLex(int argc, String[] argv);
}
