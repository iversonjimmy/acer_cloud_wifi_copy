class android_app(build_target):
    def __init__(self, name, target, links, custom_build_xml=False, deps=[]):
        super(android_app, self).__init__(ttype = 'android_app', name = name, tdeps = deps,
                outs = [name], builddirs = [name + '/libs/armeabi'], conflicts_with = [])
        self.target = target
        self.custom_build_xml = custom_build_xml
        self.links = {}
        for l in links:
            self.links[self.buildPathToMakefilePath(name + '/' + l)] =\
                    self.srcPathToMakefilePath(links[l])
    def gen_make(self, makefile):
        makefile.write(self.mkdir_rule() + '\n')
        for ln in self.links:
            makefile.write(
                    '%(dest)s: %(src)s | %(token)s_DIRS\n'
                    '\tmkdir -p $(dir $@)\n'
                    '\tln -s -T $< $@\n' %
                    { 'token': self.token,
                      'dest': ln,
                      'src': self.links[ln]
                    })
        sharedLibs = []
        traverseDeps(self, [], ['shared_cpp_lib'],
                lambda tgt,libs: libs.append(tgt.outs[0]), sharedLibs, False)
        javaLibs = []
        traverseDeps(self, [], ['external_java_lib','custom'],
                lambda tgt,libs: libs.extend(tgt.get_java_lib_outs()), javaLibs, False)
        # maps "dest location" -> "source location" 
        libs = {}
        for l in sharedLibs:
            libs[self.path + self.name + '/libs/armeabi/' + os.path.split(l)[1]] = l
        for l in javaLibs:
            libs[self.path + self.name + '/libs/' + os.path.split(l)[1]] = l
        for l in libs:
            makefile.write(
                    '%(dest)s: %(src)s | %(token)s_DIRS\n'
                    '\t@echo \#\#\# %(name)s pulls in %(lib_name)s\n'
                    '\tcp $< $@\n'
                    '%(striplib)s' %
                    { 'token': self.token,
                      'path': self.path,
                      'name': self.name,
                      'lib_name': os.path.split(l)[1],
                      'dest': l,
                      'src': libs[l],
                      'striplib': '\t$(STRIPLIB) $@\n' if l.endswith('.so') else ''
                    })
        prereqs = getPrereqs(self)
        makefile.write(
            (
                '# Incremental ant builds are acting strangely, so start clean by default.\n'
                'ifndef DEVELOP_ENV_ANDROID_APP_INCREMENTAL\n'
                '.PHONY: %(path)s%(name)s.last_cleaned_timestamp\n'
                'endif\n'
                '%(path)s%(name)s.last_cleaned_timestamp: $(DEFS_FILES) $(SRCROOT)/make/makegen/android_app.py $(SRCROOT)/%(path)sBUILD'
                    + (
                    ' $(SRCROOT)/%(path)s/build.xml'
                    if self.custom_build_xml else ''
                    ) +
                    '\n'
                '\trm -rf %(path)s%(name)s\n'
                '\tmkdir -p %(path)s%(name)s\n'
                '\ttouch $@\n'
                '.PHONY: %(token)s_PREREQS\n'
                '%(token)s_PREREQS: %(prereqs)s %(libs)s %(links)s\n'
                '%(path)s%(name)s/build.xml: %(path)s%(name)s.last_cleaned_timestamp '
                    + (
                    '$(SRCROOT)/%(path)s/build.xml '
                    if self.custom_build_xml else ''
                    ) +
                    '| %(token)s_PREREQS\n'
                '\tcd $(ANDROID_SDK_ROOT)/tools && \\\n'
                '\t    ./android update project --name \"%(name)s\" --target \"%(target)s\" --path \"$(PBROOT)/%(path)s%(name)s\"\n'
                + (
                '\trm %(path)s%(name)s/build.xml \n'
                '\tcp $(SRCROOT)/%(path)s/build.xml %(path)s%(name)s/ \n'
                if self.custom_build_xml else ''
                ) + 
                '.PHONY: %(path)s%(name)s\n' \
                '%(path)s%(name)s: %(path)s%(name)s/build.xml\n' \
                '\t@echo \#\#\# BEGIN %(path)s%(name)s\n'
                '\tcd %(path)s%(name)s && $(ANDROID_ANT) $(ANT_FLAGS) -verbose $(BUILDTYPE)\n'
                '\t@echo \#\#\# END %(path)s%(name)s\n'
            ) %
                { 'path': self.path,
                  'name': self.name,
                  'token': self.token,
                  'prereqs': str.join(' ', prereqs),
                  'target': self.target,
                  'libs': str.join(' ', libs.keys()),
                  'links': str.join(' ', self.links.keys())
                })
