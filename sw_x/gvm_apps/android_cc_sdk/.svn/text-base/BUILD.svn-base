pseudo(
    name = 'android_cc_sdk_all',
    deps = [
        ':vplex_without_internal',
        '/gvm_core/vplex/proto:vs_directory_service_types_jar_src',
        ':android_cc_sdk',
        ':android_cc_sdk_examples',
    ]
)

pseudo(
    name = 'android_cc_sdk_examples',
    deps = [
        'example_service_only:example_service_only_apk',
        # No longer supported:
        #'example_panel_and_service:example_panel_and_service_apk',
        #'example_remote_panel:example_remote_panel_apk',
    ]
)

custom(
    name = 'vplex_without_internal',
    input = [
        '$(SRCROOT)/make/scripts/remove_internal_source.py',
        '$(SRCROOT)/gvm_core/vplex/proto/vplex_vs_directory_service_types.proto',
    ],
    output = [
        '$(SRCROOT)/gvm_core/vplex/proto/sdk/vplex_vs_directory_service_types.proto',
    ],
    builddirs = ['$(SRCROOT)/gvm_core/vplex/proto'],
    recipe = [
        'mkdir -p $(SRCROOT)/gvm_core/vplex/proto/sdk',
        'python %(in1)s %(in2)s > %(out1)s',
    ],
)

custom(
    name = 'android_cc_sdk',
    deps = [
        'build_jars:build_jars',
        'build_native_libs:build_native_libs',
    ],
    force = True,
    input = [
        '$(PBROOT)/usr/lib/protobuf-lite.jar',
        'README.txt',
    ],
    output = [
        'sdk/libs/protobuf-lite.jar',
        'sdk/README.txt',
    ],
    builddirs = [
    ],
    recipe = [
        # build_jars is responsible for cleaning out and creating the 'sdk' subdirectory.
        # build_jars will add a bunch of .jar files to 'sdk'.
        # build_native_libs will add .so files to 'sdk'.
        'cp -f %(in1)s %(out1)s',
        'cp -f %(in2)s %(out2)s',
        'mkdir -p %(self.builddir)s/sdk/samples/example_panel_and_service/libs/armeabi',
        'cp -frt %(self.builddir)s/sdk/samples/example_panel_and_service/libs/armeabi'\
            ' %(self.builddir)s/sdk/libs/libccd-jni.so',
        'mkdir -p %(self.builddir)s/sdk/samples/example_panel_and_service/gen',
        'cp -frt %(self.builddir)s/sdk/samples/example_panel_and_service'\
            ' %(self.srcdir)s/example_panel_and_service/project/res'\
            ' %(self.srcdir)s/example_panel_and_service/project/src'\
            ' %(self.srcdir)s/example_panel_and_service/project/.classpath'\
            ' %(self.srcdir)s/example_panel_and_service/project/.project'\
            ' %(self.srcdir)s/example_panel_and_service/project/AndroidManifest.xml'\
            ' %(self.srcdir)s/example_panel_and_service/project/default.properties',
        'mkdir -p %(self.builddir)s/sdk/samples/example_remote_panel/gen',
        'cp -frt %(self.builddir)s/sdk/samples/example_remote_panel'\
            ' %(self.srcdir)s/example_remote_panel/project/res'\
            ' %(self.srcdir)s/example_remote_panel/project/src'\
            ' %(self.srcdir)s/example_remote_panel/project/.classpath'\
            ' %(self.srcdir)s/example_remote_panel/project/.project'\
            ' %(self.srcdir)s/example_remote_panel/project/AndroidManifest.xml'\
            ' %(self.srcdir)s/example_remote_panel/project/default.properties',
        'mkdir -p %(self.builddir)s/sdk/samples/example_service_only/libs/armeabi',
        'cp -frt %(self.builddir)s/sdk/samples/example_service_only/libs/armeabi'\
            ' %(self.builddir)s/sdk/libs/libccd-jni.so',
        'mkdir -p %(self.builddir)s/sdk/samples/example_service_only/gen',
        'cp -frt %(self.builddir)s/sdk/samples/example_service_only'\
            ' %(self.srcdir)s/example_service_only/project/res'\
            ' %(self.srcdir)s/example_service_only/project/src'\
            ' %(self.srcdir)s/example_service_only/project/.classpath'\
            ' %(self.srcdir)s/example_service_only/project/.project'\
            ' %(self.srcdir)s/example_service_only/project/AndroidManifest.xml'\
            ' %(self.srcdir)s/example_service_only/project/default.properties',
    ],
)
