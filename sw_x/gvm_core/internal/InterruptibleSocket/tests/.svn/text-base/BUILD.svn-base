cpp_binary(
    name = "FreeBlockedReadTest",
    srcs = [
        'FreeBlockedReadTest.cpp',
        'ConnectedSocketPair.cpp',
        ],
    deps = [
        '/gvm_core/internal/InterruptibleSocket:libInterruptibleSocket',
        '/gvm_core/internal/file_utils:libfile_utils',
        ],
)

cpp_binary(
    name = "SimpleIoTest",
    srcs = [
        'SimpleIoTest.cpp',
        'ConnectedSocketPair.cpp',
        ],
    deps = [
        '/gvm_core/internal/InterruptibleSocket:libInterruptibleSocket',
        ],
)

pseudo(
    name = 'tests',
    deps = [
        '/gvm_core/internal/InterruptibleSocket/tests:FreeBlockedReadTest',
        '/gvm_core/internal/InterruptibleSocket/tests:SimpleIoTest',
        ],
)
