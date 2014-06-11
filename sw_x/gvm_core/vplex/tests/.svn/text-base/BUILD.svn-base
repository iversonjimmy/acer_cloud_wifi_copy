if platform == 'android':
    pseudo(
        name = 'vplexTest',    
        deps = [
            '/gvm_apps/vplexTests/platform_android:vplexTest',
        ],
    )

    static_cpp_lib(
        name = 'libvplextest',
        srcs = [
            '../../vpl/tests/src/vplTest_common.c',
            'src/vplexTest_common.c',
            'src/vplexTest.cpp',
            'src/vplexTest_http2.cpp',
            'src/vplexTest_math.c',
            'src/vplexTest_mem_heap.c',
            'src/vplexTest_safe_serialization.c',
            'src/vplexTest_serialization.c',
            'src/vplexTest_wstring.c',
            'src/vplexTest_xml_reader.c',
            'src/vplexTest_xml_writer.c',
        ],
        includes = ['include','../../vpl/tests/include'],
        deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/internal/iosc:libiosc',
            '/gvm_core/internal/ans:libans',
            '/:libc',
            '/:liblog',
        ],
    )

else:
    cpp_binary(
        name = 'vplexTest',
        srcs = [
            '../../vpl/tests/src/vplTest_common.c',
            'src/vplexTest_common.c',
            'src/vplexTest_math.c',
            'src/vplexTest_mem_heap.c',
            'src/vplexTest_safe_serialization.c',
            'src/vplexTest_serialization.c',
            'src/vplexTest_wstring.c',
            'src/vplexTest_xml_reader.c',
            'src/vplexTest_xml_writer.c',
            'src/vplexTest.cpp',
            'src/vplexTest_http2.cpp',
        ],
        includes = ['include','../../vpl/tests/include'],
        deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/internal/iosc:libiosc',
            '/gvm_core/internal/ans:libans',
        ]
    )
