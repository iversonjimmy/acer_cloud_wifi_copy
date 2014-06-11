class system_cpp_lib(build_target):
    def __init__(self, ttype, name, file, deps, conflicts_with):
        super(system_cpp_lib, self).__init__(ttype = ttype, name = name,
                tdeps = deps, outs = [], abs_outs=[file], builddirs = [],
                conflicts_with = conflicts_with)
    def get_exported_cpp_includes(self):
        return []
    def gen_make(self, makefile):
        makefile.write(
                '.PHONY: %(path)s%(name)s\n'
                '%(path)s%(name)s: %(out)s\n' %
                {
                 'path': self.path,
                 'name': self.name,
                 'out': self.outs[0],
                })

class system_static_cpp_lib(system_cpp_lib):
    def __init__(self, name, file, deps=[], conflicts_with=[]):
        super(system_static_cpp_lib, self).__init__('system_static_cpp_lib',
                name, file, deps, conflicts_with)

class system_shared_cpp_lib(system_cpp_lib):
    def __init__(self, name, file, deps=[], conflicts_with=[]):
        super(system_shared_cpp_lib, self).__init__('system_shared_cpp_lib',
                name, file, deps, conflicts_with)
