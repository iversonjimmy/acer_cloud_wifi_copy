c_and_cxx_flags = ''
# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '0')
if ts2_no_pxd != '0':
    c_and_cxx_flags += '-DTS2_NO_PXD '
ts2_no_prx = os.environ.get('TS2_NO_PRX', '0')
if ts2_no_prx != '0':
    c_and_cxx_flags += '-DTS2_NO_PRX '

static_cpp_lib(
    name = 'libTs2Network',
    includes = ['include'],
    private_includes = [
    ],
    cflags = c_and_cxx_flags,
    cxxflags = c_and_cxx_flags,
    srcs = [
        'src/FrontEnd.cpp',
        'src/Ts2_RouteManager.cpp',
        'src/TcpDinListener.cpp',
    ] + ([
        'src/PrxP2PListener.cpp',
    ] if ts2_no_pxd == '0' else []),
    deps = [
        '/gvm_core/internal/Ts2/Link:libTs2Link',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/file_utils:libfile_utils',
        '/gvm_core/vplex:libvplex',
    ],
)
