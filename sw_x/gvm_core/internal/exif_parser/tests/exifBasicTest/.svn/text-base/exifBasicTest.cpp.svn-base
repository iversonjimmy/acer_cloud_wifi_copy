#include <log.h>
#include <exif_parser.h>
#include <vplex_time.h>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    int rv = 0;
    VPLTime_CalendarTime dateTime;
    VPLTime_t epochTime;
    string srcFile;

    if (argc < 2) {
        LOG_ERROR("%s <image file>", argv[0]);
        return -1;
    }

    VPL_Init();

    srcFile.assign(argv[1]);
    
    rv = EXIFGetImageTimestamp(srcFile, dateTime, epochTime);
    if (rv < 0) {
        LOG_ERROR("Parsing file %s (%d)", srcFile.c_str(), rv);
        goto end;
    }

    LOG_INFO("Timestamp:"FMT_VPLTime_CalendarTime_t" (file %s)", VAL_VPLTime_CalendarTime_t(dateTime), srcFile.c_str());

end:
    return rv;
}
