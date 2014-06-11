cpp_binary(
    name = 'loadgen_proxy',
    includes = [
               '/gvm_core/internal/vssi/src',
               '../common',
               '/gvm_core/internal/ans/include',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'loadgen_proxy.cpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',
            'test_client.cpp',            
            'test_server.cpp',
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/internal/ans:libans',
            '/gvm_core/vplex:libvplex',
           ],
)

