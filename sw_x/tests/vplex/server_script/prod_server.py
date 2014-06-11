import web
import web.app_common as app
from web.app_common import *

urls = (
    '/checkmethods', 'MethodsTest',
    '/download', 'DownloadLargeFileTest',
    '/cut_short', 'CutShortTest',
    '/upload', 'Upload',
    '/upload/(.*)', 'UploadProduct',
)

application = web.application(urls, globals()).wsgifunc()
application.notfound = notfound
application.internalerror = internalerror
