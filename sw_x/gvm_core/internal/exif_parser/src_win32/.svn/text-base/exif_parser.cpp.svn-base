#include <vplex_time.h>
#include <vpl_plat.h>
#include <log.h>
#include "exif_parser.h"
#include "exif_util.h"
#include <string>
#include <Propsys.h>
#include <Propvarutil.h>
#include <propkey.h>
#include <vpl_error.h>
#include <windows.h>
#include <Shobjidl.h>
#include <time.h>


using namespace std;

#define CCDPropertyKey_TakenDate        L"System.Photo.DateTaken"

std::string cleanupPath(const char* path)
{

    std::string result;
    bool lastWasSlash = false;
    for (; *path != '\0'; path++) {
        char curr = *path;
        if (curr == '/' || curr == '\\') {
            if (!lastWasSlash) {
                result.append("\\");
                lastWasSlash = true;
            }
        } else {
            result.append(1, curr);
            lastWasSlash = false;
        }
    }
    return result;
}


int EXIFGetImageTimestamp(const string& filename,
                          VPLTime_CalendarTime& dateTime,
                          VPLTime_t& timestamp)
{
    int rv = VPL_ERR_FAIL;
    wchar_t *wfilename = NULL;
    HRESULT hr;
    IPropertyStore* pps = NULL;
    bool comInitialized = false;
    string filename_clean;
   
    if (filename.empty()) {
        LOG_ERROR("No file specified");
        goto end;
    }

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        LOG_ERROR("Fail to initialize COM 0x%x", (int)hr);
        goto end;
    }
    comInitialized = true;

    memset(&dateTime, 0, sizeof(dateTime));

    filename_clean = cleanupPath(filename.c_str());
    LOG_INFO("Image: %s", filename_clean.c_str());

    _VPL__utf8_to_wstring(filename_clean.c_str(), &wfilename);

    hr = SHGetPropertyStoreFromParsingName(wfilename, NULL, GPS_DEFAULT, IID_PPV_ARGS(&pps));
    if (SUCCEEDED(hr)) {
        PROPVARIANT propvarValue = {0};
        PROPERTYKEY key;

        hr = PSGetPropertyKeyFromName(CCDPropertyKey_TakenDate, &key);
        if (SUCCEEDED(hr)) {
            hr = pps->GetValue(key, &propvarValue);
            if (SUCCEEDED(hr)) {
                FILETIME ft = propvarValue.filetime; 

                if (ft.dwHighDateTime) {
                    SYSTEMTIME sysTime; 

                    if (FileTimeToSystemTime(&ft, &sysTime)) {
                        struct tm temp;
                        SYSTEMTIME sysTimeLocal;

                        if (SystemTimeToTzSpecificLocalTime(NULL, &sysTime, &sysTimeLocal)) {

                            dateTime.year = sysTimeLocal.wYear;
                            dateTime.month = sysTimeLocal.wMonth;
                            dateTime.day = sysTimeLocal.wDay;
                            dateTime.hour = sysTimeLocal.wHour;
                            dateTime.min = sysTimeLocal.wMinute;
                            dateTime.sec = sysTimeLocal.wSecond;

                            temp.tm_year = dateTime.year - 1900;
                            temp.tm_mon = dateTime.month - 1;
                            temp.tm_mday = dateTime.day;
                            temp.tm_hour = dateTime.hour;
                            temp.tm_min = dateTime.min;
                            temp.tm_sec = dateTime.sec;
                            // Since the timestamp will be calculated back to local time, we can ignore the daylight saving time.
                            temp.tm_isdst = 0;
                            // The mktime() creates an UTC timestamp, recalculate it back to local time.
                            // Since PicStream sends the timestamp to server without timezone data, the timestamp being created here should be the current local time not UTC time.
                            timestamp = (VPLTime_t) (mktime(&temp) - timezone);
                            rv = 0;
                        }
                    }
                }
            }
        } else {
            LOG_ERROR("PSGetPropertyKeyFromName fail 0x%x", (int)hr);
        }
        PropVariantClear(&propvarValue);
    } else {
        LOG_WARN("SHGetPropertyStoreFromParsingName fail 0x%x", (int)hr);
    }

    if (pps) 
        pps->Release();
end:
    if (wfilename)
        free(wfilename);

    if (comInitialized)
        CoUninitialize();
    return rv;
}
