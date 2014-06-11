shared_cpp_lib(
    name = 'libccd-jni',
    includes = ['project/jni'],
    private_includes = ['/gvm_core/daemons/ccd/rpc_server'],
    srcs = [
            'project/jni/ccd-jni.cpp',
           ],
    deps = [
            '/gvm_core/daemons/ccd:libcc',
            '/:liblog',
           ],
    cxxflags = '-Wno-psabi', # gcc 4.5 will fix this warning, but until then, it's nothing to worry about
    ldflags = '-Wl,--no-undefined',
    whole_archive = False,
    soname = 'libccd-jni.so',
)

# We only use this to let the Android build system modify the .so files to the desired format and
# check that the service classes still compile.
android_app(
    name = "build_native_libs_apk",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'res': 'project/res',
        'iGware.keystore': '/gvm_core/demos/iGware.keystore',
    },
    deps = [
        ':libccd-jni',
        '../build_jars:build_jars',
        '/gvm_core/external/protobuf:protobuf-java',
    ]
)

custom(
    name = 'build_native_libs',
    deps = [
        ':build_native_libs_apk',
    ],
    force = True, # Bug 4646: Currently, we must always do this, since "build_jars" deletes the entire ../sdk directory.
    input = [
        '%self.builddir%/build_native_libs_apk/libs/armeabi/libccd-jni.so',
    ],
    output = [
        '../sdk/libs/libccd-jni.so',
    ],
    builddirs = [
    ],
    recipe = [
        'mkdir -p %(self.builddir)s/../sdk/libs',
        'cp -f %(in1)s %(out1)s',
    ],
)
