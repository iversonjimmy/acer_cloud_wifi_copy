#include "InStringStream.hpp"

InStringStream::InStringStream()
    : ioStopped(false)
{
}

InStringStream::InStringStream(const std::string &initdata)
    : ioStopped(false)
{
    iss.str(initdata);
}

InStringStream::~InStringStream()
{
}

ssize_t InStringStream::Read(char *buf, size_t bufsize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    iss.read(buf, bufsize);
    if ((iss.fail() || iss.bad()) && !iss.eof()) {
        return -1;  // FIXME
    }
    return iss.gcount();
}

void InStringStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;
}

bool InStringStream::IsIoStopped() const
{
    return ioStopped;
}

void InStringStream::SetInput(const std::string &input)
{
    iss.str(input);
}
