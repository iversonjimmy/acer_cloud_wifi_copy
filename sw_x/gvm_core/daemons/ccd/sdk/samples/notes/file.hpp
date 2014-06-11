#ifndef FILE_HPP__
#define FILE_HPP__

#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <vpl_types.h>

namespace file
{
    struct directory
    {
        int error;
        std::string path;
        std::vector<std::string> subdirs;
        std::vector<std::string> files;

        directory (std::string path);
        bool list (std::string filter);
        bool exists ();
        bool create ();

        operator bool () const;
    };

    struct note
    {
        enum
        {
            PARSE_ERROR = -2,
            FLUSH_ERROR = -1,
            ERROR_NONE = 0,
            FLUSH_OK = 1,
            PARSE_OK = 2,
        };

        enum
        {
            EXPECT_BEGIN,
            EXPECT_KEY,
            EXPECT_VALUE,
            EXPECT_END,
            EXPECT_COMPLETE
        };

        int error;
        std::map<std::string,std::string> data;

        note ();
        note (std::string path);

        void flush (std::string path);
        bool update (std::vector<std::string> fields);

        operator bool () const;
    };

    struct media
    {
        std::string filename;
        std::string mime_type;
        std::string extension_type;
        std::ifstream stream;

        media (std::string name, std::string mime);

        operator bool () const;
    };

    std::string get_sync_path ();

    uint64_t generate_rand64 ();
    uint64_t generate_guid64 (file::note const &note);
    uint64_t generate_guid64 (file::media const &media);
    std::string generate_guid (file::note const &note);
    std::string generate_guid (file::media const &media);
}

#endif
