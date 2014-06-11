cpp_binary(
    name = 'protorpcgen',
    srcs = [
        'GeneratorUtils.cpp',
        'main.cpp',
        'ServiceGenerator.cpp',
        'WsdlGenerator.cpp',
        'XmlUtilsGenerator.cpp',
        'XsdGenerator.cpp',
    ],
    deps = [
            '/gvm_core/external/protobuf:libprotobuf',
            '/gvm_core/external/protobuf:libprotoc',
           ]
           + {
              'android':[
                        ],
              'cygwin':[
                    ],
              'linux':[
                       ':libpthread', #GoogleOnceInit uses pthread_once
                      ],
              'linux_emb':[
                       ':libpthread',
                      ],
              'cloudnode':[
                       ':libpthread',
                      ],
              'mac':[
                    ],
              'gvm_rt':[
                        ':libpthread',
                       ],
              'win':[
                    ],
              }[platform],
    cxxflags = '-Wno-sign-compare',
)

# libpthread is copied here from sw_x/BUILD so we don't pull in that file
# to make devtools.
if platform == 'linux':
    if host_platform_sub == 'centos':
        system_shared_cpp_lib(
            name = 'libpthread',
            file = '/usr/lib/libpthread.so'
        )
    else:
        system_shared_cpp_lib(
            name = 'libpthread',
            file = {'i686':   '/usr/lib/libpthread.so',
                    'x86_64': '/usr/lib32/libpthread.so'
                   }[os.uname()[4]]
        )

elif platform == 'linux_emb':
    if host_platform_sub == 'centos':
        system_shared_cpp_lib(
            name = 'libpthread',
            file = '/usr/lib/libpthread.so'
        )
    else:
        system_shared_cpp_lib(
            name = 'libpthread',
            file = {'i686':   '/usr/lib/libpthread.so',
                    'x86_64': '/usr/lib32/libpthread.so'
                   }[os.uname()[4]]
        )

elif platform == 'cloudnode':
    system_shared_cpp_lib(
        name = 'libpthread',
        file = {'i686':   '/usr/lib/libpthread.so',
                'x86_64': '/usr/lib32/libpthread.so'
               }[os.uname()[4]]
    )
