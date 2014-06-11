android_app(
    name = "build_jars_apk",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'res': 'project/res',
        'src2': '/gvm_core/daemons/ccd/client/java',
        'src3': '/gvm_core/daemons/ccd/types/java',
        'src4': '/gvm_core/internal/protorpc/java',
        'src5': '/gvm_core/vplex/proto/java',
        'src6': '/gvm_core/vpl/java_android',
        'src7': '/gvm_core/internal/media_metadata/common/java',
        'iGware.keystore': '/gvm_core/demos/iGware.keystore',
    },
    deps = [
        '/gvm_core/internal/protorpc:proto_src',
        '/gvm_core/daemons/ccd:ccdi_rpc',
        '/gvm_core/external/protobuf:protobuf-java',
    ],
)

#----
# It would be better if we could pass these to jar, but jar doesn't support wildcards currently.
classes_ccdi_types = 'igware/gvm/pb'
classes_ccdi_aidl = 'com/igware/ccdi_android/ICcdiAidlRpc*'
classes_ccdi_abstract_client = 'com/igware/ccdi_android/AbstractCcdiClient*'
classes_ccdi_remote_client = 'com/igware/ccdi_android/CcdiClientRemoteBinder*'

classes_mca_types = 'igware/cloud/media_metadata/pb'
#----

custom(
    name = 'build_jars',
    deps = [
        ':build_jars_apk',
    ],
    force = True, # It's too hard to list all of the .java and .class files as input, so we will just run this every time.
                  # It's not a big deal because we currently have to run the Android Ant build every time too (so
                  # even if we listed the class files, they would be out-of-date in every incremental build anyway).
                  # Bug 4646: Unfortunately, this might also make parallel builds with "make -j<X>" unreliable.
    input = [
        '%self.builddir%/build_jars_apk',
        '%self.builddir%/build_jars_apk/bin/classes',
    ],
    output = [
        '../sdk/libs/igware_common.jar',
        '../sdk/libs/ccdi_types.jar',
        '../sdk/libs/mca_types.jar',
        '../sdk/libs/ccdi_client.jar',
    ],
    builddirs = [
    ],
    recipe = [
        'rm -rf %(self.builddir)s/../sdk', # Bug 4646: This is very bad news for parallel make.
        'mkdir -p %(self.builddir)s/../sdk/libs',
        # Copy the source files into the class directories too (for easier debugging).
        "cd %(in1)s/gen  && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src  && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src2 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src3 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src4 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src5 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src6 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        "cd %(in1)s/src7 && find . -name '*.java' -print | cpio -pavd $(PBROOT)/%(in2)s",
        # Create the jars.
        'jar -cMf %(out1)s '\
            ' -C %(in2)s igware/protobuf'\
            ' -C %(in2)s igware/vplex'\
            ' -C %(in2)s com/igware/vpl'\
            ,
        'jar -cMf %(out2)s '\
            ' -C %(in2)s igware/gvm/pb'\
            ,
        'jar -cMf %(out3)s '\
            ' -C %(in2)s igware/cloud/media_metadata/pb'\
            ,
        'jar -cMf %(out4)s '\
            ' -C %(in2)s com/igware/ccdi_android'\
            ,
    ],
)
