c_cxx_flags = ''

static_cpp_lib(
    name = 'libvssts',
    includes = [
        '/gvm_core/internal/vssts/include',
        '/gvm_core/internal/Ts2Alt/Ext/src',
        ],
    srcs = [
        'src/vssts.cpp',
        'src/vssts_internal.cpp'
        ],
    deps = [
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/internal/Ts2Alt/Ext:libTs2ExtClient',
        ],
)
