if platform == 'android':
    pseudo(
        name = 'syncConfigTest',    
        deps = [
            '/gvm_apps/syncConfigTest/platform_android:syncConfigTest',
        ],
    )
    static_cpp_lib(
        name = 'libsync_config_test',
        srcs = ['SyncConfigTest.cpp',
                'VcsTestSession.cpp',
                'VcsTestUtils.cpp'
        ],
        deps = [
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/db_utils:libdb_utils',
            '/gvm_core/internal/sync_config:libsync_config',
            '/gvm_core/internal/vcs:libvcs',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/protobuf_file_utils:libprotobuf_file_utils'
        ],
    )
elif platform == 'cloudnode':
    cpp_binary(
        name = 'syncConfigTest',
        cflags = '-DCLOUDNODE=1',
        cxxflags = '-DCLOUDNODE=1',
        srcs = ['SyncConfigTest.cpp',
                'VcsTestSession.cpp',
                'VcsTestUtils.cpp'
        ],
        deps = [
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/db_utils:libdb_utils',
            '/gvm_core/internal/sync_config:libsync_config',
            '/gvm_core/internal/vcs:libvcs',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/protobuf_file_utils:libprotobuf_file_utils'
        ],
    )
else:
    cpp_binary(
        name = 'syncConfigTest',
        srcs = ['SyncConfigTest.cpp',
                'VcsTestSession.cpp',
                'VcsTestUtils.cpp'
        ],
        deps = [
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/db_utils:libdb_utils',
            '/gvm_core/internal/sync_config:libsync_config',
            '/gvm_core/internal/vcs:libvcs',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/protobuf_file_utils:libprotobuf_file_utils'
        ],
    )
