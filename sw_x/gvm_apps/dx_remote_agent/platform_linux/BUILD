
cpp_binary(
    name = 'dx_remote_agent' + ('_cloudnode' if platform == 'cloudnode' else ''),
    includes = [
                '.',
                '../../../tests/dx_remote_agent',
                '../../../tests/dxshell',
                '/storageNode/src',
                '/gvm_core/internal/Ts2Alt/Ext/src',
                '/gvm_core/internal/Ts2/include',
                '/gvm_core/internal/Ts2/LocalInfo/include',
               ],
    cflags = '-DCLOUDNODE=1' if platform == 'cloudnode' else '',
    cxxflags = '-DCLOUDNODE=1' if platform == 'cloudnode' else '',
    srcs = [
        'dx_remote_agent_linux.cpp',
        'dx_remote_agent_util_linux.cpp',
        '../../../tests/dxshell/types/dx_remote_agent.pb.cc',
        '../../../tests/dxshell/common_utils.cpp',
        '../../../tests/dxshell/HttpAgent.cpp',
        '../../../tests/dxshell/HttpAgentLocal.cpp',
        '../../../tests/dxshell/HttpAgentRemote.cpp',
        '../../../tests/dxshell/ts_testClient.cpp',
        '../../../tests/dxshell/RemoteAgent.cpp',
        '../../../tests/dx_remote_agent/dx_remote_agent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteAgentListener.cpp',
        '../../../tests/dx_remote_agent/DxRemoteAgentManager.cpp',
        '../../../tests/dx_remote_agent/DxRemoteCCDAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteFileTransferAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteHTTPAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteMSAAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteOSAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteQueryDeviceAgent.cpp',
        '../../../tests/dx_remote_agent/DxRemoteTSTestAgent.cpp',
        '../../../tests/dx_remote_agent/IDxRemoteAgent.cpp'
        ],
    deps = [
        '/tests/dxshell/types:dx_remote_agent_proto',
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/internal/file_utils:libfile_utils',
        '/gvm_core/internal/csl:libcsl',
        '/gvm_core/internal/Ts2Alt/Ext:libTs2ExtClient',
    ]
)
