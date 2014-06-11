static_cpp_lib(
    name = 'libmulticast_dns_discovery',
    includes = ['include'],
    private_includes = [
                        'platform/include',
                        'platform/linux/include',
                       ],
    srcs = [
            'src/1035.c',
            'src/mdd.c',
            'src/mdnsd.c',
            'src/sdtxt.c',
            'src/xht.c',
            'platform/linux/src/mdd_fdutil.c',
            'platform/linux/src/mdd_iface.c',
            'platform/linux/src/mdd_mutex.c',
            'platform/linux/src/mdd_net.c',
            'platform/linux/src/mdd_socket.c',
            'platform/linux/src/mdd_time.c',
            'platform/linux/src/mdd_utils.c',
           ],
    deps = [
            '/gvm_core/internal/log:liblog',
           ],
    cflags = '-D_GNU_SOURCE',
    cxxflags = '-D_GNU_SOURCE',
)
