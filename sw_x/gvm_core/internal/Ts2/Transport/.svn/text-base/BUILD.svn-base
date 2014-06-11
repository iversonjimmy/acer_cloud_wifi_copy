c_and_cxx_flags = ''
# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '0')
if ts2_no_pxd != '0':
    c_and_cxx_flags += '-DTS2_NO_PXD '

static_cpp_lib(
    name = 'libTs2Transport',
    includes = ['include'],
    private_includes = [
        '/gvm_core/internal/Ts2/include',
        '/gvm_core/internal/Ts2/src',
    ],
    cflags = c_and_cxx_flags,
    cxxflags = c_and_cxx_flags,
    srcs = [
        'src/Vtunnel.cpp',
        'src/Pool.cpp',
    ],
    deps = [
        '/gvm_core/internal/Ts2/Network:libTs2Network',
        '/gvm_core/internal/Ts2/LocalInfo:libTs2LocalInfo',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/log:liblog',
    ],
)
