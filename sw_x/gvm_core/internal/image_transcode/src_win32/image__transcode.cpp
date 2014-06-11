#include "image_transcode.h"
#include "image__transcode.h"

#include <vpl_types.h>
#include <vplu_format.h>
#include <log.h>
#include <Thumbcache.h>
#include <atlimage.h>
#include <strsafe.h>

#include "aarot.hpp"

#define ENCODER_CLSID_NAME_JPG L"image/jpeg"
#define ENCODER_CLSID_NAME_TIFF L"image/tiff"
#define ENCODER_CLSID_NAME_PNG L"image/png"
#define ENCODER_CLSID_NAME_BMP L"image/bmp"
#define ENCODER_CLSID_NAME_GIF L"image/gif"
#define DEFAULT_COMPRESS_RATE 50

#define PropertyKey_Orientation L"System.Photo.Orientation"

using namespace Gdiplus;

static HRESULT GetScaledBitmap(LPWSTR pszFilename, HBITMAP &hBmpImage, int cXY);
static HRESULT DecodeImageFromThumbCache(IShellItem *pShellItem, HBITMAP &thumbnail, int cXY);
static int GetEncoderClsid(const wchar_t* format, CLSID* pClsid);
static int utf8_to_wstring(const char *utf8, wchar_t** wstring);
static bool isWin8();
static int getPhotoOrientation(const LPWSTR lpszPath);
static HBITMAP ImageMirror(HBITMAP hBitmap);

static ULONG_PTR gdiplus_startup_token;

int ImageTranscode_init_private()
{
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplus_startup_token, &gdiplusStartupInput, NULL);
    return IMAGE_TRANSCODING_OK;
}

