pseudo(
    name = 'vplextest_all',
    deps = [
        ':vplexTest',
    ]
)

shared_cpp_lib(
    name = 'libvplextest-jni',
    includes = ['project/jni'],
    srcs = [
        'project/jni/vplextest_jni.cpp'
    ],
    deps = [
        '/gvm_core/vplex:libvplex',
        '/gvm_core/vplex/tests:libvplextest',
    ],
    ldflags = '-Wl,--no-undefined',
    whole_archive = False,
    soname = 'libvplextest-jni.so',
)


android_app(
    name = "vplexTest",
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
        ':libvplextest-jni',
    ]
)
