#include <string>
#include <android/log.h>

#include "com_igware_vplex_test_VplexNative.h"
#include "vplexTest.h"

#define LOG_INFO( info, p1 ) __android_log_print( ANDROID_LOG_WARN, "NATIVE CODE", info, p1 );

extern const char* server_url;
extern const char* branch;
extern const char* product;

JNIEXPORT jint JNICALL Java_com_igware_vplex_test_VplexNative_testVPLex
  (JNIEnv *env, jclass, jint jargc, jobjectArray jstringArray)
{
	int argc = (int)jargc;
	int ret = 0;
    char* raw_argv[argc];

    int argv_length = env->GetArrayLength(jstringArray);

	for(int i=0 ; i<argv_length ;i++) {
        jstring string = (jstring) env->GetObjectArrayElement( jstringArray, i);
        const char* raw_sting = env->GetStringUTFChars( string, 0);
        raw_argv[i] = (char* )raw_sting;
        LOG_INFO("argvString: %s ", raw_argv[i]);
	}
    ret = testVPLex(argc, raw_argv);

	for(int i=0 ; i<argv_length ;i++) {
		jstring string = (jstring) env->GetObjectArrayElement( jstringArray, i);
        env->ReleaseStringUTFChars(string, raw_argv[i]);
	}
    return ret;
}

