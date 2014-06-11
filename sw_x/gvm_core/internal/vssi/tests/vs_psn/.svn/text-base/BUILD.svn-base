pseudo(
    name = 'vsTest_psn_all',
    deps = [
        ':vsTest_psn',
        ':vs_psn_dbeh',
    ]
)

cpp_binary(
    name = 'vsTest_psn',
    includes = [
               '../common',
               '../../src',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'vsTest_psn.cpp',
            'vsTest_personal_cloud_data.cpp',
            'vsTest_personal_cloud_data.hpp',
            'vsTest_vscs_async.cpp',
            'vsTest_vscs_async.hpp',
            'vsTest_async_notification_events.cpp',
            'vsTest_async_notification_events.hpp',
            '../common/vsTest_vscs_common.cpp',
            '../common/vsTest_infra.cpp',            
           ],
    deps = [
            '/gvm_core/internal/vssi:libvssi',
            '/gvm_core/vplex:libvplex',
           ],
)


custom(
    name = 'vs_psn_dbeh',
    input = [
        '%self.srcdir%/vsTest_dbeh.sh',
    ],
    output = [
        '%self.builddir%/vsTest_dbeh.sh',
    ],
    builddirs = [],
    recipe = [
        'cp -f %(in1)s $(PBROOT)/%(out1)s',
    ],
)
