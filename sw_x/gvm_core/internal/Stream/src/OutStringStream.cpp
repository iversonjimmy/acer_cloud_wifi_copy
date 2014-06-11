#include "OutStringStream.hpp"

OutStringStream::OutStringStream()
    : ioStopped(false)
{
}

OutStringStream::~OutStringStream()
{
}

ssize_t OutStringStream::Write(const char *data, size_t datasize)
{
    if (ioStopped) {
        return VPL_ERR_CANCELED;
    }

    oss.write(data, datasize);
    if (oss.fail() || oss.bad()) {
        return -1;  // FIXME
    }
    return datasize;
}

void OutStringStream::StopIo()
{
    if (!ioStopped)
        ioStopped = true;
}

bool OutStringStream::IsIoStopped() const
{
    return ioStopped;
}

std::string OutStringStream::GetOutput() const
{
    return oss.str();
}
