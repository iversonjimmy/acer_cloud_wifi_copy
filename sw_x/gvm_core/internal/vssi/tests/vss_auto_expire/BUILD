cpp_binary(
    name = 'vssExpireTest',
    includes = [
               '../common',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest.cpp',
            'vsTest_expire_data.cpp',
            'vsTest_expire_data.hpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
           ],
)

