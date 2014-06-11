
static_cpp_lib(
    name = 'libesc_ul',
    includes = ['include'],
    private_includes = [
                        'core/base/include',
                        'core/base/ioscrypto',
                        'core/base/es',
                        'core/base/esi',
                        'core/ioscrypto',
                        'core/ec',
                        'core/es'
                       ],
    cflags = '-D_USER_LEVEL -DES_DEBUG=1 -DEC_DEBUG=1 -DGVM_FEAT_GHV_DBG=1 -Wno-missing-field-initializers ',
    srcs = [
            'core/ec/core_glue.c',
            'core/ec/ec_cache.c',
            'core/ec/ec_core.c',
            'core/ec/ec_pers.c',
            'core/ec/istorage_impl.c',
            'core/es/es_container.c',
            'core/es/es_device.c',
            'core/es/es_storage.c',
            'core/ioscrypto/crypto_impl.c',
            'core/ioscrypto/mt19937-64.c',
            'core/base/esi/esi_core.c',
            'core/base/ioscrypto/bsl.c',
            'core/base/ioscrypto/keystore.c',
            'core/base/ioscrypto/keyglobals.c',
            'core/base/ioscrypto/wrapper.c',
           ],
    deps = [
            '/gvm_core/vpl:libvplcore',
            '/gvm_core/internal/csl:libcsl',
            '/gvm_core/internal/log:liblog',
           ]
)
