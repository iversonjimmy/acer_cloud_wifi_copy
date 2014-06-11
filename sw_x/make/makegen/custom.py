class custom(build_target):
    def __init__(self, name, input, output, recipe, builddirs, force=False, deps=[], conflicts_with=[]):
        super(custom, self).__init__(
                ttype = 'custom',
                name = name,
                tdeps = deps,
                outs = output,
                builddirs = builddirs,
                conflicts_with = conflicts_with)
        assert_list_of_strings(input, 'input', self.fullname)
        assert_list_of_strings(output, 'output', self.fullname)
        assert_list_of_strings(recipe, 'recipe', self.fullname)
        if len(output) < 1:
            raise Exception('"output" must have at least one element for target type "custom"; in ' + self.fullname)
        self.input = [self.srcPathToMakefilePath(curr) for curr in input]
        self.force = force
        self.recipe = ''
        params = {
            'self.builddir': (self.path[:-1] if self.path.endswith('/') else self.path),
            'self.srcdir': '$(SRCROOT)/' + self.path,
        }
        for i, curr in enumerate(self.input):
            params['in' + str(i + 1)] = curr
        for i, curr in enumerate(self.outs):
            params['out' + str(i + 1)] = curr
        for curr in recipe:
            self.recipe += '\t'
            self.recipe += curr % params
            self.recipe += '\n'
    def get_java_lib_outs(self):
        return [x for x in self.outs if x.endswith('.jar')]
    def gen_make(self, makefile):
        prereqs = getPrereqs(self)
        fmt_params = {
                 'phony_make_target': self.phony_make_target,
                 'outs': str.join(' ', self.outs),
                 'token': self.token,
                 'path': self.path,
                 'prereqs': str.join(' ', prereqs),
                 'dirrule': self.mkdir_rule(),
                 'ins': str.join(' ', self.input),
                 'recipe': self.recipe,
                }
        if self.force:
            makefile.write((
                    '.PHONY: %(phony_make_target)s\n'
                    '%(phony_make_target)s: $(DEFS_FILES) $(SRCROOT)/make/makegen/custom.py $(SRCROOT)/%(path)sBUILD %(ins)s | %(token)s_DIRS %(prereqs)s\n'
                    '\t@echo \#\#\# BEGIN %(phony_make_target)s\n'
                    '%(recipe)s' # recipe always ends with a newline
                    '\t@echo \#\#\# END %(phony_make_target)s\n'
                    
                    '%(outs)s: %(phony_make_target)s\n'
                    '\t@true # Dummy recipe so that make warns if there is another rule to generate any of these outputs (which indicates a BUILD file authoring error)\n'
                    '%(dirrule)s\n'
                    ) % fmt_params)
        else:
            makefile.write((
                    '.PHONY: %(phony_make_target)s\n'
                    '%(phony_make_target)s: %(phony_make_target)s.timestamp\n'
                    '%(phony_make_target)s.timestamp: $(DEFS_FILES) $(SRCROOT)/make/makegen/custom.py $(SRCROOT)/%(path)sBUILD %(ins)s | %(token)s_DIRS %(prereqs)s\n'
                    '\t@echo \#\#\# BEGIN %(phony_make_target)s\n'
                    '%(recipe)s' # recipe always ends with a newline
                    '\ttouch %(phony_make_target)s.timestamp\n'
                    '\t@echo \#\#\# END %(phony_make_target)s\n'
                    
                    '%(outs)s: %(phony_make_target)s.timestamp\n'
                    '\t@true # Dummy recipe so that make warns if there is another rule to generate any of these outputs (which indicates a BUILD file authoring error)\n'
                    
                    '%(dirrule)s'
                    ) % fmt_params)
