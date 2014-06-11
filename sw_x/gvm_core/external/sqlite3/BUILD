static_cpp_lib(
    name = 'sqlite3',
    includes = ['.'],
    srcs = [
            'sqlite3.c',         
           ],
#    deps = [
#           ]
#           + {
#              'android':[
#                         '/:libstlport_static',
#                         '/:libstdc++',
#                         '/:libc',
#                        ],
#              'linux':[
#                      ],
#              'gvm_rt':[
#                       ],
#              'win':[
#                       ],
#              }[platform],

    cflags = ('-Wno-undef -Wno-write-strings -Wno-float-equal -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_USLEEP=1 -DHAVE_GMTIME_R=1 -DHAVE_STRERROR_R=1 -D_REENTRANT=1 -DSQLITE_THREADSAFE=1 '
              + {
                'android':'-Dfdatasync=fsync -DHAVE_LOCALTIME_R=1 ',
                'linux':'-DHAVE_POSIX_FALLOCATE=1 -DHAVE_LOCALTIME_R=1 ',
                'linux_emb':'-DHAVE_POSIX_FALLOCATE=0 -DHAVE_LOCALTIME_R=1 ',
                'cloudnode':'-DHAVE_POSIX_FALLOCATE=1 -DHAVE_LOCALTIME_R=1 ',
                'win':'-DHAVE_POSIX_FALLOCATE=1 -Wno-error ',
              }[platform]
              + ('-Wno-strict-overflow ' if host_platform_sub != 'redhat' else '')
             )
)

# Compile options:  http://www.sqlite.org/compile.html
# http://www.sqlite.org/threadsafe.html
# http://www.sqlite.org/rtree.html -- not needed
# http://www.sqlite.org/fts3.html  -- not needed
