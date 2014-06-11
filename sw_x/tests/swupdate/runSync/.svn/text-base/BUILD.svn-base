pseudo (
    name = 'runSync',
    deps = [ '/tests/swupdate/runSync:ccdSync',
             '/tests/swupdate/runSync:ccdLogin',
             '/tests/swupdate/runSync:ccdLogout',
             '/tests/swupdate/runSync:swuTest',
             '/tests/swupdate/runSync:swuTool',
           ]
)
        
cpp_binary(
    name = 'ccdSync', 
    srcs = ['ccdSync.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdLogin', 
    srcs = ['ccdLogin.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdLogout', 
    srcs = ['ccdLogout.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'swuTest', 
    srcs = ['swuTest.cpp'],
    deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'swuTool', 
    srcs = ['swuTool.cpp'],
    deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/daemons/ccd:libccd_client',
    ]
)
