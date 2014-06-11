#ifndef CONV_HPP__
#define CONV_HPP__

template <typename T>
T hex_str_to (std::string const &str)
{
    T value;
    std::istringstream stream (str);
    stream >> std::hex >> value;
    return value;
}

#endif
