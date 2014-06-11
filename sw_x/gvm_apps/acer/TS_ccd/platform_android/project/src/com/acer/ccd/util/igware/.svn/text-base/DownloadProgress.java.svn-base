//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

package com.acer.ccd.util.igware;

public class DownloadProgress {
    
    private String mName;
    private String mLocation;
    private long mBytesDownloaded;
    private long mBytesTotal;
    
    // IEC-compliant binary prefixes
    private static final int KIBIBYTE = 1024;
    private static final int MEBIBYTE = KIBIBYTE * 1024;
    private static final int GIBIBYTE = MEBIBYTE * 1024;
    
    String getLocation() {
        
        return mLocation;
    }
    
    public DownloadProgress(String name, String location, long bytesDownloaded,
            long bytesTotal) {

        this.mName = name;
        this.mLocation = location;
        this.mBytesDownloaded = bytesDownloaded;
        this.mBytesTotal = bytesTotal;
    }
    
    /**
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {

        if (mBytesTotal <= 0) {
            return mName + ": Not started";
        }
        float percent = (100.0f * mBytesDownloaded) / mBytesTotal;
        String numBytesFormat;
        float numBytesDisplay;
        if (mBytesTotal > 999 * MEBIBYTE) {
            numBytesFormat = "%.2f GiB";
            numBytesDisplay = (float) mBytesTotal / GIBIBYTE;
        }
        else if (mBytesTotal > 999 * KIBIBYTE) {
            numBytesFormat = "%.2f MiB";
            numBytesDisplay = (float) mBytesTotal / MEBIBYTE;
        }
        else if (mBytesTotal > 999) {
            numBytesFormat = "%.2f KiB";
            numBytesDisplay = (float) mBytesTotal / KIBIBYTE;
        }
        else {
            numBytesFormat = "%.0f B";
            numBytesDisplay = mBytesTotal;
        }
        return String.format("%s: %.2f%% of " + numBytesFormat, mName, percent,
                numBytesDisplay);
    }
    
    public boolean isDownloaded() {

        return (mBytesDownloaded == mBytesTotal) && (mBytesTotal > 0);
    }
}
