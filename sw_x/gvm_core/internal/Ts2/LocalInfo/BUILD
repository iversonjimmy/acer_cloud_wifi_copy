static_cpp_lib(
    name = "libTs2LocalInfo",
    includes = [
        'include',
        '/gvm_core/daemons/ccd/src_impl',
        '/gvm_core/daemons/ccd/types/src',
        '/gvm_core/internal/media_metadata/common/include',
        '/gvm_core/internal/conf/include',
        '/es_core/include',
    ],
    private_includes = [
        '/gvm_core/internal/Ts2/include',
    ],
    srcs = [
    ],
    deps = [
    ],
)

static_cpp_lib(
    name = "libTs2LocalInfoFV",
    includes = [
        'include',
    ],
    private_includes = [
        '/gvm_core/internal/Ts2/include',
    ],
    srcs = [
        'src/LocalInfo_FixedValues.cpp',
    ],
    deps = [
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
        '/gvm_core/vplex:libvplex',
    ],
)

static_cpp_lib(
    name = "libTs2LocalInfoIH",
    includes = [
        'include',
        '/gvm_core/internal/Ts2/Link/tests/PxdTestInfraHelper',
    ],
    private_includes = [
        '/gvm_core/internal/Ts2/include',
    ],
    srcs = [
        'src/LocalInfo_InfraHelper.cpp',
        '../Link/tests/PxdTestInfraHelper/PxdTestInfraHelper.cpp',
    ],
    deps = [
        '/gvm_core/internal/error_codes:liberror_codes',
        '/gvm_core/internal/log:liblog',
        '/gvm_core/internal/pxd:libpxd',
        '/gvm_core/vplex:libvplex',
    ],
)

# Note there is no rule for LocalInfo_Ccd.
# This is because LocalInfo_Ccd.cpp must be compiled as part of libcc.
