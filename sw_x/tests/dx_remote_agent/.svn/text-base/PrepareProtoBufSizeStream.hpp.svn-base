#ifndef PREPARE_PROTO_BUF_SIZE_STREAM_H_
#define PREPARE_PROTO_BUF_SIZE_STREAM_H_
#include <vector>
#include <algorithm>

// Cannot use std::stringstream to replace this class
// Because if the protobuf message is header only, the payload will be only 1 byte and is 0
// In this scenario, the 0 will be skipped by std::stringstream and cause the compute protobuf payload method failed
class PrepareProtoBufSizeStream
{
public:
    PrepareProtoBufSizeStream(unsigned char *data, int size) : pos(0)
    {
        for (int i = 0; i < size; ++i)
            vecStream.push_back(*(data + i));
    }

    ~PrepareProtoBufSizeStream() { }

    std::vector<unsigned char>::iterator get() { return std::find(vecStream.begin(), vecStream.end(), vecStream.at(pos++)); }
    int tellg() { return pos; }
    PrepareProtoBufSizeStream& seekg(int newpos) { pos = newpos; return *this; }
    PrepareProtoBufSizeStream& ignore(int offset) { pos += offset; return *this; }
private:
    std::vector<unsigned char> vecStream;
    int pos;
};
#endif
