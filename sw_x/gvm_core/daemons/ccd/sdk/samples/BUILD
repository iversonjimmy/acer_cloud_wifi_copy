pseudo(
    name = 'all',
    deps = [
        ':logout',
        ':setup_sync_agent',
    ],
)

cpp_binary(
    name = 'logout',
    srcs = ['src/logout.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'setup_sync_agent',
    srcs = ['src/setup_sync_agent.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)
