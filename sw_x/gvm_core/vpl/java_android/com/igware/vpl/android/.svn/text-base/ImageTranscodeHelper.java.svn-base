package com.igware.vpl.android;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.File;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.BitmapFactory.Options;
import android.util.Log;

public class ImageTranscodeHelper {

    final static public int ImageType_JPG = 0;
    final static public int ImageType_PNG = 1;
    final static public int ImageType_TIFF = 2;
    final static public int ImageType_BMP = 3;
    final static public int ImageType_GIF = 4;
    final static public int ImageType_Original = 5;

    final static private String LOG_TAG = "ImageTranscodeHelper";

    private ImageTranscodeHelper() {

    }

    /**
     * Note that this function is called from the native code.
     * @return success(0) or fail(-1)
     */
    static public byte[] scaleImage(String filepath, int dst_width, int dst_height, int type) {
        File file = new File(filepath);
        if(!file.exists()){
            Log.e(LOG_TAG, "File does not exist: " + filepath);
            return null;
        }

        //first to fetch imageSize
        Options options = new Options();
        options.inJustDecodeBounds = false;
        Bitmap src_bitmap = BitmapFactory.decodeFile(filepath, options);
        int org_width = options.outWidth;
        int org_height = options.outHeight;

        //get down sample size(choose power of 2) to fix dst image size
        float scale_x = ((float) org_width) / dst_width;
        float scale_y = ((float) org_height) / dst_height;
        float scale = Math.max(scale_x, scale_y);

        if(type == ImageType_Original){
            Log.i(LOG_TAG, "options.outMimeType: " + options.outMimeType);
            if(options.outMimeType.equalsIgnoreCase("image/jpeg")){
                type = ImageType_JPG;
            }else if(options.outMimeType.equalsIgnoreCase("image/png")){
                type = ImageType_PNG;
            }else if(options.outMimeType.equalsIgnoreCase("image/bmp")){
                type = ImageType_BMP;
            }else if(options.outMimeType.equalsIgnoreCase("image/tiff")){
                type = ImageType_TIFF;
            }else if(options.outMimeType.equalsIgnoreCase("image/gif")){
                type = ImageType_GIF;
            }else{
                Log.w(LOG_TAG, "Unknown image type, transcode to JPG.");
                type = ImageType_JPG;
            }
        }

        if(org_width == 0 || org_height == 0){
            Log.e(LOG_TAG, "Failed to get original image size, org_width = " + org_width + ", org_height = " + org_height);
            return null;
        }

        Bitmap bitmap = null;
        if (dst_width >= org_width && dst_height >= org_height) {
            bitmap = src_bitmap;

        } else {
            double r1 = (double) org_height / org_width;
            double r2 = (double) dst_height / dst_width;

            // org_width > org_height
            if(r1 < 1){
                if(r1 <= r2){
                    bitmap = Bitmap.createScaledBitmap(src_bitmap, dst_width, (int) Math.round(dst_width * r1), false);
                }else{
                    bitmap = Bitmap.createScaledBitmap(src_bitmap, (int) Math.round(dst_height / r1), dst_height, false);
                }

            // org_width <= org_height
            }else{
                if(r1 >= r2){
                    bitmap = Bitmap.createScaledBitmap(src_bitmap, (int) Math.round(dst_height / r1), dst_height, false);
                }else{
                    bitmap = Bitmap.createScaledBitmap(src_bitmap, dst_width, (int) Math.round(dst_width * r1), false);
                }
            }
        }

        if(bitmap == null){
            Log.e(LOG_TAG, "Failed to scale image: " + filepath);
            return null;
        }

        switch(type){
            case ImageType_JPG:
                ByteArrayOutputStream jpg_output = new ByteArrayOutputStream();
                bitmap.compress(Bitmap.CompressFormat.JPEG, 100, jpg_output);
                return jpg_output.toByteArray();

            case ImageType_PNG:
                ByteArrayOutputStream png_output = new ByteArrayOutputStream();
                bitmap.compress(Bitmap.CompressFormat.PNG, 100, png_output);
                return png_output.toByteArray();

            case ImageType_BMP:
            case ImageType_TIFF:
            case ImageType_GIF:
            case ImageType_Original:
            default:
                Log.e(LOG_TAG, "Unsupported format.");
                break;
        }
        return null;
    }
}
