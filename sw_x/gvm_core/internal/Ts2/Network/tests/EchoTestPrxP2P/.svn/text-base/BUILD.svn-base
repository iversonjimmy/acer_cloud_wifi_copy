c_and_cxx_flags = ''

cpp_binary(
    name = "PrxP2PEchoTest",
    srcs = [
        'PrxP2PEchoTest.cpp',
        'PrxP2PServer.cpp',
        'PrxP2PClient.cpp',
    ],
    cflags = c_and_cxx_flags,
    cxxflags = c_and_cxx_flags,
    deps = [
        '/gvm_core/internal/Ts2/Network:libTs2Network',
        '/gvm_core/internal/Ts2/LocalInfo:libTs2LocalInfoIH',
        '/gvm_core/internal/ans:libans',
    ],
)
