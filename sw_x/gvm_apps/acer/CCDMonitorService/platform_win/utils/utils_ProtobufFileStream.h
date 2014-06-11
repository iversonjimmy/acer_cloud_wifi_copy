#ifndef __UTILS_PROTOBUF_FILESTREAM__
#define __UTILS_PROTOBUF_FILESTREAM__

#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <tchar.h>

#include <google/protobuf/io/zero_copy_stream_impl.h>

class FileStream
{
private:
    mutable google::protobuf::io::FileOutputStream _fOut;
    mutable google::protobuf::io::FileInputStream _fIn;

public:
    FileStream(int fd);
    virtual ~FileStream(void);
    
    int getInputSteamErr();
    int getOutputStreamErr();
    bool flushOutputStream();

    google::protobuf::io::ZeroCopyInputStream* getInputSteam();
    google::protobuf::io::ZeroCopyOutputStream* getOutputStream();

};

#endif
