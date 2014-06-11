static_cpp_lib(
    name = 'libTs2ExtClient',
    private_includes = [
                '/gvm_core/daemons/ccd/include',
        ],
    includes = [
            '/gvm_core/internal/Ts2/include',
            '/gvm_core/internal/Ts2/LocalInfo/include',
           ],
    srcs = [
            'src/ts_ext_client.cpp',
            'src/ts_ext_pkt.cpp',
           ],
    deps = [
            '/gvm_core/daemons/ccd:libccd_types',
            '/gvm_core/internal/error_codes:liberror_codes',
           ],
)

static_cpp_lib(
    name = 'libTs2ExtServer',
    private_includes = [
            '/gvm_core/daemons/ccd/include',
           ],
    includes = [],
    srcs = [
            'src/ts_ext_server.cpp',
            'src/ts_ext_pkt.cpp',
           ],
    deps = [
            '/gvm_core/internal/Ts2:libTs2',
           ]
)
