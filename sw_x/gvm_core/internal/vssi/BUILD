static_cpp_lib(
    name = 'libvssi',
    includes = ['include'],
    srcs = [
            'src/basic_vssi_wrapper.cpp',
            'src/vssi.c',
            'src/vssi_vss.c',
            'src/vssi_http.c',
            'src/vssi_internal.c',
            'src/vssi_tunnel.c',
           ],
    deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/log:liblog',
           ],
)

shared_cpp_lib (
    name = 'libvssi-shr',
    srcs = [],
    deps = [':libvssi'],
    soname = 'libvssi.so.1'
)
