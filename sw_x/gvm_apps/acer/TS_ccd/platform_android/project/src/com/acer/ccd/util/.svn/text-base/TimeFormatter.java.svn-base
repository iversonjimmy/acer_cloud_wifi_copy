package com.acer.ccd.util;

import java.util.Formatter;
import java.util.Locale;

public class TimeFormatter {
    static StringBuilder mFormatBuilder = new StringBuilder();
    static Formatter mFormatter = new Formatter(mFormatBuilder, Locale.getDefault());

    public static String makeTimeString(long timeMs) {
        if (timeMs <= 0) {
            return "00:00";
        }
        int totalSeconds = (int)(timeMs / 1000);

        int seconds = totalSeconds % 60;
        int minutes = (totalSeconds / 60) % 60;
        int hours   = totalSeconds / 3600;

        mFormatBuilder.setLength(0);
        if (hours > 0) {
            return mFormatter.format("%d:%02d:%02d", hours, minutes, seconds).toString();
        } else {
            return mFormatter.format("%02d:%02d", minutes, seconds).toString();
        }
    }
}