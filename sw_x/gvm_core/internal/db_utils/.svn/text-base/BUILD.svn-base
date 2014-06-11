static_cpp_lib(
    name = 'libdb_utils',
    includes = ['include'],
    srcs = [
            'src/db_util_check_macros.cpp',
            'src/util_open_db_handle.cpp',
           ],
    deps = [
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/vplex:libvplex',
            '/gvm_core/internal/file_utils:libfile_utils',
            '/gvm_core/internal/log:liblog',
            '/gvm_core/internal/error_codes:liberror_codes',
            '/gvm_core/external/sqlite3:sqlite3',
           ],
)
