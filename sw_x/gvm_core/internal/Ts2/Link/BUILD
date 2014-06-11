c_and_cxx_flags = ''
# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '0')
if ts2_no_pxd != '0':
    c_and_cxx_flags += '-DTS2_NO_PXD '
ts2_no_prx = os.environ.get('TS2_NO_PRX', '0')
if ts2_no_prx != '0':
    c_and_cxx_flags += '-DTS2_NO_PRX '

static_cpp_lib(
    name = 'libTs2Link',
    includes = ['include'],
    private_includes = [
    ],
    cflags = c_and_cxx_flags,
    cxxflags = c_and_cxx_flags,
    srcs = [
        'src/EndPoint.cpp',
        'src/LinkProtocol.cpp',
        'src/ConnectHelper.cpp',
        'src/TcpDinConnHelper.cpp',
        'src/AcceptHelper.cpp',
        'src/TcpDinAcceptHelper.cpp',
    ] + ([
        'src/PrxP2PConnHelper.cpp',
        'src/PrxP2PAcceptHelper.cpp',
    ] if ts2_no_pxd == '0' else []),
    deps = [
        '/gvm_core/internal/Ts2/Packet:libTs2Packet',
        '/gvm_core/internal/Ts2/LocalInfo:libTs2LocalInfo',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/file_utils:libfile_utils',
        '/gvm_core/internal/InterruptibleSocket:libInterruptibleSocket',
        '/gvm_core/vplex:libvplex',
    ] + ([
        '/gvm_core/internal/p2p:libp2p',
        '/gvm_core/internal/pxd:libpxd',
    ] if ts2_no_pxd == '0' else []),
)
