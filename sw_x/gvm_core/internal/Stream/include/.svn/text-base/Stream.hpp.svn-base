#ifndef __STREAM_HPP__
#define __STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include "InStream.hpp"
#include "OutStream.hpp"

class Stream : public InStream, public OutStream {
public:
    virtual ~Stream() {}

    // inherited interface
    virtual ssize_t Read(char *buf, size_t bufsize) = 0;
    virtual ssize_t Write(const char *data, size_t datasize) = 0;
    virtual void StopIo() = 0;
    virtual bool IsIoStopped() const = 0;
};

#endif // incl guard
