cpp_binary(
    name = 'vsStreamTest',
    includes = [
               '../common',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest.cpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
           ],
)

