# This module should be renamed to gvm_utils
static_cpp_lib(
    name = 'libfile_utils',
    includes = ['include'],
    srcs = [
            'src/util_cleanuppath.cpp',
            'src/util_createpath.c',
            'src/util_encodebase64.c',
            'src/util_mime.cpp',
            'src/util_parsestricthex64.c',
            'src/util_readfile.c',
            'src/util_rm_recursive.cpp',
            'src/util_spawnthread.c',
            'src/util_writefile.c',
            'src/util_utf8.cpp',
           ],
    deps = [
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/vplex:libvplex',
            '/gvm_core/internal/log:liblog',
            '/gvm_core/internal/error_codes:liberror_codes',
           ],
)
