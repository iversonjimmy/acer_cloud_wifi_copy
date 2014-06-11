#include "image_transcode.h"
#include "image__transcode.h"

#include <vpl_types.h>
#include <vplu_format.h>
#include <vpl_time.h>
#include <vpl_fs.h>
#include <vplex_file.h>
#include "vpl_java.h"

#include <jni.h>
#include <stdio.h>
#include <string.h>
#include <sstream>

#include <log.h>

static const char* CLASSNAME = "com/igware/vpl/android/ImageTranscodeHelper";

#define CHECK_NOT_NULL(var, rv, errcode, label, ...) \
    do { \
        if ((var) == NULL) { \
            LOG_ERROR(__VA_ARGS__); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

// TODO convert exceptions to vplex errors
#define CHECK_EXCEPTIONS(env, rv, errcode, label) \
    do { \
        if ((env)->ExceptionCheck()) { \
            LOG_ERROR("Exception during jvm callback"); \
            (env)->ExceptionDescribe(); \
            (env)->ExceptionClear(); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

static unsigned char* as_unsigned_char_array(JNIEnv* env, jbyteArray array) {
    int len = env->GetArrayLength(array);
    unsigned char* buf = new unsigned char[len];
    env->GetByteArrayRegion (array, 0, len, reinterpret_cast<jbyte*>(buf));
    return buf;
}

int ImageTranscode_init_private()
{
    return IMAGE_TRANSCODING_OK;
}

int ImageTranscode_transcode_private(std::string filepath,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    int rv = IMAGE_TRANSCODING_OK;
    jclass* clazz;
    jmethodID mid;
    JNIEnv* env;
    jstring jstr_filepath;
    jbyteArray jbyte_array;

    env = VPLJava_GetCurrentThreadEnv();
    CHECK_NOT_NULL(env, rv, VPL_FALSE, fail, "Failed to get JNI environment");

    rv = VPLJava_LoadClass(CLASSNAME, &clazz);
    if (rv != VPL_OK) {
        LOG_ERROR("Class %s not loaded", CLASSNAME);
        goto fail;
    }

    mid = env->GetStaticMethodID(*clazz, "scaleImage", "(Ljava/lang/String;III)[B");
    CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

    jstr_filepath = env->NewStringUTF(filepath.c_str());
    CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

    jbyte_array = (jbyteArray) env->CallStaticObjectMethod(*clazz, mid, jstr_filepath, width, height, type);

    if (jbyte_array == NULL) {
        LOG_ERROR("Failed to transcode image: %s", filepath.c_str());
        rv = IMAGE_TRANSCODING_FAILED;
        goto fail;
    }
    *out_image = as_unsigned_char_array(env, jbyte_array);
    out_image_len = env->GetArrayLength(jbyte_array);

fail:
    return rv;
}

int ImageTranscode_transcode_private(const unsigned char* image,
                                     size_t image_len,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    // Deprecated. Not support by using ffmpeg.
    return IMAGE_TRANSCODING_FAILED;
}

int ImageTranscode_shutdown_private()
{
    return IMAGE_TRANSCODING_OK;
}
