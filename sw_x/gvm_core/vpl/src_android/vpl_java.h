//
//  Copyright 2011 iGware Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF IGWARE INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
//

#ifndef __VPL_JAVA_H__
#define __VPL_JAVA_H__

#include "vplu_debug.h"

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VPLJAVA_CHECK_NOT_NULL(var, rv, errcode, label, ...) \
    do { \
        if ((var) == NULL) { \
            VPL_REPORT_INFO( __VA_ARGS__); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

void VPLJava_LogException(JNIEnv* env, jthrowable t);

#ifdef __cplusplus // Note that the JNI syntax is not compatible across C and C++!
# define VPLJAVA_CHECK_EXCEPTIONS(env, rv, errcode, label) \
    do { \
        jthrowable e_ = (env)->ExceptionOccurred(); \
        if (e_ != NULL) { \
            VPL_REPORT_WARN("Exception during jvm callback"); \
            VPLJava_LogException((env), e_); \
            (env)->ExceptionClear(); \
            (rv) = (errcode); \
            (env)->DeleteLocalRef(e_); \
            goto label; \
        } \
    } while (0)
#else
# define VPLJAVA_CHECK_EXCEPTIONS(env, rv, errcode, label) \
    do { \
        jthrowable e_ = (*(env))->ExceptionOccurred(env); \
        if (e_ != NULL) { \
            VPL_REPORT_WARN("Exception during jvm callback"); \
            VPLJava_LogException((env), e_); \
            (*(env))->ExceptionClear(env); \
            (rv) = (errcode); \
            (*(env))->DeleteLocalRef((env), e_); \
            goto label; \
        } \
    } while (0)
#endif

/// Obtain the Java environment for the current thread.  If the thread
/// is a native thread not registered with the jvm, it will be registered
/// and a destructor is registered (via pthread_key_create) to release
/// the thread when it exits.
JNIEnv * VPLJava_GetCurrentThreadEnv(void);

/// Update out to point to the specified class.  This does not look up
/// the class at call time but uses a list of preloaded classes set up
/// in JNI_OnLoad, due to class loader issues in native threads.  The
/// class is a global reference owned by VPLJava.
int VPLJava_LoadClass(const char * name, jclass ** out);

/// This method must be updated to preload any classes needed from
/// native code (see VPLJava_LoadClass).  It is exported for the jvm
/// and should not be called by anything else.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * vm, void * reserved);

#ifdef __cplusplus
}
#endif

#endif /* include guard */
