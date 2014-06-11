#include "utils_ProtobufFileStream.h"

FileStream::FileStream(int fd) 
    : _fIn(fd), _fOut(fd)
{
    ;
}

FileStream::~FileStream(void)
{
    ;
}

google::protobuf::io::ZeroCopyInputStream* FileStream::getInputSteam()
{
    return &_fIn;
}
google::protobuf::io::ZeroCopyOutputStream* FileStream::getOutputStream()
{
    return &_fOut;
}

int FileStream::getInputSteamErr()
{
    return _fIn.GetErrno();
}

int FileStream::getOutputStreamErr()
{
    return _fOut.GetErrno();
}

bool FileStream::flushOutputStream()
{
    return _fOut.Flush();
}
