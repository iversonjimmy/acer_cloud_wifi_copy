#include "com_igware_android_services_McaService.h"

#include "mca_control_point.hpp"
#include "mca_control_point_service.hpp"
#include "log.h"
#include "ByteArrayProtoChannel.h"

static void protobufRpcDebugCallback(const char* msg)
{
    LOG_INFO("%s", msg);
}

/*
 * Class:     com_igware_android_services_MediaMetaClientService
 * Method:    mcaJniProtoRpc
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_com_igware_android_1services_McaService_mcaJniProtoRpc
  (JNIEnv * env, jclass, jbyteArray requestJavaArray)
{
    jbyte* requestBuf = env->GetByteArrayElements(requestJavaArray, NULL);
    ByteArrayProtoChannel tempChannel(requestBuf, env->GetArrayLength(requestJavaArray));
    bool success;
    media_metadata::client::MCAForControlPointServiceImpl service;

    LOG_INFO("Handling protobuf RPC");
    success = service.handleRpc(tempChannel, protobufRpcDebugCallback);

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
