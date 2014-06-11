c_and_cxx_flags = ''
# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '0')
if ts2_no_pxd != '0':
    c_and_cxx_flags += '-DTS2_NO_PXD '
ts2_no_prx = os.environ.get('TS2_NO_PRX', '0')
if ts2_no_prx != '0':
    c_and_cxx_flags += '-DTS2_NO_PRX '

if ts2_no_pxd != '0':
    cpp_binary(
        name = "TcpDinEchoTest",
        srcs = [
            'TcpDinEchoTest.cpp',
            'TcpDinServer.cpp',
            'TcpDinClient.cpp',
        ],
        cflags = c_and_cxx_flags,
        cxxflags = c_and_cxx_flags,
        deps = [
            '/gvm_core/internal/Ts2/Link:libTs2Link',
            '/gvm_core/internal/Ts2/LocalInfo:libTs2LocalInfoFV',
        ],
    )
