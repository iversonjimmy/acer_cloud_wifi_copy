class static_cpp_lib(build_target):
    def __init__(self, name, srcs, includes=[], private_includes=[], deps=[],
                cflags='', cxxflags='', conflicts_with=[]):
        super(static_cpp_lib, self).__init__(
                ttype = 'static_cpp_lib',
                name = name,
                tdeps = deps,
                outs = [name+'.a', name+'_pic.a'],
                builddirs = [],
                conflicts_with = conflicts_with,
                cflags = cflags,
                cxxflags = cxxflags)
        assert_list_of_non_empty_strings(srcs, 'srcs', self.fullname)
        assert_list_of_non_empty_strings(private_includes, 'private_includes', self.fullname)
        self.srcs = ['%s%s' % (self.path, src) for src in srcs]
        self.builddirs = [os.path.dirname(src) for src in srcs]
        self.includes = [self.srcPathToMakefilePath(inc) for inc in includes]
        self.priv_includes = [self.srcPathToMakefilePath(inc) for inc in private_includes]
        if self.is_dummy_lib():
            self.outs = []
    def is_dummy_lib(self):
        # If there are no source files, this is a dummy library; don't contribute the .a file to dependents.
        return (len(self.srcs) == 0)
    def get_exported_cpp_includes(self):
        return self.includes
    def get_exported_libs(self, bUsePic):
        if self.is_dummy_lib():
            return []
        elif bUsePic:
            return [self.path + self.name + '_pic.a']
        else:
            return [self.path + self.name + '.a']
    def gen_make(self, makefile):
        nonPicDeps = self.getDeps(False)
        picDeps = self.getDeps(True)
        prereqs = getPrereqs(self)
        makefile.write(
                (
                # When someone explicitly asks to build this makegen target, we build the non-pic version
                # of the library by default.
                # They can append ".pic" to get the pic version instead, or ".all" to get both versions.
                # both versions of the library (pic and/or non-pic).
                '.PHONY: %(phony_make_target)s %(phony_make_target)s.pic %(phony_make_target)s.all\n'
                
                '.PHONY: %(token)s_PREREQS\n'
                '%(token)s_PREREQS: %(prereqs)s\n'
                
                '%(dirrule)s\n'
                
                + (
                    '%(phony_make_target)s %(phony_make_target)s.pic %(phony_make_target)s.all:'
                    if self.is_dummy_lib()
                    else
                    # We also need to build all of the libraries listed in our dependencies, since they
                    # would not otherwise be built (since this .a does not depend upon any of them).
                    '%(phony_make_target)s: %(path)s%(name)s.a %(non_pic_deps)s\n'
                    '%(phony_make_target)s.pic: %(path)s%(name)s_pic.a %(pic_deps)s\n'
                    '%(phony_make_target)s.all: %(phony_make_target)s %(phony_make_target)s.pic\n'
                ) + ('' if self.is_dummy_lib() else
                '%(token)s_C_SRCS := %(c_srcs)s\n' \
                '%(token)s_CC_SRCS := %(cc_srcs)s\n' \
                '%(token)s_CPP_SRCS := %(cpp_srcs)s\n' \
                '%(token)s_OBJS := $(%(token)s_C_SRCS:.c=%(obj_suffix)s.o) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.o) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.o)\n' \
                '%(token)s_PIC_OBJS := $(%(token)s_C_SRCS:.c=%(obj_suffix)s.os) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.os) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.os)\n' \
                '%(path)s%(name)s.a %(path)s%(name)s_pic.a $(%(token)s_OBJS) $(%(token)s_PIC_OBJS) %(path)s%(name)s.a.last_cleaned_timestamp %(path)s%(name)s_pic.a.last_cleaned_timestamp:\\\n' \
                '   $(DEFS_FILES) $(SRCROOT)/make/makegen/static_cpp_lib.py $(SRCROOT)/%(path)sBUILD | %(token)s_DIRS %(token)s_PREREQS\n' \
                '%(path)s%(name)s.a.last_cleaned_timestamp:\n' \
                '\trm -f %(path)s%(name)s.a\n' \
                '\ttouch $@\n' \
                '%(path)s%(name)s_pic.a.last_cleaned_timestamp:\n' \
                '\trm -f %(path)s%(name)s_pic.a\n' \
                '\ttouch $@\n' 
                '%(path)s%(name)s.a: $(%(token)s_OBJS) %(path)s%(name)s.a.last_cleaned_timestamp\n'
                '\t$(AR) cru $@ $(filter $(%(token)s_OBJS),$?)\n'
                '%(path)s%(name)s_pic.a: $(%(token)s_PIC_OBJS) %(path)s%(name)s_pic.a.last_cleaned_timestamp\n'
                '\t$(AR) cru $@ $(filter $(%(token)s_PIC_OBJS),$?)\n'
                '-include $(%(token)s_C_SRCS:.c=%(obj_suffix)s.d) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.d) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.d)\n' \
                '-include $(%(token)s_C_SRCS:.c=%(obj_suffix)s.os.d) $(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.os.d) $(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.os.d)\n' \
                '$(%(token)s_C_SRCS:.c=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.c\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CC) -c $(CFLAGS) %(cflags)s %(incflags)s $(CINCLUDES) $< -MMD -MP -o $@\n' \
                '\t$(call POST_PROCESS_C_OBJ,$<,$(basename $@).o,$(basename $@).d)\n' \
                '$(%(token)s_CC_SRCS:.cc=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.cc\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $< -MMD -MP -o $@\n' \
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).o,$(basename $@).d)\n' \
                '$(%(token)s_CPP_SRCS:.cpp=%(obj_suffix)s.o): %(path)s%%%(obj_suffix)s.o: $(SRCROOT)/%(path)s%%.cpp\n' \
                '\trm -f $@ # workaround for GCC bug 44049\n' \
                '\t$(CXX) -c $(CXXFLAGS) %(cxxflags)s %(incflags)s $(CXXINCLUDES) $< -MMD -MP -o $@\n' \
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).o,$(basename $@).d)\n' \
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
                '\t$(call POST_PROCESS_CXX_OBJ,$<,$(basename $@).os,$(basename $@).os.d)\n'
                )
                ) %
                { 'token': self.token,
                  'dirrule': self.mkdir_rule(),
                  'c_srcs': str.join(' ', filter(c_filter, self.srcs)),
                  'cc_srcs': str.join(' ', filter(cc_filter, self.srcs)),
                  'cpp_srcs': str.join(' ', filter(cpp_filter, self.srcs)),
                  'non_pic_deps': str.join(' ', nonPicDeps),
                  'pic_deps': str.join(' ', picDeps),
                  'prereqs': str.join(' ', prereqs),
                  'path': self.path,
                  'name': self.name,
                  'phony_make_target': self.phony_make_target,
                  'outs': str.join(' ', self.outs),
                  'cflags': self.args.get('cflags',''),
                  'cxxflags': self.args.get('cxxflags',''),
                  'incflags': str.join(' ', getIncFlags(self)),
                  'obj_suffix': '.' + self.name,
                })
