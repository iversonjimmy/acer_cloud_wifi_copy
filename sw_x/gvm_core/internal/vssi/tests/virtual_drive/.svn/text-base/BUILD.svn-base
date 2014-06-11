cpp_binary(
    name = 'virtual_drive_test',
    includes = [
               '../common',
               '/gvm_core/daemons/ccd/include',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest.cpp',
            'vsTest_virtual_drive_test.cpp',
            'vsTest_virtual_drive_test.hpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
            '/gvm_core/daemons/ccd:libccd_client',
           ],
)

