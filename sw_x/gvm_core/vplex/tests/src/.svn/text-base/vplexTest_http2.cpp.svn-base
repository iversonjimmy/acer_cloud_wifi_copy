#include <string>
#include <iostream>
#include <fstream>

#include "vpl_fs.h"
#include "vplex_file.h"
#include "vplex_http2.hpp"
#include "vplexTest.h"

#define TEST_TRUSTED_CERT_URL "https://www.cloud.acer.com/ops/login"
#define TEST_UNTRUSTED_CERT_URL "https://login.acer.com.tw/"

#ifdef VPL_PLAT_IS_WINRT
const char* download_url = "http://vanadium.acer.com.tw/test/download";
const char* cut_short_url = "http://10.36.141.93:8080/cut_short";

using namespace Platform;
#else
extern const char* download_url;
extern const char* cut_short_url;
#endif

#ifdef ANDROID
//#define ROOT_PATH    "/mnt/sdcard/"
#define ROOT_PATH    "/sdcard/"
#elif defined(_WIN32)
#ifndef VPL_PLAT_IS_WINRT
// For now, you need to manually create this path first.
#define ROOT_PATH   "C:\\tmp\\"
#endif
#elif defined(IOS)
extern "C" {
    const char* getRootPath();
}
#else
#define ROOT_PATH    "/tmp/"
#endif

extern const char* server_url;
extern const char* branch;
extern const char* product;

const std::string upload_path = "upload";
const std::string check_method_path = "checkmethods";
const char* cut_short = "cut_short";

/*
 *  Create large size file.
 */
static int create_file(const char* filepath, size_t size)
{
   int rv = VPL_OK;
   std::ofstream newfile;
   newfile.open(filepath, std::ios::trunc | std::ios::out );
   if(newfile.fail()) {
       VPLTEST_LOG("unable to open file: %s", filepath );
       rv = VPL_ERR_FAIL;
       goto end;
   }

   for(int i = 0; i < (int)size; i++) {
       char c =  '0' + (i / 1024) % 10;
       newfile << c;
   }

end:
   if(newfile.is_open()) {
       newfile.close();
   }
   return rv;
}

/*
 * Compare files.
 */
static int file_compare(const char* src, const char* dst)
{
    int rv = VPL_OK;
    const int BUFF_SIZE = 16*1024;
    VPLFile_handle_t handle_src = 0, handle_dst = 0;
    char* src_buf = NULL;
    char* dst_buf = NULL;
    ssize_t src_read, dst_read;
    ssize_t total_read = 0;

    // Null check
    if( src == NULL || dst == NULL ){
        VPLTEST_LOG("file path is NULL.");
        rv = VPL_ERR_FAIL;
        goto end;
    }

    // Open file
    handle_src = VPLFile_Open(src, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_src) ) {
        VPLTEST_LOG("cannot open src file: %s", src);
        rv = VPL_ERR_FAIL;
        goto end;
    }

    handle_dst = VPLFile_Open(dst, VPLFILE_OPENFLAG_READONLY, 0);
    if (!VPLFile_IsValidHandle(handle_dst) ) {
        VPLTEST_LOG("cannot open dst file: %s", dst);
        rv = VPL_ERR_FAIL;
        goto end;
    }

    src_buf = new char[BUFF_SIZE];
    if (src_buf == NULL) {
        VPLTEST_LOG("cannot allocate memory for src buffer: %s", src);
        rv = VPL_ERR_FAIL;
        goto end;
    }

    dst_buf = new char[BUFF_SIZE];
    if (dst_buf == NULL) {
        VPLTEST_LOG("cannot allocate memory for dst buffer: %s", dst);
        rv = VPL_ERR_FAIL;
        goto end;
    }
    do {
        // TODO: Note that documentation for POSIX read() states that it can legitimately
        //   return less than BUFF_SIZE before the end-of-file is reached.  However, it seems
        //   like this rarely happens in practice.  Just something to look out for if this
        //   block fails unexpectedly.
        src_read = VPLFile_Read(handle_src, src_buf, BUFF_SIZE);
        dst_read = VPLFile_Read(handle_dst, dst_buf, BUFF_SIZE);
        if(src_read < 0) {
            VPLTEST_LOG("error while reading src: %s, %d", src, (int)src_read);
            rv = VPL_ERR_FAIL;
            goto end;
        }

        if(dst_read < 0) {
            VPLTEST_LOG("error while reading dst: %s, %d", dst, (int)dst_read);
            rv = VPL_ERR_FAIL;
            goto end;
        }
        if(src_read != dst_read) {
            VPLTEST_LOG("At offset "FMT_ssize_t": file sizes are different (src_read="FMT_ssize_t", dst_read="FMT_ssize_t")!",
                    total_read, src_read, dst_read);
            rv = VPL_ERR_FAIL;
            goto end;
        }
        if(memcmp(src_buf, dst_buf, src_read) ) {
            VPLTEST_LOG("At offset "FMT_ssize_t"-"FMT_ssize_t": file content is different!",
                    total_read, (total_read + src_read));
            rv = VPL_ERR_FAIL;
            goto end;
        }
        total_read += src_read;
    } while (src_read > 0);

