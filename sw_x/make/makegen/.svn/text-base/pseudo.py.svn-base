# phony target to build other targets
class pseudo(build_target):
    def __init__(self, name, deps, default=False):
        super(pseudo, self).__init__(ttype = 'pseudo', name = name, tdeps = deps,
                outs = [name], builddirs = [], conflicts_with = [])
        if default:
            if default_target != '':
                raise Exception('More than one default target: %s, %s' % (self.fullname, targets[default_target].fullname))
            default_target = self.fullname
    def gen_make(self, makefile):
        phony_deps = []
        for tdep in self.tdeps:
            phony_deps.append(targets[tdep].phony_make_target)
        makefile.write(
                '.PHONY: %(build_target_name)s\n' \
                '%(build_target_name)s: %(dep_outs)s\n' %
                { 'build_target_name': self.path + self.name,
                  'dep_outs': str.join(' ', phony_deps)
                })
