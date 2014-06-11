pseudo(
    name = 'all',
    deps = [
        ':InStringStreamTester',
        ':OutStringStreamTester',
        ':HttpReqHeaderTester',
        ':HttpRespHeaderTester',
        ':HttpStringStreamTester',
        ':HttpFileUploadStreamTester',
        ],
)

cpp_binary(
    name = 'InStringStreamTester',
    srcs = ['src/InStringStreamTester.cpp'],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)

cpp_binary(
    name = 'OutStringStreamTester',
    srcs = ['src/OutStringStreamTester.cpp'],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)

cpp_binary(
    name = 'HttpReqHeaderTester',
    srcs = [
        'src/HttpReqHeaderTester.cpp',
        'src/SampleRequest.cpp',
        ],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)

cpp_binary(
    name = 'HttpRespHeaderTester',
    srcs = [
        'src/HttpRespHeaderTester.cpp',
        'src/SampleResponse.cpp',
        ],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)

cpp_binary(
    name = 'HttpStringStreamTester',
    srcs = [
        'src/HttpStringStreamTester.cpp',
        'src/SampleRequest.cpp',
        'src/SampleResponse.cpp',
        ],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)

cpp_binary(
    name = 'HttpFileUploadStreamTester',
    srcs = [
        'src/HttpFileUploadStreamTester.cpp',
        'src/SampleRequest.cpp',
        'src/SampleResponse.cpp',
        ],
    deps = [
        '/gvm_core/internal/Stream:libStream',
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
	],
)
