# TODO: detect conflicts between targets and directories; targets named the same as a subdirectory
#    can cause problems in make because a rule may conflict with the directory name
# TODO: pseudo target default parameter broken
# TODO: make clean? (maybe this is just rm $(BUILDROOT))
# TODO: when a header listed in a .d is removed, the source file that depends on it should be considered out-of-date
# TODO: consider "conflicts_with" hard vs soft errors, strict mode option
import sys
import optparse
import os
sys.path.append('make/makegen/')
from internal_defs import *

# Command line arguments
parser = optparse.OptionParser()
parser.add_option("-o", "--outdir", type="string", dest="outdir", default=None,
        help="Location to write Makefile and Make.deps.")
parser.add_option("-s", "--startdir", type="string", dest="startdir", default=".",
        help="Directory to read initial BUILD and BUILD.defs from.")
parser.add_option("-l", "--list_make_targets", action="store_true", dest="list_make_targets", default=False,
        help="List all targets found for the current product, as make targets")
parser.add_option("--list_makegen_targets", action="store_true", dest="list_makegen_targets", default=False,
        help="List all targets found for the current product, as makegen target references")
(options, args) = parser.parse_args()
if options.outdir == None and not options.list_make_targets and not options.list_makegen_targets:
    print "No output dir specified; will not generate Makefile."

# Get an environment variable, or a default if the variable is not set.
# If default is None and the variable is not set, an exception is raised.
def env(var, default):
    if var in os.environ:
        return os.environ[var]
    else:
        if default == None:
            raise Exception('Environment variable %s not defined.' % var)
        return default

# Map of canonicalized target names to the actual build_target objects, built up
# as BUILD files are processed. 
targets = {}
# Path of current BUILD file, usable in target constructors.
curpath = ''
# Canonicalized name of the target that should be invoked by "make default" or "make".
# Currently broken.
default_target = ''
# Set of target names that are required but not yet defined.  Used during
# traversal of BUILD files.
# Each key is a target name, each value is a list of references that caused the target to
# be added to the build. 
undef_targets = dict()
resolver_stack = []

def tokenify(s):
    return s.replace('/', '_').replace(':','__').replace('-','_')[1:]
def c_filter(s):
    return s.endswith('.c')
def cc_filter(s):
    return s.endswith('.cc')
def cpp_filter(s):
    return s.endswith('.cpp')

# Remove duplicates from a list and preserve the order.
def remove_duplicates(list):
    map = {}
    result = []
    for curr in list:
        if curr in map: continue
        map[curr] = True
        result.append(curr)
    return result

# Canonicalize a target name: normalize and make absolute the path,
# and ensure the target name is appended
def canonicalize_target(tname):
    if tname.rfind(':') == -1:
        path = tname
        shortname = None
    else:
        path = tname[:tname.rfind(':')]
        shortname = tname[tname.rfind(':')+1:]
    if len(path) == 0 or path[0] != '/':
        path = '/' + curpath + path
    path = os.path.normpath(path)
    if shortname == None:
        shortname = path[path.rfind('/')+1:]
    return path + ':' + shortname

# Traverse all dependencies of a target, and sort into topological order.
# Do not traverse beyond targets of types in stop_at.
# Returns an ordered list of targets where each target comes after all other
# targets that it depends on.
# Circular dependencies are not detected by this function and will be silently ignored.
# You can use findCircularDep to detect circular dependencies.
def sortDeps(target, inc_self, stop_at_types=[], include_types=all_types):
    result = []
    sortDepsRec(result, set(), target, stop_at_types, include_types)
    if inc_self and (target.ttype in include_types):
        result.append(target)
    return result

def sortDepsRec(result, visited, target, stop_at_types, include_types):
    if not target.ttype in stop_at_types:
        # Traverse in reverse order to keep "siblings" in the same order that they were declared.
        for currDepName in reversed(target.tdeps):
            currDep = targets[currDepName]
            if currDep.fullname in visited:
                continue
            visited.add(currDep.fullname)
            sortDepsRec(result, visited, currDep, stop_at_types, include_types)
            if target.ttype in include_types:
                result.append(currDep)

