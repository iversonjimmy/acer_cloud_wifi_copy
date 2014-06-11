static_cpp_lib(
    name = 'libvsstsshim',
    includes = [
        '/gvm_core/internal/vssts/include'
        ],
    srcs = [
        'vssts_shim.cpp',
        ],
    deps = [
        '/gvm_core/internal/vssts/wrapper:libvsstswrapper',
        '/gvm_core/internal/vssts/:libvssts',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/vplex:libvplex',
        ],
)
