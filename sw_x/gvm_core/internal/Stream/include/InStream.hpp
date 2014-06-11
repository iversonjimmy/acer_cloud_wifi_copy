#ifndef __IN_STREAM_HPP__
#define __IN_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

class InStream {
public:
    virtual ~InStream() {}
    virtual ssize_t Read(char *buf, size_t bufsize) = 0;
    virtual void StopIo() = 0;
    virtual bool IsIoStopped() const = 0;
};

#endif // incl guard
