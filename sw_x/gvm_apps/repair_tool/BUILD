cpp_binary(
    name = 'repair_tool',
    includes = [
                   'include',
                   '/gvm_core/internal/vssi/include',
                   '/gvm_core/internal/vssi/src',
                   'common/src',
                   '/gvm_core/daemons/ccd/include',
               ],
    cxxflags = '-Wno-missing-field-initializers ',
    srcs = [
            'src/repair_tool.cpp',
           ],
    deps = [
            '/storageNode:libStorageNodeForCloudNode',
            '/storageNode:libStorageNode',
           ],
)


