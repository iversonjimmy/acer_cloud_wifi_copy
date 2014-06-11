static_cpp_lib(
    name = 'libInterruptibleSocket',
    private_includes = [
        ],
    includes = [
        'include',
        ],
    srcs = [
        'src/InterruptibleSocket.cpp',
        'src/InterruptibleSocket_SigBySockShutdown.cpp',
        'src/InterruptibleSocket_SigByCmdPipe.cpp',
        'src/InterruptibleSocket_SigByCmdSocket.cpp',
        ],
    deps = [
        '/gvm_core/internal/log:liblog',
        '/gvm_core/vplex:libvplex',
        ],
)

