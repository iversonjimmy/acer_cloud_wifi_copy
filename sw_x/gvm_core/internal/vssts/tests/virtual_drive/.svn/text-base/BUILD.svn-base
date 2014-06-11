cpp_binary(
    name = 'virtual_drive',
    includes = [
               '../common',
               '/gvm_core/daemons/ccd/include',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest.cpp',
            'vsTest_virtual_drive_test.cpp',
            'vsTest_virtual_drive_test.hpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssts/shim:libvsstsshim',
            '/gvm_core/vplex:libvplex',
            '/gvm_core/daemons/ccd:libccd_client',
           ],
)