# Traverse all dependencies of a target.  Do not traverse beyond targets
# of types in stop_at.  For each target of a type in call_at, the callback
# cb is called with arguments (target,cb_arg).  If inc_self is True, the
# starting target may also be passed to the callback.  Deps are processed
# in topological order with downstream dependencies first.
def traverseDeps(target, stop_at, call_at, cb, cb_arg, inc_self):
    traverseDepsRec(set(), target, stop_at, call_at, cb, cb_arg)
    if inc_self and target.ttype in call_at:
        cb(target, cb_arg)

def traverseDepsRec(visited, target, stop_at, call_at, cb, cb_arg):
    for tdepname in target.tdeps:
        tdep = targets[tdepname]
        if tdep.fullname in visited:
            continue
        visited.add(tdep.fullname)
        if not tdep.ttype in stop_at:
            traverseDepsRec(visited, tdep, stop_at, call_at, cb, cb_arg)
        if tdep.ttype in call_at:
            cb(tdep, cb_arg)

# Gets transitive dependencies of a target, in order, such that a target
# only depends on targets after it in the list.  Does not traverse beyond
# targets of types listed in the stop argument, e.g. ['shared_cpp_lib'].
def getTransitiveDeps(target, stop=[]):
    acc = []
    traverseDeps(target, stop, all_types,
            lambda tgt,acc: acc.append(tgt.fullname), acc, False)
    acc.reverse()
    return acc

# Generates the -I flags for a compile command line.  The flags are ordered
# such that each module's own include paths come before the include paths of its
# dependencies.
def getIncFlags(target):
    incflags = []
    # It is a bit sketchy to use os.path.normpath when the paths include
    # make variables, but it seems to work.
    if target.ttype in ['static_cpp_lib','shared_cpp_lib']:
        incflags.extend(['-I' + os.path.normpath(inc) for inc in target.priv_includes])
    sorted_targets = sortDeps(target, True)
    for curr_target in reversed(sorted_targets):
        incflags.extend(['-I' + os.path.normpath(inc) for inc in curr_target.get_exported_cpp_includes()])
    deduped_incflags = remove_duplicates(incflags)
    return deduped_incflags

# Returns a list of the .PHONY rules for all direct dependencies that define such a rule
# ensuring all outputs are created, e.g. protobuf targets create a rule that can be
# depended on by targets needing the generated headers.  This set of rules is then
# depended on order-only by the target.
def getPrereqs(target):
    result = []
    # TODO: Bug 4646: If we remove 'android_app', it will currently cause the special 'build_native_libs_apk'
    #   target to be invoked for Android builds (and we don't want that).
    target_types_to_skip = ['system_static_cpp_lib', 'system_shared_cpp_lib', 'cpp_binary',
                            'android_app']
    for tname in target.tdeps:
        if targets[tname].ttype == 'static_cpp_lib':
            # Without this, we would end up building both the PIC
            # and non-PIC versions of the library even when we only need the PIC version.
            result.append(targets[tname].token + '_PREREQS')
        elif not targets[tname].ttype in target_types_to_skip:
            result.append(targets[tname].path + targets[tname].name)
    return result

# Returns the list of BUILD files that may affect the linker command line of this target.
# If the dependencies (transitive) of this target change but those objects are already
# up to date, the linker will not execute even though the set of objects being linked
# has changed.  Linker output is considered out of date if any file in this list changes.
def getBuildFilesAffectingLinkLine(target):
    files = set([])
    traverseDeps(target, shared_lib_types, linker_types,
                 lambda tgt,files: files.add(os.path.normpath('$(SRCROOT)/' + tgt.path + '/BUILD')),
                 files, True)
    return list(files)

# Debug utilities to query dependency graph
def findpaths(dest, tgt, acc, cb, cb_arg):
    if tgt.fullname == dest:
        cb(acc, cb_arg)
    for tdepname in tgt.tdeps:
        tdep = targets[tdepname]
        findpaths(dest, tdep, acc+[tdepname], cb, cb_arg)
def allpaths(t_from, t_to):
    paths = []
    if not t_from in targets:
        print 'Unknown target: ' + t_from
    elif not t_to in targets:
        print 'Unknown target: ' + t_to
    else:
        findpaths(t_to, targets[t_from], [t_from],
                lambda p,ps: ps.append(p), paths)
    return paths

