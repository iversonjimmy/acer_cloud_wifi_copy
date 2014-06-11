protobuf(
    name = 'ans_message_src',
    protofile = 'vplex_ans_message.proto',
    cpp_out = '.',
)

protobuf(
    name = 'common_types_src',
    protofile = 'vplex_common_types.proto',
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'community_notifier_src',
    protofile = 'vplex_community_notifier.proto',
    deps = [':community_types_src'],
    cpp_out = '.',
)

protobuf(
    name = 'community_service_types_src',
    protofile = 'vplex_community_service_types.proto',
    deps = [':community_types_src'],
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'community_types_src',
    protofile = 'vplex_community_types.proto',
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'ias_service_types_src',
    protofile = 'vplex_ias_service_types.proto',
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'nus_service_types_src',
    protofile = 'vplex_nus_service_types.proto',
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'sync_agent_notifier_src',
    protofile = 'vplex_sync_agent_notifier.proto',
    cpp_out = '.',
)

protobuf(
    name = 'vs_directory_service_types_jar_src',
    input_path = 'sdk',
    protofile = 'vplex_vs_directory_service_types.proto',
    deps = [':common_types_src'],
    java_out = 'java',
)

protobuf(
    name = 'vs_directory_service_types_src',
    protofile = 'vplex_vs_directory_service_types.proto',
    deps = [':common_types_src'],
    cpp_out = '.',
    xmlu_out = '.',
)

protobuf(
    name = 'shared_credential_src',
    protofile = 'vplex_shared_credential.proto',
    cpp_out = '.',
)

if platform != 'ios' and platform != 'msvc':
    static_cpp_lib(
        name = 'proto',
        srcs = [
            'vplex_ans_message.pb.cc',
            'vplex_common_types.pb.cc',
            'vplex_community_notifier.pb.cc',
            'vplex_community_service_types.pb.cc',
            'vplex_community_types.pb.cc',
            'vplex_ias_service_types.pb.cc',
            'vplex_nus_service_types.pb.cc',
            'vplex_sync_agent_notifier.pb.cc',
            'vplex_vs_directory_service_types.pb.cc',
            'vplex_shared_credential.pb.cc',
        ],
        includes = [''],
        deps = [
            ':ans_message_src',
            ':common_types_src',
            ':community_notifier_src',
            ':community_service_types_src',
            ':community_types_src',
            ':ias_service_types_src',
            ':nus_service_types_src',
            ':sync_agent_notifier_src',
            ':vs_directory_service_types_src',
            ':shared_credential_src',
            '/gvm_core/internal/protorpc:protorpc',
        ],
        cxxflags = '-Wno-sign-compare',
    )
    
    static_cpp_lib(
        name = 'xml',
        srcs = [
            'vplex_common_types-xml.pb.cc',
            'vplex_community_service_types-xml.pb.cc',
            'vplex_community_types-xml.pb.cc',
            'vplex_ias_service_types-xml.pb.cc',
            'vplex_nus_service_types-xml.pb.cc',
            'vplex_vs_directory_service_types-xml.pb.cc',
        ],
        includes = [''],
        deps = [
            ':common_types_src',
            ':community_service_types_src',
            ':community_types_src',
            ':ias_service_types_src',
            ':nus_service_types_src',
            ':vs_directory_service_types_src',
            ':proto',
            '/gvm_core/internal/protoxml',
        ],
        cxxflags = '-Wno-sign-compare'
    )
