static_cpp_lib(
    name = 'libTs2Wrapper',
    private_includes = [
        '/gvm_core/internal/vssi/src',
        '/gvm_core/daemons/ccd/include',
    ],
    includes = [
        '/gvm_core/internal/Ts2/include'
    ],
    srcs = [
        'src/ts_pool.cpp',
        'src/ts_client.cpp',
    ],
    deps = [
        '/gvm_core/internal/Ts2/LocalInfo:libTs2LocalInfo',
        '/gvm_core/daemons/ccd:libccd_types',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/file_utils:libfile_utils',
        '/gvm_core/internal/vssi:libvssi',
        '/gvm_core/vplex:libvplex',
    ],
)
