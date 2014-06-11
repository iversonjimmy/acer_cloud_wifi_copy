# External targets are targets that are built by recursing into another Makefile,
# such as for third-party libs like protobuf or curl.

# Base class for all external target types.
class external_thing(build_target):
    def __init__(self, ttype, name, srcdir, includes, abs_outs, deps, conflicts_with):
        super(external_thing, self).__init__(
                ttype = ttype, name = name, tdeps = deps, outs = [], abs_outs = abs_outs,
                builddirs = [], conflicts_with = conflicts_with)
        assert_string(srcdir, 'srcdir', self.fullname)
        assert_list_of_strings(includes, 'includes', self.fullname)
        self.srcdir = self.srcPathToMakefilePath(srcdir)
        self.includes = ['$(PBROOT)/' + inc for inc in includes]
    def get_exported_cpp_includes(self):
        return self.includes
    def gen_make(self, makefile):
        prereqs = getPrereqs(self)
        makefile.write(
                (('.PHONY: %(path)s%(name)s\n' \
                  '%(path)s%(name)s: %(out)s\n' \
                  if not self.path + self.name in self.outs else '') +\
                '%(out)s: __FORCE | %(prereqs)s\n' \
                '\t$(MAKE) -C %(srcdir)s\n') %
                { 'token': self.token,
                  'prereqs': str.join(' ', prereqs),
                  'path': self.path,
                  'name': self.name,
                  'out': self.outs[0],
                  'srcdir': self.srcdir
                })

class external_phony(external_thing):
    def __init__(self, name, srcdir='.', deps=[], conflicts_with=[]):
        super(external_phony, self).__init__(
                'external_phony', name, srcdir, [], [], deps, conflicts_with)
    def gen_make(self, makefile):
        prereqs = getPrereqs(self)
        makefile.write(
                ('.PHONY: %(path)s%(name)s\n'
                 '%(path)s%(name)s: %(prereqs)s\n'
                 '\t$(MAKE) -C %(srcdir)s\n') %
                { 'prereqs': str.join(' ', prereqs),
                  'path': self.path,
                  'name': self.name,
                  'srcdir': self.srcdir
                })

class external_static_cpp_lib(external_thing):
    def __init__(self, name, includes, outfile, srcdir='.', deps=[], conflicts_with=[]):
        super(external_static_cpp_lib, self).__init__(
                'external_static_cpp_lib', name, srcdir, includes, [outfile], deps, conflicts_with)

class external_shared_cpp_lib(external_thing):
    def __init__(self, name, includes, outfile, srcdir='.', deps=[], conflicts_with=[]):
        super(external_shared_cpp_lib, self).__init__(
                'external_shared_cpp_lib', name, srcdir, includes, [outfile], deps, conflicts_with)

class kernel_module(external_thing):
    def __init__(self, name, outfile, srcdir='.', deps=[]):
        super(kernel_module, self).__init__(
                'kernel_module', name, srcdir, [], [outfile], deps, [])

class external_binary(external_thing):
    def __init__(self, name, outfile, srcdir='.'):
        super(external_binary, self).__init__(
                'external_binary', name, srcdir, [], [outfile], [], [])
    def gen_make(self, makefile):
        super(external_binary, self).gen_make(makefile)
        # To help make we alias the full path of the binary to the relative path.
        # Make is not good at resolving paths and some external_binary targets
        # need to be usable as tools during the build process.
        if self.outs[0][0] != '/':
            makefile.write(
                    '$(PBROOT)/%(out)s: %(out)s\n' %\
                    { 'out': self.outs[0]
                    })

class external_java_lib(external_thing):
    def __init__(self, name, outs, srcdir='.', deps=[], conflicts_with=[]):
        super(external_java_lib, self).__init__(
                'external_java_lib', name, srcdir, [], outs, deps, conflicts_with)
    def get_java_lib_outs(self):
        return self.outs

