# This is for building the protobuf tools (including protobuf compiler binary, protobuf compiler library,
# and protobuf runtime library).
# We only need to build this for the platform *hosting* the build.
# We had problems getting this Autoconf-based build working for platforms like Android,
# so there's a copy of just the runtime library under "../protobuflib".  (There's no
# need to ever run the compiler on Android/iOS/WinRT anyway.)

external_shared_cpp_lib(
    name = 'libprotobuf-shr',
    # Note that includes are relative to $(PBROOT) for external_* makegen targets:
    includes = ['usr/include'],
    outfile = 'usr/lib/libprotobuf.so'
)

external_static_cpp_lib(
    name = 'libprotobuf',
    # Note that includes are relative to $(PBROOT) for external_* makegen targets:
    includes = ['usr/include'],
    outfile = 'usr/lib/libprotobuf.a'
)

external_static_cpp_lib(
    name = 'libprotoc',
    # Note that includes are relative to $(PBROOT) for external_* makegen targets:
    includes = [
                'usr/include',
                # There are many useful functions in usr/lib/libprotoc.a, but some of the
                # headers aren't copied to usr/include; grab them from the source.
                'gvm_core/external/protobuf/protobuf-2.4.0a/src',
               ],
    outfile = 'usr/lib/libprotoc.a',
    deps = [':libprotobuf']
)

if platform == 'android':
    external_java_lib(
        name = 'protobuf-java',
        outs = ['usr/lib/protobuf-lite.jar'],
    )
