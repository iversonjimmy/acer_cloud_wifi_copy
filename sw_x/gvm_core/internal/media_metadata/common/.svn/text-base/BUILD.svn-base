protobuf(
    name = 'media_metadata_types_proto',
    protofile = 'media_metadata_types.proto',
    input_path = 'include',
    deps = [],
    hpp_out = 'include',
    cpp_out = 'src',
    java_out = 'java',
)

if platform != 'ios' and platform != 'msvc':
    static_cpp_lib(
        name = 'libmedia_metadata_common',
        includes = ['include'],
        srcs = [
                'src/media_metadata_types.pb.cc',
               ],
        deps = [
                ':media_metadata_types_proto',
                '/gvm_core/external/protobuflib:protobuflib',
               ],
    )
