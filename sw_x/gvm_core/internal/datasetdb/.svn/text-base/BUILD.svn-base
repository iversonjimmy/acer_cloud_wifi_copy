static_cpp_lib(
    name = "libdatasetdb",
    includes = ['include'],
    srcs = [
            'src/DatasetDB.cpp',
            'src/unicode_upcase_table.cpp',
            'src/utf8.cpp',
           ],
    deps = [
            '/gvm_core/vplex:libvplex',
            '/gvm_core/external/sqlite3:sqlite3',
           ]
)
