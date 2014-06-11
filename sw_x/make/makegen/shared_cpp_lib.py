class shared_cpp_lib(build_target):
    def __init__(self, name, srcs, soname, includes=[], private_includes=[],
                deps=[], cflags = '', cxxflags = '', ldflags = '', conflicts_with=[],
                whole_archive=True):
        super(shared_cpp_lib, self).__init__(
                ttype = 'shared_cpp_lib',
                name = name,
                tdeps = deps,
                outs = [name+'.so'],
                builddirs = [],
                conflicts_with = conflicts_with,
                cflags = cflags,
                cxxflags = cxxflags,
                ldflags = ldflags)
        self.srcs = ['%s%s' % (self.path, src) for src in srcs]
        self.builddirs = [os.path.dirname(src) for src in srcs]
        self.includes = [self.srcPathToMakefilePath(inc) for inc in includes]
        self.priv_includes = [self.srcPathToMakefilePath(inc) for inc in private_includes]
        self.soname = soname
        self.whole_archive = whole_archive
    def get_exported_cpp_includes(self):
        return self.includes
    def gen_make(self, makefile):
        out = self.outs[0]
        deps = self.getDeps(True)
        prereqs = getPrereqs(self)
        linkBuildFiles = getBuildFilesAffectingLinkLine(self)
        makefile.write(
                '.PHONY: %(path)s%(name)s\n' \
                '%(path)s%(name)s: %(out)s\n' \
                '%(dirrule)s\n' \
                '%(token)s_C_SRCS := %(c_srcs)s\n' \
                '%(token)s_CC_SRCS := %(cc_srcs)s\n' \
                '%(token)s_CPP_SRCS := %(cpp_srcs)s\n' \
                '%(token)s_OBJS := $(%(token)s_C_SRCS:.c=%(obj_suffix)s.os) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.os) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.os)\n' \
                '%(token)s_OBJS_AND_DEPS := $(%(token)s_OBJS) %(deps)s\n' \
                '%(out)s $(%(token)s_OBJS): $(DEFS_FILES) $(SRCROOT)/make/makegen/shared_cpp_lib.py $(SRCROOT)/%(path)sBUILD | %(token)s_DIRS %(prereqs)s\n' \
                '%(out)s: $(%(token)s_OBJS_AND_DEPS) | %(link_build_files)s\n' \
                '\t$(CXX) -Wl,-soname,%(soname)s $(LDFLAGS) %(ldflags)s -shared %(archive_flag_1)s$(%(token)s_OBJS_AND_DEPS) %(archive_flag_2)s-o $@\n' \
                '\t$(call POST_PROCESS_SHARED_LIB,$@)\n' \
                '-include $(%(token)s_C_SRCS:.c=%(obj_suffix)s.os.d) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.os.d)\n' \
                '$(%(token)s_C_SRCS:.c=%(obj_suffix)s.os): %(path)s%%%(obj_suffix)s.os: $(SRCROOT)/%(path)s%%.c\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CC) -c $(CFLAGS) %(cflags)s %(incflags)s $(CINCLUDES) $(PIC_COMPILE_FLAGS) $< -MMD -MP -MF $(basename $@).os.d -o $@\n' \
                '\t$(call POST_PROCESS_C_OBJ,$<,$(basename $@).os,$(basename $@).os.d)\n' \
                '$(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.os): %(path)s%%%(obj_suffix)s.os: $(SRCROOT)/%(path)s%%.cc\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $(PIC_COMPILE_FLAGS) $< -MMD -MP -MF $(basename $@).os.d -o $@\n' \
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).os,$(basename $@).os.d)\n' \
                '$(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.os): %(path)s%%%(obj_suffix)s.os: $(SRCROOT)/%(path)s%%.cpp\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $(PIC_COMPILE_FLAGS) $< -MMD -MP -MF $(basename $@).os.d -o $@\n' \
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).os,$(basename $@).os.d)\n' \
                 %
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
                  'soname' : self.soname,
                  'archive_flag_1': {True:'-Wl,--whole-archive ', False:''}[self.whole_archive],
                  'archive_flag_2': {True:'-Wl,--no-whole-archive ', False:''}[self.whole_archive],
                  'link_build_files': str.join(' ', linkBuildFiles),
                  'obj_suffix': '.' + self.name,
                })
