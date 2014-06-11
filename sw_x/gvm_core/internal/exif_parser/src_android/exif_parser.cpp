#include <vpl_plat.h>
#include <vplex_time.h>
#include "vpl_java.h"
#include <string>
#include <log.h>
#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include <exif_util.h>
#include <exif_parser.h>
#include <time.h>

static const char* CLASSNAME = "com/igware/vpl/android/ExifManager";

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

using namespace std;

int EXIFGetImageTimestamp(const string& filename,
                          VPLTime_CalendarTime& dateTime,
                          VPLTime_t& timestamp)
{
    int rv = -1;
    jclass * clazz; 
    jstring jFile;
    jstring jTag; 
    jmethodID mid;

    JNIEnv *env = VPLJava_GetCurrentThreadEnv();
    CHECK_NOT_NULL(env, rv, VPL_FALSE, fail, "Failed to get JNI environment");

    rv = VPLJava_LoadClass(CLASSNAME, &clazz);
    if (rv != VPL_OK) {
        LOG_ERROR("Class %s not loaded", CLASSNAME);
        goto fail;
    }

    mid = env->GetStaticMethodID(*clazz, "getDateTimeTag", "(Ljava/lang/String;)Ljava/lang/String;");
    CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

    if (!filename.empty()) {
        jFile = env->NewStringUTF(filename.c_str());      
        CHECK_EXCEPTIONS(env, rv, VPL_ERR_FAIL, fail);

        jTag = (jstring) env->CallStaticObjectMethod(*clazz, mid, jFile);
        if (jTag != 0) {
            const char *tag = env->GetStringUTFChars(jTag, 0);

            // Entry should be of the form:
            // DateTime: YYYY:MM:DD HH:MM:SS
            // Convert to time.
            if (tag == NULL) {
                rv = VPL_ERR_FAIL;
                LOG_WARN("Fail to get tag string");
                goto fail_after_jtag;
            } 
            LOG_INFO("Extracted timestamps string {%s} from file %s.", tag, filename.c_str());

            // Convert the timestamp.
            rv = EXIFTagToCalendarTime(tag, dateTime);

            if (rv == 0) 
            {
                struct tm temp;
                temp.tm_year = dateTime.year - 1900;
                temp.tm_mon = dateTime.month - 1;
                temp.tm_mday = dateTime.day;
                temp.tm_hour = dateTime.hour;
                temp.tm_min = dateTime.min;
                temp.tm_sec = dateTime.sec;
                // Since the timestamp will be calculated back to local time, we can ignore the daylight saving time.
                temp.tm_isdst = 0;
                // The mktime() creates an UTC timestamp, recalculate it back to local time.
                // Since PicStream sends the timestamp to server without timezone data, the timestamp being created here should be the current local time not UTC time.
                timestamp = (VPLTime_t) (mktime(&temp) - timezone);
            }

            env->ReleaseStringUTFChars(jTag, tag);
fail_after_jtag:
            env->DeleteLocalRef(jTag);
        } else {
            rv = VPL_ERR_FAIL;
        }

        env->DeleteLocalRef(jFile);
    }

fail:
    return rv;
}
