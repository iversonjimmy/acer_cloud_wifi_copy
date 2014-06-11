cpp_binary(
    name = 'pxd_test_client',
    includes = [
                '/gvm_core/daemons/ccd/include',
               ],
    srcs = [
            'pxd_test_client.cpp',
            'pxd_test_common.cpp',
            '../PxdTestInfraHelper/PxdTestInfraHelper.cpp'
           ],
    deps = ['/gvm_core/vplex:libvplex',
            '/gvm_core/internal/pxd:libpxd',
            '/gvm_core/internal/p2p:libp2p',
            '/gvm_core/internal/ans:libans',
           ],
)
cpp_binary(
    name = 'pxd_test_server',
    includes = [
                '/gvm_core/daemons/ccd/include',
               ],
    srcs = [
            'pxd_test_server.cpp',
            'pxd_test_common.cpp',
            '../PxdTestInfraHelper/PxdTestInfraHelper.cpp'
           ],
    deps = ['/gvm_core/vplex:libvplex',
            '/gvm_core/internal/pxd:libpxd',
            '/gvm_core/internal/ans:libans',
            '/gvm_core/internal/p2p:libp2p',
           ],
)