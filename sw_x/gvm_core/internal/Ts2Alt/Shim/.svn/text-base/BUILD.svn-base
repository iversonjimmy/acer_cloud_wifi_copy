static_cpp_lib(
    name = 'libTs2Shim',
    includes = [
        '/gvm_core/internal/Ts2/include',
    ],
    private_includes = [
        '/gvm_core/daemons/ccd/include',
    ],
    srcs = [
        'src/ts_shim.cpp',
    ],
    deps = [
        '/gvm_core/internal/Ts2Alt/Wrapper:libTs2Wrapper',
        '/gvm_core/internal/Ts2:libTs2',
    ],
)