end:
    if (VPLFile_IsValidHandle(handle_src) ) {
        VPLFile_Close(handle_src);
    }
    if (VPLFile_IsValidHandle(handle_dst) ) {
        VPLFile_Close(handle_dst);
    }
    if ( src_buf != NULL ) {
        delete[] src_buf;
        src_buf = NULL;
    }
    if ( dst_buf != NULL ) {
        delete[] dst_buf;
        dst_buf = NULL;
    }
    return rv;
}

static void clean_up(const char* filename ) {
    VPLFS_stat_t filestat;

    if( !VPLFS_Stat(filename, &filestat) && (filestat.type == VPLFS_TYPE_FILE) ) {
        remove(filename);
    }else{
        VPLTEST_LOG("fail to clean file: %s", filename);
    }
}

static int simple_get_test(const char* url, std::string &response)
{
    int rv = VPL_OK;
    std::string uri(url);
    VPLHttp2 http;
    http.SetUri(uri.append(check_method_path));
    http.SetDebug(1);
    rv = http.Get(response);

    if (rv < 0) {
        VPLTEST_LOG("fail to send GET request: %s", uri.c_str());
        goto end;
    }
    // More checks go here.

end:
    return rv;
}

static int simple_put_test(const char* url, std::string &path, std::string &response)
{
    int rv = VPL_OK;
    std::string uri(url);
    uri.append(upload_path).append("/").append(product);
    VPLHttp2 http;

    http.SetUri(uri);
    http.SetDebug(1);
    rv = http.Put(path, NULL, NULL, response);

    if ( rv < 0) {
         VPLTEST_LOG("fail to PUT file");
    }

    return rv;
}

static int put_test_file_doesnt_exist(const char* url, std::string &path)
{
    int rv = VPL_OK;
    std::string uri(url);
    uri.append(upload_path).append("/").append(product);

    VPLHttp2 http;
    http.SetUri(uri);
    http.SetDebug(1);
    std::string dummyResponse;
    rv = http.Put(path + "_does_not_exist", NULL, NULL, /*out*/dummyResponse);
    return rv;
}

static int get_file_test(const char * url, std::string &path, std::string &response){
    int rv = VPL_OK;
    std::string uri(url);
    uri.append(upload_path).append("/").append(product);
    VPLHttp2 http;

    std::string resp_filepath(path);
    resp_filepath.append(".resp");

    http.SetUri(uri);
    http.SetDebug(1);
    rv = http.Get(resp_filepath, NULL, NULL  );

    if( rv != VPL_OK ) {
         VPLTEST_LOG("Fail to download file!");
         goto end;
    }

    rv = file_compare(path.c_str(), resp_filepath.c_str());

    if( rv != VPL_OK ) {
         VPLTEST_LOG("Downloaded content is not correct!");
         goto end;
    }

end:
    return rv;
}

static int response_cutshort_test(const char * url, std::string &response)
{
    int rv = VPL_OK;
    std::string uri(url);
    VPLHttp2 http;

    http.SetUri(uri.append(cut_short));
    http.SetDebug(1);
    rv = http.Get(response);

    if (rv < 0) {
        VPLTEST_LOG("fail to send GET request");
        goto end;
    }
    // More checks go here.

end:
    return rv;
}


/*
 *  1. Send a GET request.
 *  2. Upload file with PUT request.
 *  3. Download file with GET request.
 */
void testVPLHttp(void)
{
    std::string response;
    std::string http_url("http://");
    http_url.append(server_url).append("/").append(branch).append("/");
    std::string url = http_url;

#ifdef VPL_PLAT_IS_WINRT 
    auto wLogPath = Windows::Storage::ApplicationData::Current->LocalFolder->Path + ref new String(L"\\");
    char* tmpPath;

    _VPL__wstring_to_utf8_alloc(wLogPath->Data(), &tmpPath);

    std::string filepath( std::string(tmpPath) + std::string("test_tmp") );
    free(tmpPath);
#elif defined(IOS)
    std::string filepath( getRootPath() + std::string("/test_tmp") );
#else
    std::string filepath( std::string(ROOT_PATH) + std::string("test_tmp") );
#endif

    create_file(filepath.c_str(), 100000);

    VPLTEST_CALL_AND_CHK_RV(simple_get_test(url.c_str(), response) ,VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(simple_put_test(url.c_str(), filepath, response) ,VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(get_file_test(url.c_str(), filepath, response) ,VPL_OK);
    VPLTEST_CALL_AND_CHK_RV(response_cutshort_test(url.c_str(), response), VPL_ERR_RESPONSE_TRUNCATED);

#if 0
    // TODO: bug 16475: fix the problem, then enable this instead of the #else block.
    VPLTEST_CALL_AND_CHK_RV(put_test_file_doesnt_exist(url.c_str(), filepath), VPL_ERR_NOENT);
#else
    {
        int temp_rc = put_test_file_doesnt_exist(url.c_str(), filepath);
        if (temp_rc != VPL_ERR_NOENT) {
            VPLTEST_LOG("Expected to fail: put_test_file_doesnt_exist() returned %d, should be %d (VPL_ERR_NOENT)", temp_rc, VPL_ERR_NOENT);
        } else {
            VPLTEST_LOG("put_test_file_doesnt_exist() correctly returned VPL_ERR_NOENT");
        }
    }
#endif

    clean_up(filepath.c_str());
    clean_up(std::string(filepath + std::string(".resp") ).c_str() ) ;

    return;
}


