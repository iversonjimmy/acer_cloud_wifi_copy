static_cpp_lib(
    name = 'libfattr_db',
    includes = ['include'],
    srcs = [
            'src/dataset_db.cpp',
           ],
    deps = [
            '/:libsqlite3',
            # VPL for vplu_types.h, could be optimized better
           '/gvm_core/vpl:libvplcore'
          ],
)
