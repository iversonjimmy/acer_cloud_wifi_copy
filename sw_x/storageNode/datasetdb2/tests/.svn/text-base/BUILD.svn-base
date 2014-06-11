cpp_binary(
    name = 'TestDatasetDB',
    includes = [
        '../include',
#        '../../../gvm_core/external/sqlite3'
    ],
    cxxflags = '-DTESTDATASETDB',
    srcs = [
        '../src/DatasetDB.cpp',
        'TestDatasetDB.cpp', 
        'Test.cpp', 
        'TestRun.cpp', 
        'utils.cpp',
        'TestOpenClose.cpp',
        'TestCreateComponent.cpp',
        'TestComponentSize.cpp',
        'TestComponentPath.cpp',
        'TestComponentPermission.cpp',
        'TestComponentVersion.cpp',
        'TestComponentMTime.cpp',
        'TestListComponents.cpp',
        'TestBigDB.cpp',
        'TestTransaction.cpp',
        'TestMoveComponent.cpp',
        'TestDatasetVersion.cpp',
#        'TestDBConstraint.cpp',
    ],
    deps = [
        '/storageNode/datasetdb2:libdatasetdb',
        '/gvm_core/vplex:libvplex',
    ]
)