# Base class for all build targets.
# Subclasses should define gen_make(self, makefile), which is called to output
# text to the generated Makefile.
class build_target(object):
    # path = relative path to the BUILD file that defined this target (relative to SRCROOT)
    # ttype = target type (string)
    # tdeps = list of targets we depend on
    # outs = list of final outputs of the target
    # builddirs = list of directories that must be constructed in the build
    #    tree for this target to run (relative to PBROOT)
    def __init__(self, ttype, name, tdeps, outs, builddirs, conflicts_with, abs_outs = [], **args):
        assert_string(name, 'name', curpath)
        self.fullname = canonicalize_target(':' + name)
        assert_string(ttype, 'ttype', self.fullname)
        assert_list_of_non_empty_strings(tdeps, 'deps', self.fullname)
        assert_list_of_non_empty_strings(outs, 'outs', self.fullname)
        assert_list_of_non_empty_strings(builddirs, 'builddirs', self.fullname)
        assert_list_of_non_empty_strings(conflicts_with, 'conflicts_with', self.fullname)
        assert_list_of_non_empty_strings(abs_outs, 'abs_outs', self.fullname)
        self.name = name
        self.path = curpath
        self.ttype = ttype
        self.token = tokenify(self.fullname)
        self.tdeps = [canonicalize_target(t) for t in tdeps]
        self.outs = [self.buildPathToMakefilePath(p) for p in outs] + abs_outs
        self.builddirs = builddirs + ['.']
        self.conflicts_with = [canonicalize_target(t) for t in conflicts_with]
        self.args = args
        self.protoc_includes = []
        self.phony_make_target = self.path + self.name
        if '/' in name or '\\' in name or '.' in name or ':' in name:
            raise Exception('makegen target names cannot contain any of [\\/.:]  Bad target: ' + self.fullname)
        if self.fullname in targets:
            raise Exception('Duplicate target name ' + self.fullname)
        targets[self.fullname] = self
        for tdep in self.tdeps:
            if not tdep in targets:
                if not tdep in undef_targets.keys():
                    undef_targets[tdep] = resolver_stack + [self.fullname]
        if self.fullname in undef_targets.keys():
            undef_targets.pop(self.fullname)
    # Get a list of outputs of direct dependencies, which are not of
    # types listed in exclude_types.
    def get_dependency_outs(self, exclude_types=[]):
        deps = []
        for tdep in self.tdeps:
            if targets[tdep].ttype in exclude_types:
                continue
            deps = deps + targets[tdep].outs
        return deps
    # Lists .a and .so libraries that this makegen target depends upon.
    # Used by cpp_binary, shared_cpp_lib, and static_cpp_lib.
    def getDeps(self, bUsePic):
        tgts = getTransitiveDeps(self, shared_lib_types)
        deps = []
        for tname in tgts:
            tgt = targets[tname]
            if tgt.ttype == 'static_cpp_lib':
                deps.extend(tgt.get_exported_libs(bUsePic))
            elif tgt.ttype != 'protobuf':
                deps.extend(tgt.outs)
        return deps
    # Return C/C++ include paths that should be contributed to targets that depend on this.
    #def get_exported_cpp_includes(self): abstract
    
    # Generate a rule to construct necessary directories in the build tree for
    # this target's recipes to execute.
    def mkdir_rule(self):
        self.builddirs = list(set([self.buildPathToMakefilePath(p) for p in self.builddirs]))
        return '.PHONY: %(token)s_DIRS\n' \
               '%(token)s_DIRS:\n' \
               '%(dircmds)s\n' % \
               { 'token': self.token,
                 'dircmds': str.join('\n', ['\tmkdir -p ' + bd for bd in self.builddirs if bd != ''])
               }
    # Convert the specified makegen-target-relative path to a path suitable for use in the Makefile.
    def replaceSpecialPath(self, rawPath):
        if rawPath.startswith('%self.builddir%/'):
            return os.path.normpath(rawPath.replace('%self.builddir%/', self.path))
        elif rawPath.startswith('%self.srcdir%/'):
            return os.path.normpath(rawPath.replace('%self.srcdir%/', '$(SRCROOT)/' + self.path))
        else:
            raise Exception('Illegal %% macro: %s' % (rawPath))
    def srcPathToMakefilePath(self, rawPath):
        if len(rawPath) > 0:
            if rawPath[0] == '/':
                return os.path.normpath('$(SRCROOT)' + rawPath)
            if rawPath[0] == '$':
                if rawPath.startswith('$(PBROOT)/'):
                    # Need to strip off PBROOT from make targets and prereqs;
                    # make treats "foo" and "$(PBROOT)/foo" as different targets,
                    # even though we are running from PBROOT.
                    return os.path.normpath(rawPath[10:])
                else:
                    return os.path.normpath(rawPath)
            if rawPath[0] == '%':
                return self.replaceSpecialPath(rawPath)
        return os.path.normpath('$(SRCROOT)/' + self.path + rawPath)
    def buildPathToMakefilePath(self, rawPath):
        if len(rawPath) > 0:
            if rawPath[0] == '/':
                return os.path.normpath(rawPath[1:])
            if rawPath[0] == '$':
                if rawPath.startswith('$(PBROOT)/'):
                    # Need to strip off PBROOT from make targets and prereqs;
                    # make treats "foo" and "$(PBROOT)/foo" as different targets,
                    # even though we are running from PBROOT.
                    return os.path.normpath(rawPath[10:])
                else:
                    return os.path.normpath(rawPath)
            if rawPath[0] == '%':
                return self.replaceSpecialPath(rawPath)
        return os.path.normpath(self.path + rawPath)

