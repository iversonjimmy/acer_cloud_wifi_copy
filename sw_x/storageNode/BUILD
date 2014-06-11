c_cxx_flags = ''

# TEMPORARY for TS2 development/integration
ts2_no_pxd = os.environ.get('TS2_NO_PXD', '1')
if ts2_no_pxd != '0':
    c_cxx_flags += '-DTS2_NO_PXD '

sn_includes = ['src']

sn_private_includes = [
    '/gvm_core/internal/vssi/src',
    '/gvm_core/daemons/ccd/include',
    '/gvm_core/internal/Ts2Alt/Ext/src',
]

sn_srcs = [
    'src/vss_query.cpp',
    'src/vss_server.cpp',
    'src/vss_cmdproc.cpp',
    'src/vss_client.cpp',
    'src/vss_session.cpp',
    'src/xid_sequence.cpp',
    'src/vss_object.cpp',
    'src/dataset.cpp',
    'src/managed_dataset.cpp',
    'src/fs_dataset.cpp',
    'src/vss_file.cpp',
    'src/vss_util.cpp',
    'src/vss_p2p_client.cpp',
    'src/vssts_srvr.cpp',
    'src/HttpSvc_Sn_Dispatcher.cpp',
    'src/HttpSvc_Sn_Handler.cpp',
    'src/HttpSvc_Sn_Handler_cmd.cpp',
    'src/HttpSvc_Sn_Handler_mediafile.cpp',
    'src/HttpSvc_Sn_Handler_minidms.cpp',
    'src/HttpSvc_Sn_Handler_mm.cpp',
    'src/HttpSvc_Sn_Handler_rexe.cpp',
    'src/HttpSvc_Sn_Handler_rf.cpp',
    'src/HttpSvc_Sn_Handler_vcs_archive.cpp',
    'src/HttpSvc_Sn_MediaFile.cpp',
    'src/HttpSvc_Sn_MediaDsFile.cpp',
    'src/HttpSvc_Sn_MediaFsFile.cpp',
    'src/HttpSvc_Sn_MediaFileSender.cpp',
    'src/HttpSvc_Sn_MediaMetadata.cpp',
    'src/HttpSvc_TsToHsAdapter.cpp',
    'src/strm_http.cpp',
    'src/echoSvc.cpp',
    'src/executable_manager.cpp',
    'src/HttpSvc_Sn_ASFileSender.cpp'
]

sn_deps = [
    '/gvm_core/internal/file_utils:libfile_utils',
    '/gvm_core/vplex:libvplex',
    '/gvm_core/internal/simple_http:libsimple_http',
    '/storageNode/datasetdb2:libdatasetdb',
    '/gvm_core/internal/csl:libcsl',
    '/gvm_core/internal/mini_ucf:libmini_ucf',
    '/gvm_core/daemons/ccd:libccd_types',
    '/gvm_core/internal/cJSON2:libcJSON2',
    '/gvm_core/internal/vssi:libvssi',
    '/gvm_core/internal/Stream:libStream',
] + ([
    '/gvm_core/internal/image_transcode:libimage_transcode',
    '/:libpthread',
    '/gvm_core/internal/db_utils:libdb_utils',
    ] if platform == 'linux' or platform == 'linux_emb' or platform == 'cloudnode' else []) + ([
    '/gvm_core/internal/Ts2:libTs2',
    '/gvm_core/internal/Ts2Alt/Ext:libTs2ExtServer',
    ])

# Note: When linking this library into an executable, you must choose the
#   appropriate CCDI implementation (either /gvm_core/daemons/ccd:libccd_client
#   or /gvm_core/daemons/ccd:libcc) and link that library as well.

static_cpp_lib(
    name = 'libStorageNode',
    cflags = c_cxx_flags,
    cxxflags = c_cxx_flags,
    includes = sn_includes,
    private_includes = sn_private_includes,
    srcs = sn_srcs,
    deps = sn_deps,
)

cloudnode_c_cxx_flags = '-DCLOUDNODE=1'

static_cpp_lib(
    name = 'libStorageNodeForCloudNode',
    cflags = c_cxx_flags + cloudnode_c_cxx_flags,
    cxxflags = c_cxx_flags + cloudnode_c_cxx_flags,
    includes = sn_includes,
    private_includes = sn_private_includes,
    srcs = sn_srcs,
    deps = sn_deps,
)
