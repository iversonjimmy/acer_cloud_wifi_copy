static_cpp_lib(
    name = 'libvsstswrapper',
    includes = [
        '/gvm_core/internal/vssts/include',
        '/gvm_core/internal/vssi/include'
        ],
    srcs = [
        'vssts_wrapper.cpp',
        'wrapper_glue.cpp'
        ],
    deps = [
        '/gvm_core/internal/vssi:libvssi',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/daemons/ccd:libccd_client',
        ],
)
