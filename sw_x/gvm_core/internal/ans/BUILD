static_cpp_lib(
    name = 'libans',
    includes = ['include'],
    srcs = ['src/ans_device.cpp'],
    deps = [
            '/gvm_core/internal/log:liblog',
            '/gvm_core/vplex:libvplexcore',
            '/gvm_core/internal/csl:libcsl',
           ],
)

cpp_binary(
    name = 'ans_device_test',
    srcs = [
            'src/ans_device_test.cpp',
           ],
    deps = [
            ':libans',
           ]
)