execfile('make/makegen/android_app.py', locals());
execfile('make/makegen/cpp_binary.py', locals());
execfile('make/makegen/custom.py', locals());
execfile('make/makegen/extern_targets.py', locals());
execfile('make/makegen/protobuf.py', locals());
execfile('make/makegen/pseudo.py', locals());
execfile('make/makegen/shared_cpp_lib.py', locals());
execfile('make/makegen/static_cpp_lib.py', locals());
execfile('make/makegen/system_targets.py', locals());

def refChainString(chain):
    result = 'Reference chain:'
    for (i, curr) in enumerate(chain):
        if (i % 2) == 0:
            result += '\n  '
            result += curr
        else:
            result += ' => '
            result += curr.partition(':')[0]
    return result

# parse the BUILD files, starting at startdir
dirs_read = set([])
startfile = os.path.normpath(options.startdir + '/BUILD')
curpath = os.path.split(startfile)[0] + '/'
if curpath == '/':
    curpath = ''
execfile(startfile + '.defs', locals())
execfile(startfile, dict(locals()))
dirs_read.add(curpath)
files_read = set([startfile, startfile + '.defs'])
while len(undef_targets) > 0:
    curr_undef_target = undef_targets.items()[0]
    tname = curr_undef_target[0]
    resolver_stack = curr_undef_target[1]
    resolver_stack.append(tname)
    curpath = tname[1:tname.rfind(':')] + '/'
    if curpath == '/':
        curpath = ''
    referenced_by = [t for t in targets if tname in targets[t].tdeps]
    oldcwd = os.getcwd()
    try:
        os.chdir('./' + curpath)
    except:
        print('*** Error searching for %s.\n%s' % (tname, refChainString(resolver_stack)))
        raise
    if curpath in dirs_read:
        raise Exception('%s undefined but already processed %sBUILD; ' \
                'referenced by %s.\n%s' %\
                (tname, curpath, str(referenced_by), refChainString(resolver_stack)))
    if os.path.isfile('BUILD'):
        try:
            execfile('BUILD', dict(locals()))
        except:
            print ('*** Error processing %sBUILD (see details below):\n%s' % (curpath, refChainString(resolver_stack)))
            raise
        files_read.add(curpath+'BUILD')
    else:
        raise Exception('%s undefined and %sBUILD does not exist; ' \
                'referenced by %s.\n%s' %\
                (tname, curpath, str(referenced_by), refChainString(resolver_stack)))
    dirs_read.add(curpath)
    os.chdir(oldcwd)

# check for circular dependencies
def findCircularDep(remain, target, chain):
    found = target.fullname in chain
    chain.append(target.fullname)
    if found:
        raise Exception('Circular dependency: ' + str.join(' ', chain))
    if target.fullname in remain:
        remain.remove(target.fullname)
    else:
        del chain[-1]
        return
    for tdep in target.tdeps:
        findCircularDep(remain, targets[tdep], chain)
    del chain[-1]

remain = set([t for t in targets])
while len(remain) > 0:
    t = list(remain)[0]
    findCircularDep(remain, targets[t], [])

