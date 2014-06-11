# This file is meant for items that control internal behavior of makegen but do not
# significantly affect the generated Makefile.  More precisely, build outputs do
# not depend on this file so it may be updated without forcing rebuilds.  If changing
# a part of makegen may cause something to need to be rebuilt in a way that is not
# detected through the usual prerequisites, it does not belong in this file.

# All target types.
# Okay to be here as it is only updated when new types are added, and BUILD files need to be
#   updated to make use of the new type.
all_types = [
    'android_app',
    'cpp_binary',
    'custom',
    'external_binary',
    'external_java_lib',
    'external_phony',
    'external_shared_cpp_lib',
    'external_static_cpp_lib',
    'kernel_module',
    'protobuf',
    'pseudo',
    'shared_cpp_lib',
    'static_cpp_lib',
    'system_shared_cpp_lib',
    'system_static_cpp_lib',
]

# What types of targets a target may depend on.
# Okay to be here as it only affects internal constraints and not generated Makefile.
allowed_deps = {
    'static_cpp_lib': ['static_cpp_lib', 'shared_cpp_lib', 'external_static_cpp_lib', 'external_shared_cpp_lib',
                       'system_static_cpp_lib', 'system_shared_cpp_lib', 'protobuf', 'external_phony'],
    'shared_cpp_lib': ['static_cpp_lib', 'shared_cpp_lib', 'external_shared_cpp_lib', 'system_shared_cpp_lib',
                       'system_static_cpp_lib', 'protobuf', 'external_phony'],
    'protobuf': ['protobuf', 'external_phony', 'custom'],
    'cpp_binary': ['static_cpp_lib', 'shared_cpp_lib', 'external_static_cpp_lib', 'external_shared_cpp_lib',
                   'system_static_cpp_lib', 'system_shared_cpp_lib', 'protobuf', 'external_phony'],
    'custom': all_types,
    'pseudo': all_types,
    'external_static_cpp_lib': ['external_static_cpp_lib', 'external_shared_cpp_lib', 'system_static_cpp_lib',
                                'system_shared_cpp_lib', 'external_phony'],
    'external_shared_cpp_lib': ['system_static_cpp_lib', 'system_shared_cpp_lib', 'external_phony'],
    'external_binary': all_types,
    'external_phony': all_types,
    'system_static_cpp_lib': ['system_static_cpp_lib', 'system_shared_cpp_lib', 'external_phony'],
    'system_shared_cpp_lib': ['system_shared_cpp_lib', 'external_phony'],
    'android_app': all_types,
    'external_java_lib': all_types,
    'kernel_module': ['kernel_module', 'external_phony'],
}

# Shared library types.  Needed to control dependency traversal for building linker commands.
#   (Enumeration of linker prereqs does not advance beyond shared libraries.)
# Okay to be here as, barring errors in the list, it is only updated if new types are added.
shared_lib_types = ['shared_cpp_lib', 'external_shared_cpp_lib', 'system_shared_cpp_lib']

# Set of targets that, when depended on transitively, may affect linker command lines.
# Okay to be here as, barring errors in the list, it is only updated if new types are added.
linker_types = ['static_cpp_lib','shared_cpp_lib','cpp_binary','external_static_cpp_lib',
                'external_shared_cpp_lib','system_static_cpp_lib','system_shared_cpp_lib','external_java_lib']

# Okay to be here as it only affects internal constraints and not generated Makefile.
def assert_list_of_strings(l, item_name, target_name):
    if not isinstance(l, list) or isinstance(l, basestring):
        raise Exception('"' + item_name + '" must be specified as a list; it was ' + str(type(l)) + ', in ' + target_name)
    for curr in l:
        if not isinstance(curr, basestring):
            raise Exception('"' + item_name + '" must be specified as a list of strings; found ' + str(type(curr)) + ' within list, in ' + target_name)

# Okay to be here as it only affects internal constraints and not generated Makefile.
def assert_list_of_non_empty_strings(l, item_name, target_name):
    assert_list_of_strings(l, item_name, target_name)
    for curr in l:
        if len(curr) == 0:
            raise Exception('There was an empty string in list "' + item_name + '" in ' + target_name)

# Okay to be here as it only affects internal constraints and not generated Makefile.
def assert_string(s, item_name, target_name):
    if not isinstance(s, basestring):
        raise Exception('"' + item_name + '" must be specified as a string; it was ' + str(type(s)) + ', in ' + target_name)