int ImageTranscode_transcode_private(std::string filepath,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    int rc = IMAGE_TRANSCODING_OK;
    HRESULT result;
    HBITMAP hbitmap = NULL;
    wchar_t* tfilepath = NULL;
    unsigned char* buf = NULL;
    GUID raw_format_guid;
    Image* image = NULL;
    size_t org_width = 0;
    size_t org_height = 0;
    size_t max_wh_size = 0;
    double r1;
    double r2;
    Bitmap* bitmap;
    IStream* stream = NULL;
    CLSID clsid;
    ULONG quality = DEFAULT_COMPRESS_RATE;
    Status saveStatus;

    if (width == 0 || height == 0) {
        return IMAGE_TRANSCODING_BAD_PARAMETER;
    }

    CoInitialize(NULL);

    for (unsigned int i = 0; i < filepath.size(); i++) {
        if (filepath[i] == '/') {
            filepath[i] = '\\';
        }
    }
    utf8_to_wstring(filepath.c_str(), &tfilepath);

    // Get the width and height of original file
    image = Image::FromFile(tfilepath, FALSE);
    org_width = image->GetWidth();
    org_height = image->GetHeight();
    image->GetRawFormat(&raw_format_guid);
    delete image;

    r1 = (double)org_height / org_width;
    r2 = (double)height / width;

    if (org_width == 0 || org_height == 0) {
        LOG_ERROR("Failed to get original image size, org_width = "FMTu_size_t", org_height = "FMTu_size_t"", org_width, org_height);
        rc = IMAGE_TRANSCODING_FAILED;
        goto failed_to_get_org_img_size;
    }

    // org_width > org_height
    if (r1 < 1) {
        if (r1 <= r2) {
            max_wh_size = width;
        } else {
            max_wh_size = org_width * height / org_height;
        }
    // org_width <= org_height
    } else {
        if (r1 >= r2) {
            max_wh_size = height;
        } else {
            max_wh_size = org_height * width / org_width;
        }
    }

    result = GetScaledBitmap(tfilepath, hbitmap, max_wh_size);
    if (result != S_OK || hbitmap == NULL) {
        LOG_ERROR("Failed to transcode, HRESULT = %d", result);
        rc = IMAGE_TRANSCODING_FAILED;
        goto failed_to_scale;
    }

    if (isWin8() && raw_format_guid != Gdiplus::ImageFormatTIFF) {
        HBITMAP hRotate = NULL;
        HBITMAP hMirror = NULL;
        int orientation = getPhotoOrientation(tfilepath);
        aarot _aarot;
        CImage image;

        LOG_DEBUG("Is win8, need to rotate if orientation is not 0 nor 1.");
        switch (orientation) {
            case 0:
            case 1:
                image.Attach(hbitmap);
                break;
            case 2:
                hMirror = ImageMirror(hbitmap);
                image.Attach(hMirror);
                break;
            case 3:
                hRotate = _aarot.rotate(hbitmap, 180, NULL);
                image.Attach(hRotate);
                break;
            case 4:
                hMirror = ImageMirror(hbitmap);
                hRotate = _aarot.rotate(hMirror, 180, NULL);
                image.Attach(hRotate);
                break;
            case 5:
                hMirror = ImageMirror(hbitmap);
                hRotate = _aarot.rotate(hMirror, 90, NULL);
                image.Attach(hRotate);
                break;
            case 6:
                hRotate = _aarot.rotate(hbitmap, 90, NULL);
                image.Attach(hRotate);
                break;
            case 7:
                hMirror = ImageMirror(hbitmap);
                hRotate = _aarot.rotate(hMirror, -90, NULL);
                image.Attach(hRotate);
                break;
            case 8:
                hRotate = _aarot.rotate(hbitmap, -90, NULL);
                image.Attach(hRotate);
                break;
        }

        hbitmap = image.Detach();
    }

    bitmap = Bitmap::FromHBITMAP(hbitmap, NULL);

    result = CreateStreamOnHGlobal(NULL, VPL_TRUE, (LPSTREAM*)&stream);
    if (result != S_OK) {
        LOG_ERROR("Failed to create stream, HRESULT = %d", result);
        rc = IMAGE_TRANSCODING_FAILED;
        goto failed_to_create_stream;
    }

    switch(type) {
        case ImageType_JPG:
            GetEncoderClsid(ENCODER_CLSID_NAME_JPG, &clsid);
            break;
        case ImageType_PNG:
            GetEncoderClsid(ENCODER_CLSID_NAME_PNG, &clsid);
            break;
        case ImageType_TIFF:
            GetEncoderClsid(ENCODER_CLSID_NAME_TIFF, &clsid);
            break;
        case ImageType_BMP:
            GetEncoderClsid(ENCODER_CLSID_NAME_BMP, &clsid);
            break;
        case ImageType_GIF:
            GetEncoderClsid(ENCODER_CLSID_NAME_GIF, &clsid);
            break;
        case ImageType_Original:
        default:
            // To use the original image type
            if (raw_format_guid == Gdiplus::ImageFormatJPEG) {
                GetEncoderClsid(ENCODER_CLSID_NAME_JPG, &clsid);
            } else if (raw_format_guid == Gdiplus::ImageFormatPNG) {
                GetEncoderClsid(ENCODER_CLSID_NAME_PNG, &clsid);
            } else if (raw_format_guid == Gdiplus::ImageFormatTIFF) {
                GetEncoderClsid(ENCODER_CLSID_NAME_TIFF, &clsid);
            } else if (raw_format_guid == Gdiplus::ImageFormatBMP) {
                GetEncoderClsid(ENCODER_CLSID_NAME_BMP, &clsid);
            } else if (raw_format_guid == Gdiplus::ImageFormatGIF) {
                GetEncoderClsid(ENCODER_CLSID_NAME_GIF, &clsid);
            } else {
                // Unknown original image type.
                LOG_WARN("Unknown original image type, change to use \"image/jpeg\"");
                GetEncoderClsid(ENCODER_CLSID_NAME_JPG, &clsid);
            }
            break;
    }

    // Setup encoder parameters
    EncoderParameters encoderParameters;
    encoderParameters.Count = 1;
    encoderParameters.Parameter[0].Guid = EncoderQuality;
    encoderParameters.Parameter[0].Type = EncoderParameterValueTypeLong;
    encoderParameters.Parameter[0].NumberOfValues = 1;
    encoderParameters.Parameter[0].Value = &quality; // setup compression level

    //  Save the image to the stream
    saveStatus = bitmap->Save(stream, &clsid, &encoderParameters);
    if (saveStatus != Ok) {
        LOG_WARN("Failed to save into stream.");
        rc = IMAGE_TRANSCODING_FAILED;
        goto stream_failed;
    }

    // get the size of the stream
    ULARGE_INTEGER ulnSize;
    LARGE_INTEGER lnOffset;
    lnOffset.QuadPart = 0;
    if (stream->Seek(lnOffset, STREAM_SEEK_END, &ulnSize) != S_OK) {
        LOG_WARN("Failed to seek stream.");
        rc = IMAGE_TRANSCODING_FAILED;
        goto stream_failed;
    }

    // now move the pointer to the begining of the file
    if(stream->Seek(lnOffset, STREAM_SEEK_SET, NULL) != S_OK) {
        LOG_WARN("Failed to seek stream.");
        rc = IMAGE_TRANSCODING_FAILED;
        goto stream_failed;
    }

    out_image_len = (size_t) ulnSize.QuadPart;
    buf = new unsigned char[out_image_len];
    *out_image = buf;
    ULONG ulBytesRead;
    if (stream->Read(buf, out_image_len, &ulBytesRead) != S_OK) {
        LOG_WARN("Failed to read from stream.");
        rc = IMAGE_TRANSCODING_FAILED;
        free(buf);
        *out_image = NULL;
        out_image_len = 0;
    }

 stream_failed:
    stream->Release();
 failed_to_create_stream:
    DeleteObject(hbitmap);
    delete bitmap;
 failed_to_scale:
 failed_to_get_org_img_size:
    free(tfilepath);
    CoUninitialize();
    return rc;
}

