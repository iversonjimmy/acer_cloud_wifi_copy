# TEMPORARY, for TS integration
c_cxx_flags = ''
stream_use_ts_wrapper = os.environ.get('STREAM_USE_TS_WRAPPER', '0')
c_cxx_flags += ' -DSTREAM_USE_TS_WRAPPER=' + stream_use_ts_wrapper + ' '

static_cpp_lib(
    name = 'libStream',
    cflags = c_cxx_flags,
    cxxflags = c_cxx_flags,
    private_includes = [
        ],
    includes = [
        'include',
        ],
    srcs = [
        'src/InStringStream.cpp',
        'src/OutStringStream.cpp',
        'src/SocketStream.cpp',
        'src/HttpStream.cpp',
        'src/HttpFileDownloadStream.cpp',
        'src/HttpFileUploadStream.cpp',
        'src/HttpSocketStream.cpp',
        'src/HttpStringStream.cpp',
        'src/HttpTsStream.cpp',
        'src/HttpReqHeader.cpp',
        'src/HttpRespHeader.cpp',
        'src/HttpStream_Helper.cpp',
        ],
    deps = [
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
        '/gvm_core/internal/simple_http:libsimple_http',
        '/gvm_core/internal/Ts2:libTs2',
        '/gvm_core/vplex:libvplex',
        ],
)
