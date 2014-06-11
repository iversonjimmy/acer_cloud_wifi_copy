#include <vpl_plat.h>
#include <vplex_time.h>
#include <log.h>
#include "exif_parser.h"
#include "exif_util.h"
#include <string>
#include <vpl_error.h>
#include <time.h>

#include <vplex__file_priv.h>

using namespace std;


int EXIFGetImageTimestamp(const string& filename,
                          VPLTime_CalendarTime& dateTime,
                          VPLTime_t& timestamp)
{
    int rv = VPL_ERR_FAIL;

    time_t time = 0;
    rv = _VPLFile__EXIFGetImageTimestamp(filename.c_str(), &time);
    timestamp = (VPLTime_t) time - timezone;
    VPLTime_ConvertToCalendarTimeLocal(VPLTime_FromSec(time), &dateTime);

    return rv;
}