int ImageTranscode_transcode_private(const unsigned char* image,
                                     size_t image_len,
                                     ImageTranscode_ImageType type,
                                     size_t width,
                                     size_t height,
                                     unsigned char** out_image,
                                     size_t& out_image_len)
{
    // Currently, not support on win32.
    return IMAGE_TRANSCODING_NOT_SUPPORT;
}

int ImageTranscode_shutdown_private()
{
    GdiplusShutdown(gdiplus_startup_token);
    return IMAGE_TRANSCODING_OK;
}


static int utf8_to_wstring(const char *utf8, wchar_t** wstring)
{
    int nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
    if (nwchars == 0) {
        return -1;
    }

    wchar_t* buf = (wchar_t*) malloc(nwchars * sizeof(wchar_t));
    if (buf == NULL) {
        return -1;
    }

    nwchars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, buf, nwchars);
    if (nwchars == 0) {
        free(buf);
        return -1;
    }

    *wstring = buf;
    return 0;
}

static HRESULT GetScaledBitmap(LPWSTR pszFilename,
                               HBITMAP &hBitmap,
                               int cXY) {
    HRESULT hr = S_OK;

    // Walk the Pictures library
    IShellItem* pShellItem;

    hr = SHCreateItemFromParsingName(pszFilename,
                                     NULL,
                                     IID_PPV_ARGS(&pShellItem));

    if (SUCCEEDED(hr)) {
        INamespaceWalk* pNamespaceWalk;
        hr = CoCreateInstance(CLSID_NamespaceWalker,
                              NULL,
                              CLSCTX_INPROC,
                              IID_PPV_ARGS(&pNamespaceWalk));

        if (SUCCEEDED(hr)) {
            hr = pNamespaceWalk->Walk(pShellItem,
                                      NSWF_NONE_IMPLIES_ALL,
                                      1,
                                      NULL);
            if (SUCCEEDED(hr)) {
                // Retrieve the array of PIDLs gathered in the walk
                UINT itemCount;
                PIDLIST_ABSOLUTE* ppidls;
                hr = pNamespaceWalk->GetIDArrayResult(&itemCount, &ppidls);
                if (SUCCEEDED(hr)) {
                    // Create the uninitialized thumbnails
                    // Get the bitmap for each item and initialize the corresponding thumbnail object
                    for (UINT i = 0; i < itemCount; i++) {
                        IShellItem* pShellItem;
                        hr = SHCreateItemFromIDList(ppidls[i], IID_PPV_ARGS(&pShellItem));
                        if (SUCCEEDED(hr)) {
                            hr = DecodeImageFromThumbCache(pShellItem, hBitmap, cXY);
                            pShellItem->Release();
                        }
                    }

                    // The calling function is responsible for freeing the PIDL array
                    FreeIDListArray(ppidls, itemCount);
                } else {
                    LOG_ERROR("Failed to walk to get the array of PIDLs, result = %d", hr);
                }
            } else {
                LOG_ERROR("Failed to walk to the shell item, result = %d", hr);
            }
            pNamespaceWalk->Release();
        } else {
            LOG_ERROR("Failed to create instance of INamespaceWalk, result = %d", hr);
        }

        pShellItem->Release();
    } else {
        LOG_ERROR("Failed to create instance of IShellItem, result = %d", hr);
    }

    return hr;
}

