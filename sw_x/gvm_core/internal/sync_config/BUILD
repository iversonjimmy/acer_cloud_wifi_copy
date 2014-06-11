static_cpp_lib(
    name = 'libsync_config',
    includes = ['include'],
    srcs = [
            'src/FileMonitorDelayQ.cpp',
            'src/SyncConfig.cpp',
            'src/SyncConfigDb.cpp',
            'src/SyncConfigThreadPool.cpp',
            'src/SyncConfigUtil.cpp',
           ],
    deps = [
            '/gvm_core/external/sqlite3:sqlite3',
            '/gvm_core/internal/error_codes:liberror_codes',
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/protobuf_file_utils:libprotobuf_file_utils',
            '/gvm_core/internal/db_utils:libdb_utils',
            '/gvm_core/internal/log:liblog',
            '/gvm_core/internal/vcs:libvcs',
            '/gvm_core/vplex:libvplex',
           ],
)
