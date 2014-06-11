cpp_binary(
    name = 'imltool',
    includes = [
                   'include',
                   '/gvm_core/internal/vssi/include',
                   '/gvm_core/internal/vssi/src',
                   'common/src',
                   '/gvm_core/daemons/ccd/include',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'src/imltool.cpp',
           ],
    deps = [
            '/storageNode:libStorageNodeForCloudNode',
            '/storageNode:libStorageNode',
           ],
)


