static_cpp_lib(
    name = 'liblog',
    includes = ['include'],
    srcs = [
            'src/log.cpp',
           ],
    deps = [
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/vplex:libvplexcore',
            '/gvm_core/internal/error_codes:liberror_codes',
           ] + ([
                 '/:liblog',
                 '/:libstlport_static',
                 '/:libstdc++',
                ] if platform == 'android' else []),
)
