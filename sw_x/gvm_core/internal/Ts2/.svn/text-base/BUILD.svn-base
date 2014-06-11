c_and_cxx_flags = ''
# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '0')
if ts2_no_pxd != '0':
    c_and_cxx_flags += '-DTS2_NO_PXD '

static_cpp_lib(
    name = 'libTs2',
    includes = ['include'],
    private_includes = [
        '/gvm_core/daemons/ccd/include',
    ],
    cflags = c_and_cxx_flags,
    cxxflags = c_and_cxx_flags,
    srcs = [
        'src/ts_api.cpp',
        'src/ts_server.cpp',
    ],
    deps = [
        '/gvm_core/internal/Ts2/Transport:libTs2Transport',
        '/gvm_core/internal/Ts2/Network:libTs2Network',
        '/gvm_core/internal/Ts2/Link:libTs2Link',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/Ts2/Packet:libTs2Packet',
    ],
)

pseudo(
    name = 'tests',
    deps = [
        '/gvm_core/internal/Ts2/tests/ApiTest:apiTest',
        '/gvm_core/internal/Ts2/Network/tests/EchoTest:TcpDinEchoTest',
    ]
    + ([
        '/gvm_core/internal/Ts2/Link/tests/EchoTest:TcpDinEchoTest',
    ] if ts2_no_pxd != '0' else [
        '/gvm_core/internal/Ts2/Network/tests/EchoTestPrxP2P:PrxP2PEchoTest',
    ]),
)
