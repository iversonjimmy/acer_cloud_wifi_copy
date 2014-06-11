#include "com_igware_android_services_ServiceSingleton.h"

#include "ccd_core.h"
#include "ccd_core_service.hpp"
#include "log.h"
#include "ByteArrayProtoChannel.h"

//----------------------------------------------------------
// JNI calls from ServiceSingleton.
//----------------------------------------------------------

/*
 * Class:     com_igware_android_services_ServiceSingleton
 * Method:    startNativeService
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_igware_android_1services_ServiceSingleton_startNativeService
  (JNIEnv* env, jclass, jstring filesDirStr, jstring titleIdStr, jstring brandNameStr, jstring appNameStr)
{
    jboolean isCopy;
    const char* filesDir = env->GetStringUTFChars(filesDirStr, &isCopy);
    const char* titleId = env->GetStringUTFChars(titleIdStr, &isCopy);
    const char* brandName = env->GetStringUTFChars(brandNameStr, &isCopy);
    const char* appName = env->GetStringUTFChars(appNameStr, &isCopy);
    CCDSetBrandName(brandName);
    int rv = CCDStart(appName, filesDir, NULL, titleId);
    env->ReleaseStringUTFChars(titleIdStr, titleId);
    env->ReleaseStringUTFChars(filesDirStr, filesDir);
    env->ReleaseStringUTFChars(brandNameStr, brandName);
    env->ReleaseStringUTFChars(appNameStr, appName);
    return (rv == 0);
}

/*
 * Class:     com_igware_android_services_ServiceSingleton
 * Method:    stopNativeService
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_igware_android_1services_ServiceSingleton_stopNativeService
  (JNIEnv *, jclass)
{
    CCDShutdown();
    CCDWaitForExit();
    return VPL_TRUE;
}

/*
 * Class:     com_igware_android_services_ServiceSingleton
 * Method:    ccdiJniProtoRpc
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_igware_android_1services_ServiceSingleton_ccdiJniProtoRpc
  (JNIEnv * env, jclass, jbyteArray requestJavaArray, jboolean fromOtherApp)
{
    jbyte* requestBuf = env->GetByteArrayElements(requestJavaArray, NULL);
    ByteArrayProtoChannel tempChannel(requestBuf, env->GetArrayLength(requestJavaArray));
    bool success;
    ccd::CCDIServiceImpl service;
    if (fromOtherApp) {
        LOG_ERROR("Protobuf RPC (game) deprecated ");
        success = false;
    } else {
        LOG_INFO("Handling protobuf RPC (system)");
        success = service.CCDIService::handleRpc(tempChannel,
                                                 ccd::CCDProtobufRpcDebugGenericCallback,
                                                 ccd::CCDProtobufRpcDebugRequestCallback,
                                                 ccd::CCDProtobufRpcDebugResponseCallback);
    }

    // We didn't change the data, so use JNI_ABORT to avoid copying back.
    // If the isCopy flag from GetByteArrayElements was false (indicating that
    // the memory is pinned), then the flag is ignored anyway.
    env->ReleaseByteArrayElements(requestJavaArray, requestBuf, JNI_ABORT);
    const std::string& responseStr = tempChannel.getOutputAsString();
    jbyteArray result = env->NewByteArray(responseStr.size());
    env->SetByteArrayRegion(result, 0, responseStr.size(),
            reinterpret_cast<const jbyte*>(responseStr.c_str()));
    return result;
}
