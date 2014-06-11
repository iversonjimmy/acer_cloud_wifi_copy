pseudo(
	name = 'tests',
	deps = [
		':ccd_link_device',
		':ccd_list_storage_nodes',
		':ccd_login',
		':ccd_ready_test',
		':ccd_register_storage_node',
		':ccd_shutdown_test',
		':TestSyncUpJobs',
		':TestSyncDownJobs',
#		':TestHttpSvcListener',
	],
)

static_cpp_lib(
	name = 'libccdi_test_common',
	includes = ['include'],
	srcs = [
		'src/ccdi_test_common.cpp',
	],
	deps = [
		'/gvm_core/daemons/ccd:libccd_client',
		'/gvm_core/vplex:libvplex',
	],
)

cpp_binary(
	name = 'ccd_link_device',
	srcs = ['src/ccd_link_device.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'ccd_list_storage_nodes',
	srcs = ['src/ccd_list_storage_nodes.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'ccd_login',
	srcs = ['src/ccd_login.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'ccd_ready_test',
	srcs = ['src/ccd_ready_main.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'ccd_register_storage_node',
	srcs = ['src/ccd_register_storage_node.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'ccd_shutdown_test',
	srcs = ['src/ccd_shutdown_main.cpp'],
	deps = [
		':libccdi_test_common',
	],
)

cpp_binary(
	name = 'TestSyncUpJobs',
	srcs = ['src/TestSyncUpJobs.cpp'],
        includes = [
                '/gvm_core/daemons/ccd/src_impl',
        ],
	deps = [
                '/gvm_core/daemons/ccd:libcc',
                '/gvm_core/vplex:libvplex',
	],
)

cpp_binary(
	name = 'TestSyncDownJobs',
	srcs = ['src/TestSyncDownJobs.cpp'],
        includes = [
                '/gvm_core/daemons/ccd/src_impl',
        ],
	deps = [
                '/gvm_core/daemons/ccd:libcc',
                '/gvm_core/vplex:libvplex',
	],
)

cpp_binary(
	name = 'TestHttpSvcListener',
	srcs = ['src/TestHttpSvcListener.cpp'],
        includes = [
                '/gvm_core/daemons/ccd/src_impl',
        ],
	deps = [
                '/gvm_core/daemons/ccd:libcc',
                '/gvm_core/vplex:libvplex',
	],
)
