pseudo (
    name = 'test_utils',
    deps = [ '/tests/test_utils:ccdSync',
             '/tests/test_utils:ccdLogin',
             '/tests/test_utils:ccdLogout',
             '/tests/test_utils:ccdStop',
             '/tests/test_utils:ccdWaitUntilInSync',
             '/tests/test_utils:linkDevice',
             '/tests/test_utils:unlinkDevice',
             '/tests/test_utils:subscribe',
             '/tests/test_utils:subscribeMobileCam',
             '/tests/test_utils:registerStorageNode',
             '/tests/test_utils:listStorageNodes',
             '/tests/test_utils:addSyncSubscription',
             '/tests/test_utils:linkedDevices',
             '/tests/test_utils:CallGetLocalHttpInfo',
             '/tests/test_utils:vsListOwnedDataSets',
             '/tests/test_utils:UploadToPicstream',
             '/tests/test_utils:ConfigPicstream',
             '/tests/test_utils:GetAcerCloudAccessTokens',
           ]
)
        
cpp_binary(
    name = 'ccdSync', 
    srcs = ['ccdSync.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdLogin', 
    srcs = ['ccdLogin.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdLogout', 
    srcs = ['ccdLogout.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdStop',
    srcs = ['ccdStop.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'ccdWaitUntilInSync',
    srcs = ['ccdWaitUntilInSync.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'linkDevice',
    srcs = ['linkDevice.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'unlinkDevice',
    srcs = ['unlinkDevice.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'subscribe',
    srcs = ['subscribe.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'subscribeMobileCam',
    srcs = ['subscribeMobileCam.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ]
)

cpp_binary(
    name = 'importToDataset',
    srcs = ['importToDataset.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'exportFromDataset',
    srcs = ['exportFromDataset.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'registerStorageNode',
    srcs = ['registerStorageNode.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'listStorageNodes',
    srcs = ['listStorageNodes.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'addSyncSubscription',
    srcs = ['addSyncSubscription.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'linkedDevices',
    srcs = ['linkedDevices.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/csl:libcsl',
    ],
)

cpp_binary(
    name = 'CallGetLocalHttpInfo',
    srcs = ['CallGetLocalHttpInfo.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
    ],
)

cpp_binary(
    name = 'vsListOwnedDataSets',
    srcs = ['vsListOwnedDataSets.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/csl:libcsl',
    ],
)

cpp_binary(
    name = 'UploadToPicstream',
    srcs = ['UploadToPicstream.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/csl:libcsl',
    ],
)

cpp_binary(
    name = 'ConfigPicstream',
    srcs = ['ConfigPicstream.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/csl:libcsl',
    ],
)

cpp_binary(
    name = 'GetAcerCloudAccessTokens',
    srcs = ['GetAcerCloudAccessTokens.cpp'],
    deps = [
        '/gvm_core/daemons/ccd:libccd_client',
        '/gvm_core/vplex:libvplex',
        '/gvm_core/internal/csl:libcsl',
        '/gvm_core/internal/file_utils:libfile_utils',
    ],
)
