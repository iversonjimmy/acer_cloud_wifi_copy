import web

TMP_FILEDIR = '/var/www/upload'
TMP_FILENAME = 'myfile'

urls = (
    '/checkmethods', 'MethodsTest',
    '/download', 'DownloadLargeFileTest',
    '/cut_short', 'CutShortTest',
    '/upload', 'Upload',
)


class MethodsTest:
    def HEAD(self):
        return {'method': 'HEAD'} ### no use
    def GET(self):
        debug_print_request()
        return {'method': 'GET'}
    def POST(self):
        debug_print_request()
        data = web.data()
        return {'method': 'POST', 'data': data}
    def PUT(self):
        data = web.data()
        return {'method': 'PUT', 'data': data}
    def DELETE(self):
        return {'method': 'DELETE'}


class DownloadLargeFileTest:
    def GET(self):
        web.header("Content-Type", "text/plain")
        for x in range(1024):
            for y in range(100):
                tmp_c = "%d" % (y%10)
                tmp_s = ""
                for z in range(1024):
                    tmp_s += tmp_c
                yield tmp_s


class CutShortTest:
    def GET(self):
        web.header("Content-Type", "text/html")
        web.header("Content-Length", "10000")
        return '<html><head><title>Cut Short Response Test</title></head>'
        
class Upload:
    def GET(self):
        try:  
            debug_print_request()
            web.header("Content-Type","text/html; charset=utf-8")
            web.debug('filenddame : ' + TMP_FILENAME);
            fopen = open(TMP_FILEDIR + '/' + TMP_FILENAME );
            return fopen.read()
        except:
            return '' 
    def POST(self):
        debug_print_request()
        rc = do_upload()
        if rc == 0:
            return 'Upload done.'
        else: 
            return 'Upload failed.'

    def PUT(self):
        debug_print_request()
        rc = do_upload()
        if rc == 0:
            return 'Upload done.'
        else:
            return 'Upload failed.'

def debug_print_request():
    req_body_size = int(web.ctx.env.get('CONENT_LENGTH', 0)) 
    e = web.ctx.env

#    web.debug(web.ctx.env['wsgi.input'].read(req_body_size))
    web.debug("repr:" + str(web.ctx.env.__repr__()) )
    web.debug("server protocol: " + e.get('SERVER_PROTOCOL', '') )
    web.debug("http host:       " + e.get('HTTP_HOST', '') )
    web.debug("request uri:     " + e.get('REQUEST_URI' '' ) )
    web.debug("port:            " + e.get('SERVER_PORT' '' ) )
    web.debug("request method:  " + e.get('REQUEST_METHOD', '') )
    web.debug("script_name:     " + e.get('SCRIPT_NAME', '') )
    web.debug("path_info:       " + e.get('PATH_INFO', '') )
    web.debug("query_string:    " + e.get('QUERY_STRING', '') )

def do_upload():

    #### MAIN LOGIC ####
    z = web.webapi.data()
    try:
            #filename = x.get('1234', 'myfile')
            #filepath=x.myfile.filename.replace('\\','/') # replaces the windows-style slashes with linux ones.
            
#            filename=filepath.split('/')[-1] # splits the and chooses the last part (the filename with extension)
        fout = open(TMP_FILEDIR +'/'+TMP_FILENAME,'wb+') # creates the file where the uploaded file should be stored
        fout.write(z) # writes the uploaded file to the newly created file.
        fout.close() # closes the file, upload complete.
        raise web.seeother('/upload')
    except KeyError:
        pass
    return 0


def notfound():
    return web.notfound("{'error': 'Page not found'}")
    

def internalerror():
    return web.internalerror("{'error': 'Internal Error'}")


application = web.application(urls, globals()).wsgifunc()
application.notfound = notfound
application.internalerror = internalerror
