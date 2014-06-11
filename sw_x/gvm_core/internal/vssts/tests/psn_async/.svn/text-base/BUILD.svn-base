cpp_binary(
    name = 'psn_async',
    includes = [
                '../common',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'psn_async.cpp',
            'vsTest_vscs_async.cpp',
            'vsTest_vscs_async.hpp',
            '../common/vsTest_infra.cpp'
           ],
    deps = [
            '/gvm_core/internal/vssts/shim:libvsstsshim',
            '/gvm_core/vplex:libvplex',
            '/gvm_core/internal/csl:libcsl',
           ],
)