static HRESULT DecodeImageFromThumbCache(IShellItem* pShellItem,
                                         HBITMAP &thumbnail,
                                         int cXY) {
    // Read the bitmap from the thumbnail cache
    IThumbnailCache* pThumbCache;
    HRESULT hr = CoCreateInstance(CLSID_LocalThumbnailCache,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&pThumbCache));
    if (SUCCEEDED(hr)) {
        ISharedBitmap* pBitmap = NULL;
        hr = pThumbCache->GetThumbnail(pShellItem,
                                       cXY,
                                       WTS_EXTRACT | WTS_SCALETOREQUESTEDSIZE,
                                       &pBitmap,
                                       NULL,
                                       NULL);

        if (SUCCEEDED(hr) && pBitmap != NULL) {
            hr = pBitmap->GetSharedBitmap(&thumbnail);

            if (!SUCCEEDED(hr)) {
                LOG_ERROR("Failed to get shared bitmap, result = %d", hr);
            } else if (thumbnail == NULL) {
                LOG_ERROR("Failed to get shared bitmap, thumbnail == NULL");
            }
        } else {
            LOG_ERROR("Failed to get thumbnail, result = %d", hr);
        }

        pThumbCache->Release();
    } else {
        LOG_ERROR("Failed to create instance of IThumbnailCache, result = %d", hr);
    }

    return hr;
}

static int GetEncoderClsid(const wchar_t* format, CLSID* pClsid) {
    UINT num = 0;  // number of image encoders
    UINT size = 0; // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0) {
        return -1; // Failure
    }
    pImageCodecInfo = (ImageCodecInfo*) (malloc(size));
    if (pImageCodecInfo == NULL)
        return -1; // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j; // Success
        }
    }

    free(pImageCodecInfo);
    return -1; // Failure
}

static bool isWin8()
{
    DWORD dwVersion = 0;
    DWORD dwMajorVersion = 0;
    DWORD dwMinorVersion = 0;

    dwVersion = GetVersion();

    // Get the Windows version.
    dwMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));
    dwMinorVersion = (DWORD)(HIBYTE(LOWORD(dwVersion)));

    bool bWin8 = dwMajorVersion == 6 && dwMinorVersion >= 2;

    return bWin8;
}

static int getPhotoOrientation(const LPWSTR lpszPath)
{
    int nRet = 0;
    IPropertyStore* pps = NULL;
    WCHAR szExpanded[MAX_PATH];
    HRESULT hr = ExpandEnvironmentStrings(lpszPath, szExpanded, ARRAYSIZE(szExpanded)) ? S_OK : HRESULT_FROM_WIN32(GetLastError());

    if (SUCCEEDED(hr))
    {
        WCHAR szAbsPath[MAX_PATH];

        hr = _wfullpath(szAbsPath, szExpanded, ARRAYSIZE(szAbsPath)) ? S_OK : E_FAIL;

        if (SUCCEEDED(hr)) {
            hr = SHGetPropertyStoreFromParsingName(szAbsPath, NULL, GPS_DEFAULT, IID_PPV_ARGS(&pps));

            if (SUCCEEDED(hr)) {
                PROPVARIANT propvarValue = {0};
                PROPERTYKEY key;

                hr = PSGetPropertyKeyFromName(PropertyKey_Orientation, &key);

                if (SUCCEEDED(hr)) {
                    hr = pps->GetValue(key, &propvarValue);

                    if (SUCCEEDED(hr)) {
                        nRet = propvarValue.uintVal;
                    }
                }

                PropVariantClear(&propvarValue);
            }
        }
    }

    if (pps) {
        pps->Release();
    }

    return nRet;
}

static HBITMAP ImageMirror(HBITMAP hBitmap)
{
    // Create a memory DC compatible with the display
    HDC sourceDC, destDC, tmpDC;

    tmpDC = GetDC(NULL);
    sourceDC = CreateCompatibleDC(NULL);
    destDC = CreateCompatibleDC(NULL);

    // Get logical coordinates
    BITMAP bm;
    ::GetObject( hBitmap, sizeof( bm ), &bm );

    // Create a bitmap to hold the result
    HBITMAP hbmResult = ::CreateCompatibleBitmap(tmpDC, bm.bmWidth, bm.bmHeight);

    // Select bitmaps into the DCs
    HBITMAP hbmOldSource = (HBITMAP)::SelectObject( sourceDC, hBitmap );
    HBITMAP hbmOldDest = (HBITMAP)::SelectObject( destDC, hbmResult );

    StretchBlt(destDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceDC, bm.bmWidth-1, 0, -bm.bmWidth, bm.bmHeight, SRCCOPY );
    //StretchBlt(destDC, 0, 0, bm.bmWidth, bm.bmHeight, sourceDC, 0, bm.bmHeight-1, bm.bmWidth, -bm.bmHeight, SRCCOPY );

    // Reselect the old bitmaps
    ::SelectObject( sourceDC, hbmOldSource );
    ::SelectObject( destDC, hbmOldDest );

    DeleteObject(hbmOldSource);
    DeleteObject(hbmOldDest);
    DeleteObject(tmpDC);
    return hbmResult;
}

