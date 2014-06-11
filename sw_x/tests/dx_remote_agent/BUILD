cpp_binary(
    name = 'dx_remote_agent',
    srcs = [
        'dx_remote_agent.cpp',
        '../dxshell/types/dx_remote_agent.pb.cc'
        ],
    deps = [
        '/tests/dxshell/types:dx_remote_agent_proto',
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/internal/csl:libcsl'
    ]
)
