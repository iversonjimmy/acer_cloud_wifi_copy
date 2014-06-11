# This is for building just the protobuf *runtime* library.
# We need to build this for every *target* platform.
# (See "../protobuf" for building the entire protobuf package, which includes the compiler.)

static_cpp_lib(
    name = 'protobuflib',
    includes = ['.'],
    srcs = [
            'google/protobuf/stubs/common.cc',         
            'google/protobuf/stubs/once.cc',           
            'google/protobuf/extension_set.cc',        
            'google/protobuf/generated_message_util.cc',
            'google/protobuf/message_lite.cc',         
            'google/protobuf/repeated_field.cc',       
            'google/protobuf/wire_format_lite.cc',     
            'google/protobuf/io/coded_stream.cc',      
            'google/protobuf/io/zero_copy_stream.cc',  
            'google/protobuf/io/zero_copy_stream_impl_lite.cc',
            'google/protobuf/stubs/strutil.cc',
            'google/protobuf/stubs/substitute.cc',                 
            'google/protobuf/stubs/structurally_valid.cc',         
            'google/protobuf/descriptor.cc',                       
            'google/protobuf/descriptor.pb.cc',                    
            'google/protobuf/descriptor_database.cc',              
            'google/protobuf/dynamic_message.cc',                  
            'google/protobuf/extension_set_heavy.cc',              
            'google/protobuf/generated_message_reflection.cc',     
            'google/protobuf/message.cc',                          
            'google/protobuf/reflection_ops.cc',                   
            'google/protobuf/service.cc',                          
            'google/protobuf/text_format.cc',                      
            'google/protobuf/unknown_field_set.cc',                
            'google/protobuf/wire_format.cc',                      
            'google/protobuf/io/gzip_stream.cc',                   
            'google/protobuf/io/printer.cc',                       
            'google/protobuf/io/tokenizer.cc',                     
            'google/protobuf/io/zero_copy_stream_impl.cc',         
           ],
    deps = [
           ]
           + {
              'android':[
                         '/:libstlport_static',
                         '/:libstdc++',
                         '/:libc',
                        ],
              'linux':[
                      ],
			  'linux_emb':[
                          ],
              'cloudnode':[
                      ],
              'gvm_rt':[
                       ],
              'win':[
                       ],
              }[platform],
    # The older g++ compiler on Red Hat chokes on this code.
    cxxflags = ('-Wno-missing-declarations ' if host_platform_sub != 'redhat' else '-Wno-error ') + \
               '-Wno-switch-enum -Wno-float-equal -Wno-undef',
)

shared_cpp_lib(
    name = 'protobuflib-shr',
    srcs = [],
    deps = [':protobuflib'],
    soname = 'libprotobuf.so.1'
)
