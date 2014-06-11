static_cpp_lib(
    name = 'libvcs',
    includes = ['include'],
    srcs = ['src/vcs_file_url_access_basic_impl.cpp',
            'src/vcs_file_url_operation_http.cpp',
            'src/vcs_common.cpp',
            'src/vcs_common_priv.cpp',
            'src/vcs_defs.cpp',
            'src/vcs_v1_util.cpp',
            'src/vcs_share_util.cpp',
            'src/vcs_util.cpp'
           ],
    deps = [
            '/gvm_core/internal/log:liblog',
            '/gvm_core/vplex:libvplexcore',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/cJSON2:libcJSON2',
            '/gvm_core/internal/file_utils:libfile_utils'
           ],
)

