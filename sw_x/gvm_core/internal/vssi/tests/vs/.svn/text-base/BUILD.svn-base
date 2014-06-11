cpp_binary(
    name = 'vsTest',
    includes = [
               '../common',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest.cpp',
            'vsTest_personal_cloud_data.cpp',
            'vsTest_personal_cloud_data.hpp',
            'vsTest_personal_cloud_http.cpp',
            'vsTest_personal_cloud_http.hpp',
            'vsTest_vscs_async.cpp',
            'vsTest_vscs_async.hpp',
            'vsTest_camera_roll.cpp',
            'vsTest_camera_roll.hpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
           ],
)

