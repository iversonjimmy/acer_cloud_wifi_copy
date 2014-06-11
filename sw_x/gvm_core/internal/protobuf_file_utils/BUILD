static_cpp_lib(
    name = 'libprotobuf_file_utils',
    includes = ['include'],
    srcs = [
            'src/protobuf_file_reader.cpp',
            'src/protobuf_file_writer.cpp',
           ],
    deps = [
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/log:liblog',
            '/gvm_core/internal/error_codes:liberror_codes',
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/vplex:libvplex',
           ] +
           (['/gvm_core/external/protobuflib:protobuflib-shr'] if product == 'cloudnode' else ['/gvm_core/external/protobuflib:protobuflib']),
)
