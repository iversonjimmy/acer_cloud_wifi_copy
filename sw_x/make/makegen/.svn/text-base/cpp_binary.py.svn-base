class cpp_binary(build_target):
    def __init__(self, name, srcs, includes=[], deps=[], cflags='',
                cxxflags='', ldflags=''):
        super(cpp_binary, self).__init__(
                ttype = 'cpp_binary',
                name = name,
                tdeps = deps,
                outs = [name + '$(BINARY_EXECUTABLE_SUFFIX)'],
                builddirs = [],
                conflicts_with = [],
                cflags = cflags,
                cxxflags = cxxflags,
                ldflags = ldflags)
        self.srcs = ['%s%s' % (self.path, src) for src in srcs]
        self.builddirs = [os.path.dirname(src) for src in srcs]
        self.includes = [self.srcPathToMakefilePath(inc) for inc in includes]
    def get_exported_cpp_includes(self):
        return self.includes
    def gen_make(self, makefile):
        out = self.outs[0]
        deps = self.getDeps(False)
        prereqs = getPrereqs(self)
        linkBuildFiles = getBuildFilesAffectingLinkLine(self)
        makefile.write(
                'ifneq ($(BINARY_EXECUTABLE_SUFFIX),)\n'
                '.PHONY: %(path)s%(name)s\n'
                '%(path)s%(name)s: %(out)s\n'
                'endif\n'
                '%(dirrule)s\n'
                '%(token)s_C_SRCS := %(c_srcs)s\n'
                '%(token)s_CC_SRCS := %(cc_srcs)s\n'
                '%(token)s_CPP_SRCS := %(cpp_srcs)s\n'
                '%(token)s_OBJS := $(%(token)s_C_SRCS:.c=%(obj_suffix)s.o) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.o) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.o)\n'
                '%(token)s_OBJS_AND_DEPS := $(%(token)s_OBJS) %(deps)s\n'
                '%(out)s $(%(token)s_OBJS): $(DEFS_FILES) $(SRCROOT)/make/makegen/cpp_binary.py $(SRCROOT)/%(path)sBUILD | %(token)s_DIRS %(prereqs)s\n'
                '%(out)s: $(%(token)s_OBJS_AND_DEPS) | %(link_build_files)s\n'
                '\t$(CXX) $(LDFLAGS) %(ldflags)s $(%(token)s_OBJS_AND_DEPS) -o $@\n'
                '\t$(call POST_PROCESS_BINARY,$@)\n'
                '-include $(%(token)s_C_SRCS:.c=%(obj_suffix)s.d) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.d)\n'
                '$(%(token)s_C_SRCS:.c=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.c\n'
                '\trm -f $@ # workaround for GCC bug 44049\n'
                '\t$(CC) -c $(CFLAGS) %(cflags)s %(incflags)s $(CINCLUDES) $< -o $@ -MMD -MP\n'
                '\t$(call POST_PROCESS_C_OBJ,$<,$(basename $@).o,$(basename $@).d)\n'
                '$(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.cc\n'
                '\trm -f $@ # workaround for GCC bug 44049\n'
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $< -o $@ -MMD -MP\n'
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).o,$(basename $@).d)\n'
                '$(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.cpp\n'
                '\trm -f $@ # workaround for GCC bug 44049\n'
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $< -o $@ -MMD -MP\n'
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).o,$(basename $@).d)\n'
                '$(PBROOT)/%(out)s: %(out)s\n' %
                { 'token': self.token,
                  'dirrule': self.mkdir_rule(),
                  'c_srcs': str.join(' ', filter(c_filter, self.srcs)),
                  'cc_srcs': str.join(' ', filter(cc_filter, self.srcs)),
                  'cpp_srcs': str.join(' ', filter(cpp_filter, self.srcs)),
                  'out': out,
                  'deps': str.join(' ', deps),
                  'prereqs': str.join(' ', prereqs),
                  'path': self.path,
                  'name': self.name,
                  'cflags': self.args.get('cflags', ''),
                  'cxxflags': self.args.get('cxxflags', ''),
                  'ldflags': self.args.get('ldflags', ''),
                  'incflags': str.join(' ', getIncFlags(self)),
                  'link_build_files': str.join(' ', linkBuildFiles),
                  'obj_suffix': '.' + self.name,
                })
