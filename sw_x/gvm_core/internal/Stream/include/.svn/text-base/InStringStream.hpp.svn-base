#ifndef __IN_STRING_STREAM_HPP__
#define __IN_STRING_STREAM_HPP__

#include <vplu_types.h>
#include <vplu_common.h>
#include <vplu_missing.h>

#include <sstream>
#include <string>

#include "InStream.hpp"

class InStringStream : public InStream {
public:
    InStringStream();
    InStringStream(const std::string &initdata);
    ~InStringStream();

    // inherited interface
    ssize_t Read(char *buf, size_t bufsize);
    void StopIo();
    bool IsIoStopped() const;

    // new interface
    void SetInput(const std::string &input);

private:
    VPL_DISABLE_COPY_AND_ASSIGN(InStringStream);

    bool ioStopped;
    std::istringstream iss;
};

#endif // incl guard
