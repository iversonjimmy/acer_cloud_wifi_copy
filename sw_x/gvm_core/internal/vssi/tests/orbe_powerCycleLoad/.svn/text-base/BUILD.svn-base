cpp_binary(
    name = 'orbe_pcl',
    includes = [
               '../common',
               '../../src',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'orbe_pcl.cpp',
            'vsTest_vscs_async.cpp',
            'vsTest_vscs_async.hpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
           ],
)

