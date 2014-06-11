#include <libexif/exif-data.h>  
#include <vpl_plat.h>
#include <vpl_error.h>
#include <log.h>
#include "exif_parser.h"
#include "exif_util.h"

using namespace std;


int EXIFGetImageTimestamp(const string& filename,
                          VPLTime_CalendarTime& dateTime,
                          VPLTime_t& timestamp)
{
    int rv = VPL_ERR_FAIL;
    ExifData *ed;
    ExifEntry *entry;
    char buf[1024];

    ed = exif_data_new_from_file(filename.c_str());
    if (ed) {
        entry = exif_data_get_entry(ed, EXIF_TAG_DATE_TIME_ORIGINAL);
        if (entry) {
            LOG_INFO("File %s has ORIGINAL EXIF timestamp.", filename.c_str());
        } else {
            entry = exif_data_get_entry(ed, EXIF_TAG_DATE_TIME_DIGITIZED);
            if (entry) {
                LOG_INFO("File %s has DIGITIZED EXIF timestamp.", filename.c_str());
            } else {
                entry = exif_data_get_entry(ed, EXIF_TAG_DATE_TIME);
                if (!entry) {
                    LOG_INFO("File %s has no EXIF timestamp.", filename.c_str());
                    goto fail_extract;
                }
            }
        }

        exif_entry_get_value(entry, buf, sizeof(buf));
        // Entry should be of the form:
        // DateTime: YYYY:MM:DD HH:MM:SS
        // Convert to time.
        LOG_INFO("DateTime Tag {%s} from file %s.", buf, filename.c_str());
        rv = EXIFTagToCalendarTime(buf, dateTime);

        if (rv == 0)
        {
            struct tm temp;
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
        }
fail_extract:
        exif_data_unref(ed);
    } else {
        LOG_WARN("No EXIF info");
    }

    return rv;
}
