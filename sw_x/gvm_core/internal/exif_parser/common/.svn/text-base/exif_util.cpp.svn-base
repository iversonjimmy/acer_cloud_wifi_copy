#include <string>
#include <vpl_types.h>
#include <vpl_error.h>
#include <vplex_time.h>
#include <log.h>
#include <exif_util.h>

using namespace std;

int EXIFTagToCalendarTime(const char* buf,
                          VPLTime_CalendarTime& date_time)
{
    int rv = VPL_ERR_FAIL;
    string date_time_str;
    size_t cursor;

    if (buf == NULL) {
        LOG_ERROR("Null tag buffer");
        goto end;
    }

    date_time_str = buf;
    cursor = date_time_str.find_first_of("0123456789");
    if (cursor == string::npos) {
        goto end;
    }

    memset(&date_time, 0, sizeof(date_time));

    date_time.year = strtol(date_time_str.data() + cursor, 0, 10);
    cursor = date_time_str.find_first_of(":", cursor);
    if (cursor == string::npos) {
        goto end;
    }

    cursor++;
    date_time.month = strtol(date_time_str.data() + cursor, 0, 10);
    cursor = date_time_str.find_first_of(":", cursor);
    if (cursor == string::npos) {
        goto end;
    }

    cursor++;
    date_time.day = strtol(date_time_str.data() + cursor, 0, 10);
    cursor = date_time_str.find_first_of(" ", cursor);
    if(cursor == string::npos) {
        goto end;
    }

    cursor++;
    date_time.hour = strtol(date_time_str.data() + cursor, 0, 10);
    cursor = date_time_str.find_first_of(":", cursor);
    if(cursor == string::npos) {
        goto end;
    }

    cursor++;
    date_time.min = strtol(date_time_str.data() + cursor, 0, 10); 
    cursor = date_time_str.find_first_of(":", cursor);
    if(cursor == string::npos) {
        goto end;
    }

    cursor++;
    date_time.sec = strtol(date_time_str.data() + cursor, 0, 10);
    LOG_INFO("Extracted datetime: "FMT_VPLTime_CalendarTime_t, VAL_VPLTime_CalendarTime_t(date_time));
    rv = 0;

end:
    return rv;
}
