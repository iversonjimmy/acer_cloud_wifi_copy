protobuf(
    name = 'proto_src',
    protofile = 'rpc.proto',
    cpp_out = '.',
    java_out = 'java',
    #csharp_out = ('csharp' if platform == 'msvc' else None),
)

if platform != 'ios' and platform != 'msvc':
    static_cpp_lib(
        name = 'protorpc',
        srcs = [
            'rpc.pb.cc',
            'ByteArrayProtoChannel.cpp',
            'FileProtoChannel.cpp',
            'ProtoChannel.cpp',
            'ProtoRpc.cpp',
            'ProtoRpcClient.cpp',
            'ProtoRpcClientAsync.cpp',
        ],
        includes = [''],
        deps = [
                ':proto_src',
                '/gvm_core/vplex:libvplexcore',
               ] +
               (['/gvm_core/external/protobuflib:protobuflib-shr'] if product == 'cloudnode' else ['/gvm_core/external/protobuflib:protobuflib']),
        cxxflags = '-Wno-sign-compare',
    )
