package com.igware.sync_config.test;


public class SyncConfigNative {
    static final String TAG = "SyncConfigTestNatives";
    
    static {
        System.loadLibrary("sync_config_test-jni");
    }
    
    // Test cases
    native static int testSyncConfig(int argc, String[] argv);
    
}
