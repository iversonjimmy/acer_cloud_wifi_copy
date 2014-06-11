static_cpp_lib(
    name = 'libpxd',
    includes = ['include'],
    srcs = [
            'src/pxd_mtwist.cpp',
            'src/pxd_util.cpp',
            'src/pxd_log.cpp',
            'src/pxd_event.cpp',
            'src/pxd_packet.cpp',
            'src/pxd_client.cpp',
           ],
    deps = [
            '/gvm_core/internal/log:liblog',
            '/gvm_core/vplex:libvplexcore',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/ans:libans'
           ],
)

cpp_binary(
    name = 'pxd_client_test',
    srcs = [
            'src/pxd_client_test.cpp',
           ],
    deps = [
            ':libpxd',
           ]
)
