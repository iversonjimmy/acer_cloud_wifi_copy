targetlogin = 'build'
targetos = 'win32'
logdir = '.'
testroot = '/temp/igware/testroot'
testrootpath = { 'win32' : '/cygdrive/c%s' % testroot,
	         'linux' : testroot }
binext = { 'win32' : '.exe',
       	   'linux' : ''}
ccdrootpath = { 'win32' : '/cygdrive/c/temp/igware/ccd/vs', 
	        'linux' : '/temp/igware/ccd/vs' }

def cloudRoot(cygwin):
    global targetos
    global targetlogin
    if targetos == 'win32':
        if cygwin == True:
            path = '/cygdrive/c/Users/' + targetlogin + '/\"My Cloud\"'
        else:
            path = 'c:\\Users\\' + targetlogin + '\\My Cloud' 
    else:
        path = '/home/' + targetlogin + '/\"My Cloud\"'
        
    return path
