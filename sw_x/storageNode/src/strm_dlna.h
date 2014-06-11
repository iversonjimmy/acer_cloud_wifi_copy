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

#ifndef STORAGE_NODE__STRM_DLNA_H__
#define STORAGE_NODE__STRM_DLNA_H__

#define HTTP_DLNA_MEDIAITEMS                     "/MediaItems/"
#define HTTP_DLNA_THUMBNAILS                     "/Thumbnails/"
#define HTTP_DLNA_ALBUMARTS                      "/AlbumArt/"
#define HTTP_DLNA_RESIZED                        "/Resized/"
#define HTTP_DLNA_ICONS                          "/icons/"
#define HTTP_DLNA_CAPTIONS                       "/Captions/"

#define CONNECTIONMGR_PATH                      "/ConnectionMgr.xml"
#define CONNECTIONMGR_CONTROLURL                "/ctl/ConnectionMgr"
#define CONNECTIONMGR_EVENTURL                  "/evt/ConnectionMgr"

#define FLAG_TIMEOUT            0x00000001
#define FLAG_SID                0x00000002
#define FLAG_RANGE              0x00000004
#define FLAG_HOST               0x00000008
#define FLAG_HTML               0x00000080
#define FLAG_INVALID_REQ        0x00000010
#define FLAG_CHUNKED            0x00000100
#define FLAG_TIMESEEK           0x00000200
#define FLAG_REALTIMEINFO       0x00000400
#define FLAG_PLAYSPEED          0x00000800
#define FLAG_XFERSTREAMING      0x00001000
#define FLAG_XFERINTERACTIVE    0x00002000
#define FLAG_XFERBACKGROUND     0x00004000
#define FLAG_CAPTION            0x00008000
#define FLAG_DLNA               0x00100000
#define FLAG_MIME_AVI_DIVX      0x00200000
#define FLAG_MIME_AVI_AVI       0x00400000
#define FLAG_MIME_FLAC_FLAC     0x00800000
#define FLAG_NO_RESIZE          0x01000000

#endif // include guard
