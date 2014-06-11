#include "vplex_plat.h"
#include "vpl_java.h"
#include "vplex_private.h"

static const char* CLASSNAME = "com/igware/vpl/android/DeviceManager";

#define CHECK_NOT_NULL(var, rv, errcode, label, ...) \
    do { \
        if ((var) == NULL) { \
            VPL_LIB_LOG_ERR(VPL_SG_PLAT, __VA_ARGS__); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

#define CHECK_EXCEPTIONS(env, rv, errcode, label) \
    do { \
        if (env->ExceptionCheck()) { \
            VPL_LIB_LOG_ERR(VPL_SG_PLAT, "Exception during jvm callback"); \
            env->ExceptionDescribe(); \
            env->ExceptionClear(); \
            (rv) = (errcode); \
            goto label; \
        } \
    } while (0)

static int
getJavaClass(JNIEnv** env_out, jclass** class_out)
{
    int rv;
    *env_out = VPLJava_GetCurrentThreadEnv();
    CHECK_NOT_NULL(*env_out, rv, VPL_ERR_FAIL, out, "Failed to get JNI environment");
    rv = VPLJava_LoadClass(CLASSNAME, class_out);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_PLAT, "Class %s not loaded", CLASSNAME);
        goto out;
    }
    CHECK_NOT_NULL(*class_out, rv, VPL_ERR_FAIL, out, "Class was NULL");
out:
    return rv;
}

int VPL_GetExternalStorageDirectory(std::string& path)
{
    int rv = VPL_OK;  

    JNIEnv* env;
    jclass* clazz;
    rv = getJavaClass(&env, &clazz);
    if (rv != VPL_OK) {
        VPL_LIB_LOG_ERR(VPL_SG_PLAT, "getJavaClass failed: %d", rv);
        goto fail;
    }

    {
        jmethodID mid = env->GetStaticMethodID(*clazz, "getExternalStorageDirectory", "()Ljava/lang/String;");
        CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);
        jstring jPath = (jstring)env->CallStaticObjectMethod(*clazz, mid);
        CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

        const char* cPath = env->GetStringUTFChars(jPath, NULL);
        path = cPath;
        
        env->ReleaseStringUTFChars(jPath, cPath);
        env->DeleteLocalRef(jPath);
        
    }

fail:
    return rv;
}
