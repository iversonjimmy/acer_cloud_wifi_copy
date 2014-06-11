#ifndef __OUT_STRING_STREAM_HPP__
#define __OUT_STRING_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <sstream>
#include <string>

#include "OutStream.hpp"

class OutStringStream : public OutStream {
public:
    OutStringStream();
    ~OutStringStream();

    // inherited interface
    ssize_t Write(const char *data, size_t datasize);
    void StopIo();
    bool IsIoStopped() const;

    // new interface
    std::string GetOutput() const;

private:
    VPL_DISABLE_COPY_AND_ASSIGN(OutStringStream);

    bool ioStopped;
    std::ostringstream oss;
};

#endif // incl guard
