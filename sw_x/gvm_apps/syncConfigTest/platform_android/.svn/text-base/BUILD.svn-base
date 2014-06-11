shared_cpp_lib(
    name = 'libsync_config_test-jni',
    includes = ['project/jni',],
    srcs = [
        'project/jni/syncconfigtest_jni.cpp'
    ],
    deps = [
        '/tests/sync_config/syncConfig:libsync_config_test',
    ],
    ldflags = '-Wl,--no-undefined',
    whole_archive = False,
    soname = 'libsync_config_test-jni.so',
)


android_app(
    name = "syncConfigTest",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'src1':'/gvm_core/vpl/java_android',
        'res': 'project/res',
        'assets': 'project/assets',
        'iGware.keystore': '/gvm_core/demos/iGware.keystore',
    },
    deps = [
        ':libsync_config_test-jni',
    ]
)