# check for disallowed dependencies
def lintDeps(target):
    adeps = allowed_deps[target.ttype]
    for tdep in target.tdeps:
        if not targets[tdep].ttype in adeps:
            raise Exception('%s cannot depend on %s (%s -> %s)' %\
                    (target.ttype, targets[tdep].ttype, target.fullname, tdep))
for t in targets:
    lintDeps(targets[t])
    deps = []
    traverseDeps(targets[t], [], all_types, lambda tgt,deps: deps.append(tgt.fullname), deps, True)
    depSet = set(deps)
    for d in deps:
        for c in targets[d].conflicts_with:
            if c in depSet:
                dpath = allpaths(t,d)[0]
                cpath = allpaths(t,c)[0]
                raise Exception('%s requires both %s and %s, which conflict.\nPaths: %s %s' %
                        (t, d, c, str(dpath), str(cpath)))
    # TODO detect static linked as static and through shared

targetnames = [t for t in targets]
targetnames.sort()

if options.list_make_targets:
    print '"make" targets for the current PRODUCT:'
    for t in targetnames:
        print '  ' + t.replace('/:','/').replace(':', '/')[1:]

if options.list_makegen_targets:
    print 'Makegen targets (for making references within BUILD files):'
    for t in targetnames:
        print '  ' + t

if options.outdir != None:
    if (os.access(options.outdir + '/Makefile', os.F_OK)):
        os.remove(options.outdir + '/Makefile')
    makefile = file(options.outdir + '/Makefile.temp', 'w')
    makefile.write(
            '#\n'
            '# NOTE: Do not modify this file directly; it is generated by makegen.py.\n'
            '#   To make changes, you need to either update the BUILD files in the source\n'
            '#   tree or update makegen itself.\n'
            '#\n'
            'ifndef SRCROOT\n'
            '$(error It looks like you tried to run "make" from within your BUILDROOT; please run "make" from your source tree instead)\n'
            'endif\n'
            'export SRCROOT\n'
            'export BUILDROOT\n'
            'export BUILDTYPE\n'
            'export PBROOT\n'
            'export PRODUCT_VARS\n'
            'export $(PRODUCT_VARS)\n'
            '# Start with a clean environment (needed when we are being called as part of sw_i build)\n'
            'CFLAGS := \n'
            'CXXFLAGS := \n'
            'C_CXX_FLAGS := \n'
            'CINCLUDES := \n'
            'CXXINCLUDES := \n'
            'CXX_ONLY_FLAGS := \n'
            'GCC_WARNINGS := \n'
            'GXX_WARNINGS := \n'
            'GCC_GXX_WARNINGS := \n'
            'LDFLAGS := \n'
            'ARFLAGS := \n'
            '.SUFFIXES: # Remove all built-in suffix rules\n'
            '\n'
            'DEFS_FILES := $(SRCROOT)/make/makegen.py $(SRCROOT)/make/%(platform)s_defs.mk\n'
            '\n'
            'include $(SRCROOT)/make/%(platform)s_defs.mk\n'
            '\n'
            '.PHONY: __FORCE\n'
            '__FORCE:\n'
            '\n' %
            { 'platform': env('PLATFORM',None)
            })
    if default_target != '':
        makefile.write('default: %s\n\n' % (targets[default_target].fullname.replace(':', '/')))
    for t in targetnames:
        try:
            makefile.write('##### ' + t + '\n')
            targets[t].gen_make(makefile)
            makefile.write('\n')
        except:
            print '*** Error generating rules for ' + t
            raise
    
    makedeps = file(options.outdir + '/Make.deps', 'w')
    makedeps.write(
            '#\n'
            '# NOTE: Do not modify this file directly; it is generated by makegen.py.\n'
            '#   To make changes, you need to either update the BUILD files in the source\n'
            '#   tree or update makegen itself.\n'
            '#\n'
            'BUILD_FILES = %s\n'
            '$(PBROOT)/Makefile $(PBROOT)/Make.deps: $(BUILD_FILES)\n'
            '\n'
            '%s\n' %
            (str.join(' ', files_read), str.join('\n\n', [f+':' for f in files_read])))
    os.rename(options.outdir + '/Makefile.temp', options.outdir + '/Makefile')
