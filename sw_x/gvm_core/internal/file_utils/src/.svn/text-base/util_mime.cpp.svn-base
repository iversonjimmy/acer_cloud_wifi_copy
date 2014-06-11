/*
 *  Copyright 2010 iGware Inc.
 *  All Rights Reserved.
 *
 *  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND 
 *  TRADE SECRETS OF IGWARE INC.
 *  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT 
 *  THE PRIOR EXPRESS WRITTEN PERMISSION OF IGWARE INC.
 *
 */

#include "util_mime.hpp"

void Util_CreateVideoMimeMap(std::map<std::string, std::string, case_insensitive_less> &m)
{
    m.clear();

    m["mp2"] = "video/mpeg";
    m["mpa"] = "video/mpeg";
    m["mpeg"] = "video/mpeg";
    m["mp3"] = "video/mpeg";
    m["mpg"] = "video/mpeg";
    m["mpe"] = "video/mpeg";
    m["mpv2"] = "video/mpeg";
    m["mp4"] = "video/mp4";
    m["mov"] = "video/quicktime";
    m["qt"] = "video/quicktime";
    m["lsf"] = "video/x-la-asf";
    m["lsx"] = "video/x-la-asf";
    m["asf"] = "video/x-ms-asf";
    m["asr"] = "video/x-ms-asf";
    m["asx"] = "video/x-ms-asf";
    m["wmv"] = "video/x-ms-wmv";
    m["avi"] = "video/x-msvideo";
    m["movie"] = "video/x-sgi-movie";
    m["m4v"] = "video/x-m4v";
    m["m4u"] = "video/vnd.mpegurl";
    m["mxu"] = "video/vnd.mpegurl";
    m["dv"] = "video/x-dv";
    m["dif"] = "video/x-dv";
}

void Util_CreateAudioMimeMap(std::map<std::string, std::string, case_insensitive_less> &m)
{
    m.clear();

    m["aac"] = "audio/x-aac";
    m["au"] = "audio/basic";
    m["wma"] = "audio/x-ms-wma";
    m["snd"] = "audio/basic";
    m["mid"] = "audio/mid";
    m["rmi"] = "audio/mid";
    m["kar"] = "audio/midi";
    m["midi"] = "audio/midi";
    m["mp3"] = "audio/mpeg";
    m["mp2"] = "audio/mpeg";
    m["mpga"] = "audio/mpeg";
    m["aif"] = "audio/x-aiff";
    m["aifc"] = "audio/x-aiff";
    m["aiff"] = "audio/x-aiff";
    m["m3u"] = "audio/x-mpegurl";
    m["ra"] = "audio/x-pn-realaudio";
    m["ram"] = "audio/x-pn-realaudio";
    m["wav"] = "audio/x-wav";
    m["m4a"] = "audio/mp4a-latm";
    m["m4b"] = "audio/mp4a-latm";
    m["m4p"] = "audio/mp4a-latm";
}

void Util_CreatePhotoMimeMap(std::map<std::string, std::string, case_insensitive_less> &m)
{
    m.clear();

    m["bmp"] = "image/bmp";
    m["cgm"] = "image/cgm";
    m["gif"] = "image/gif";
    m["pic"] = "image/pict";
    m["pct"] = "image/pict";
    m["pict"] = "image/pict";
    m["jpg"] = "image/jpeg";
    m["jpe"] = "image/jpeg";
    m["jpeg"] = "image/jpeg";
    m["ief"] = "image/ief";
    m["jp2"] = "image/jp2";
    m["jfif"] = "image/pipeg";
    m["png"] = "image/png";
    m["tif"] = "image/tiff";
    m["tiff"] = "image/tiff";
    m["svg"] = "image/svg+xml";
    m["ico"] = "image/x-icon";
    m["ppm"] = "image/x-portable-pixmap";
    m["pnm"] = "image/x-portable-anymap";
    m["pbm"] = "image/x-portable-bitmap";
    m["pgm"] = "image/x-portable-graymap";
    m["djv"] = "image/vnd.djvu";
    m["djvu"] = "image/vnd.djvu";
    m["mac"] = "image/x-macpaint";
    m["pnt"] = "image/x-macpaint";
    m["pntg"] = "image/x-macpaint";
    m["qti"] = "image/x-quicktime";
    m["qtif"] = "image/x-quicktime";
    m["ras"] = "image/x-cmu-raster";
    m["rgb"] = "image/x-rgb";
    m["wbmp"] = "image/vnd.wap.wbmp";
    m["xbm"] = "image/x-xbitmap";
    m["xpm"] = "image/x-xpixmap";
    m["xwd"] = "image/x-xwindowdump";
}

class Util_MediaTypeMap {
public:
    Util_MediaTypeMap(void (*initializer)(std::map<std::string, std::string, case_insensitive_less> &)) {
        initializer(m);
    }
    std::map<std::string, std::string, case_insensitive_less>::const_iterator find(const std::string &key) const {
        return m.find(key);
    }
    std::map<std::string, std::string, case_insensitive_less>::const_iterator end() const {
        return m.end();
    }
protected:
    std::map<std::string, std::string, case_insensitive_less> m;
};

static const Util_MediaTypeMap util_videoMediaTypeMap(Util_CreateVideoMimeMap);
static const Util_MediaTypeMap util_audioMediaTypeMap(Util_CreateAudioMimeMap);
static const Util_MediaTypeMap util_photoMediaTypeMap(Util_CreatePhotoMimeMap);

static const std::string &util_findMediaTypeFromExtension(const Util_MediaTypeMap &m, const std::string &extension)
{
    static const std::string empty;

    std::map<std::string, std::string, case_insensitive_less>::const_iterator it = m.find(extension);
    if (it != m.end()) {
        return it->second;
    }
    else {
        return empty;
    }
}

const std::string &Util_FindVideoMediaTypeFromExtension(const std::string &extension)
{
    return util_findMediaTypeFromExtension(util_videoMediaTypeMap, extension);
}

const std::string &Util_FindAudioMediaTypeFromExtension(const std::string &extension)
{
    return util_findMediaTypeFromExtension(util_audioMediaTypeMap, extension);
}

const std::string &Util_FindPhotoMediaTypeFromExtension(const std::string &extension)
{
    return util_findMediaTypeFromExtension(util_photoMediaTypeMap, extension);
}

