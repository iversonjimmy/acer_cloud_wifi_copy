package com.igware.vpl.android;

import android.util.Log;
import android.os.Environment;

public class DeviceManager
{
    private static final String logTag = "DeviceManager";

    private DeviceManager() { } 

    /**
     * Note that this function is called from the native code.
     * @return UUID of Acer device
     */
    public static String getHwUuid ( ) {
        // TODO: get device id for non-Acer device.
        return null;
    }

    /**
     * Note that this function is called from the native code.
     * @return String of external storage directory
     */
    public static String getExternalStorageDirectory( ) {

        return Environment.getExternalStorageDirectory().toString();
    }

}
