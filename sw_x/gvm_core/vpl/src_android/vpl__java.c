//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#include "vpl_java.h"

#include "vpl_error.h"
#include "vplu_debug.h"

#include <pthread.h>

static JavaVM * _jvm = NULL;
static pthread_key_t _jenvKey;

static void
jvmDetachThread(void * env)
{
    JNIEnv * jenv = (JNIEnv *)env;
    (void)jenv;
    // TODO error?
    if (_jvm != NULL) {
        (*_jvm)->DetachCurrentThread(_jvm);
    }
}

JNIEnv *
VPLJava_GetCurrentThreadEnv(void)
{
    JNIEnv * env = pthread_getspecific(_jenvKey);
    if (env == NULL && _jvm != NULL) {
        if ((*_jvm)->AttachCurrentThread(_jvm, &env, NULL) != 0) {
            // TODO error
        }
        pthread_setspecific(_jenvKey, env);
    }
    return env;
}

typedef struct {
    const char * name;
    jclass clazz;
} NamedClass;

#define NUM_PRELOAD_CLASSES 4
static const char * classNames[NUM_PRELOAD_CLASSES] = {
    "com/igware/vpl/android/HttpManager2",
    "com/igware/vpl/android/DeviceManager",
    "com/igware/vpl/android/ExifManager",
    "com/igware/vpl/android/ImageTranscodeHelper",
};
static NamedClass classes[NUM_PRELOAD_CLASSES];

int
VPLJava_LoadClass(const char * name, jclass ** out)
{
    int i;

    for (i = 0; i < NUM_PRELOAD_CLASSES; i++) {
        if (strcmp(name, classes[i].name) == 0) {
            *out = &classes[i].clazz;
            return VPL_OK;
        }
    }
    return VPL_ERR_INVALID;
}

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM * vm, void * reserved)
{
    JNIEnv * env;
    int i;
    jclass clazz;

    // TODO: Check errors.  Is this able to throw an exception up to the Java if this fails?
    _jvm = vm;
    // This ensures that jvmDetachThread will be called for any thread in this process that exits
    // with a non null _jenvKey.
    pthread_key_create(&_jenvKey, jvmDetachThread);

    env = VPLJava_GetCurrentThreadEnv();
    for (i = 0; i < NUM_PRELOAD_CLASSES; i++) {
        classes[i].name = classNames[i];
        clazz = (*env)->FindClass(env, classNames[i]);
        classes[i].clazz = (*env)->NewGlobalRef(env, clazz);
        (*env)->DeleteLocalRef(env, clazz);
    }
    VPL_REPORT_INFO("JNI_OnLoad finished");

    return JNI_VERSION_1_4;
}

void VPLJava_LogException(JNIEnv* env, jthrowable t)
{
    jmethodID toString = NULL;
    jstring jMsg = NULL;
    const char* msg = NULL;
    // NOTE: After an exception has been raised, the native code must first clear the exception before making other JNI calls.
    //   ReleaseStringUTFChars and DeleteLocalRef are safe to call though.
    //   http://docs.oracle.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp17626
    {
        // Get the Object.toString() method.
        (*env)->ExceptionClear(env);
        toString = (*env)->GetMethodID(env, (*env)->FindClass(env, "java/lang/Object"), "toString", "()Ljava/lang/String;");
        if ((toString == NULL) || (*env)->ExceptionCheck(env)) {
            VPL_REPORT_WARN("Failed to get toString.");
            goto done;
        }
        // Call t.toString().
        (*env)->ExceptionClear(env);
        jMsg = (jstring) (*env)->CallObjectMethod(env, t, toString);
        if ((jMsg == NULL) || (*env)->ExceptionCheck(env)) {
            VPL_REPORT_WARN("Failed to call toString.");
            goto done;
        }
        // Get the c-string.
        (*env)->ExceptionClear(env);
        msg = (*env)->GetStringUTFChars(env, jMsg, NULL);
        if (msg == NULL) {
            VPL_REPORT_WARN("Failed to get array");
            goto done;
        }
        // Log it.
        VPL_REPORT_WARN("Exception: %s", msg);
    }
done:
    if (msg != NULL) {
        (*env)->ReleaseStringUTFChars(env, jMsg, msg);
    }
    if (jMsg != NULL) {
        (*env)->DeleteLocalRef(env, jMsg);
    }
    if (toString != NULL) {
        (*env)->DeleteLocalRef(env, toString);
    }
    (*env)->ExceptionClear(env);
}
