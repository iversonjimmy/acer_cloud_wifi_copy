if es_is_kernel_module:
    # Build as stubs for kernel calls.
    static_cpp_lib(
        name = 'libescore',
        includes = ['include'],
        private_includes = [
                            'kernel/include',
                            'esc/include',
                           ],
        cflags = '-D_GSS_SUPPORT',
        srcs = [
                'user_kernel/escore_base.c'
               ],
        deps = [
                '/gvm_core/vpl:libvpl_minimal',
               ],
    )
else:
    # Build as user-land library.
    static_cpp_lib(
        name = 'libescore',
        includes = ['include'],
        private_includes = ['esc/include'],
        cflags = '-D_USER_LEVEL -DESCORE_IS_REENTRANT -DES_DEBUG=1 -DEC_DEBUG=1 -DGVM_FEAT_GHV_DBG=1',
        srcs = [
                'user_local/escore_base.c'
               ],
        deps = [
                '/gvm_core/vpl:libvplcore',
                '/es_core/esc:libesc_ul'
               ],
    )
