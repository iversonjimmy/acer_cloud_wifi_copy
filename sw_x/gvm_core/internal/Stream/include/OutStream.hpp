#ifndef __OUT_STREAM_HPP__
#define __OUT_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

class OutStream {
public:
    virtual ~OutStream() {}
    virtual ssize_t Write(const char *data, size_t datasize) = 0;
    virtual void StopIo() = 0;
    virtual bool IsIoStopped() const = 0;
};

#endif // incl guard
