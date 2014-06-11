
#include <sstream>
#include <limits>
#include <ctime>
#include <vpl_fs.h>

#include "file.hpp"

#ifdef WIN32
#include <windows.h>
#include <shlwapi.h>

    #ifdef max
    #undef max
    #endif
    #ifdef min
    #undef min
    #endif
#endif

namespace file
{
#ifdef WIN32
    directory::directory (std::string dir) :
        path (dir), error (0) {}

    bool directory::list (std::string filter)
    {
        std::string glob = path + filter;

        WIN32_FIND_DATA data;
        HANDLE handle = FindFirstFile (glob.c_str(), &data);
        bool proceed = handle != INVALID_HANDLE_VALUE;

        while (proceed)
        {
            if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                subdirs.push_back (data.cFileName);
            else
                files.push_back (data.cFileName);

            proceed = (FindNextFile (handle, &data) != 0);
        }

        if (handle != INVALID_HANDLE_VALUE)
            FindClose (handle);

        proceed = (error = GetLastError ()) == ERROR_NO_MORE_FILES;

        return proceed;
    }

    bool directory::exists ()
    {
        bool proceed = PathFileExists (path.c_str()) == TRUE;

        if (!proceed)
            error = GetLastError ();

        return proceed;
    }

    bool directory::create ()
    {
        bool proceed = CreateDirectory (path.c_str(), 0) == TRUE;

        if (!proceed)
            error = GetLastError ();

        return proceed;
    }

    directory::operator bool () const
    {
        return error == ERROR_SUCCESS || 
            error == ERROR_FILE_NOT_FOUND || 
            error == ERROR_NO_MORE_FILES;
    }
#endif

    note::note () :
        error (ERROR_NONE)
    {
    }

    note::note (std::string path)
    {
        std::fstream stream (path.c_str());
        bool proceed = stream.is_open();

        if (proceed)
        {
            char token;
            int state = EXPECT_BEGIN;
            std::string key, val;
            stream >> std::skipws;

            do
            {
                stream >> token;
                switch (token)
                {
                    case '{':
                        proceed = state == EXPECT_BEGIN;
                        if (proceed) state = EXPECT_KEY;
                        break;

                    case '}':
                        proceed = state == EXPECT_END;
                        if (proceed) state = EXPECT_COMPLETE;
                        break;

                    case ',':
                        proceed = state == EXPECT_END;
                        if (proceed) state = EXPECT_KEY;
                        break;

                    case ':':
                        proceed = state == EXPECT_VALUE;
                        break;

                    case '"':
                        proceed = state == EXPECT_KEY || state == EXPECT_VALUE;
                        if (proceed)
                        {
                            std::getline (stream, (state == EXPECT_KEY)? key : val, '"');

                            bool waiting_on_pair = key.empty() || val.empty();
                            if (!waiting_on_pair)
                                data [key] = val, key.clear(), val.clear();

                            state = (state == EXPECT_KEY)? EXPECT_VALUE : EXPECT_END;
                        }
                        break;

                    default:
                        proceed = false;
                }
            }
            while (proceed && stream);

            error = (state == EXPECT_COMPLETE)? PARSE_OK : PARSE_ERROR;
        }
    }

    void note::flush (std::string path)
    {
        std::ostringstream buffer;

        buffer << "{";

        std::map<std::string, std::string>::const_iterator field = data.begin();
        std::map<std::string, std::string>::const_iterator end = data.end();
        for (; field != end; ++field)
            buffer << " \"" << field->first << "\" : \"" << field->second << "\",";

        buffer.seekp (-1, std::ios_base::cur);
        buffer << " }";

        // TODO: safer two-phase commit (write to temp and rename+unlink original)
        std::ofstream stream (path.c_str(), std::ios_base::trunc);
        bool proceed = stream.is_open() &&
            stream << buffer.str();

        error = proceed? FLUSH_OK : FLUSH_ERROR;
    }

    bool note::update (std::vector<std::string> arguments)
    {
        bool proceed = true;

        std::vector<std::string>::const_iterator field = arguments.begin();
        std::vector<std::string>::const_iterator end = arguments.end();
        for (; proceed && field != end; ++field)
        {
            std::string key, map, val;
            std::stringstream stream (*field);
            std::streamsize N = std::numeric_limits<std::streamsize>::max();

            proceed =
                stream >> key &&
                stream >> map && map == ":" &&
                stream.ignore (N, ' ') &&
                std::getline (stream, val);

            if (proceed)
                data [key] = val;
        }

        return proceed;
    }

    note::operator bool () const
    {
        return error != PARSE_ERROR && error != FLUSH_ERROR;
    }

    media::media (std::string name, std::string mime) :
        filename (name),
        mime_type (mime),
        extension_type (name.substr (name.size()-3)),
        stream () {}

    media::operator bool () const
    {
        return stream.good();
    }

    std::string get_sync_path ()
    {
        std::string path;

#ifdef WIN32
        char *pointer = 0;

        if (_VPLFS__GetLocalAppDataPath (&pointer) == VPL_OK)
            path.assign (pointer);

        if (pointer)
            free (pointer);

        path += "\\Company\\NotesExample\\notes";
#endif

        return path;
    }

    uint64_t generate_rand64 ()
    {
        // std::rand portably guarantees only 15 bits of randomness (see RAND_MAX)
        uint64_t A = 0xFFFF & std::rand ();
        uint64_t B = 0xFFFF & std::rand ();
        uint64_t C = 0xFFFF & std::rand ();
        uint64_t D = 0xFFFF & std::rand ();
        return (A << (3 * 16)) | (B << (2 * 16)) | (C << (1 * 16)) | (D << (0 * 16));
    }

    uint64_t generate_guid64 (std::string const &bytes)
    {
        time_t now = std::time (0);
        std::srand ((uint32_t) now);

        uint64_t seed = generate_rand64 ();
        uint64_t guid = (now * seed) ^ 0xC96C5795D7870F42;

        std::string::const_iterator byte = bytes.begin ();
        std::string::const_iterator end = bytes.end ();
        for (; byte != end; ++byte)
        {
            guid =
                ((guid & 0xFFFF000000000000) >> (2 * 16)) |
                ((guid & 0x0000FFFF00000000) << (1 * 16)) |
                ((guid & 0x00000000FFFF0000) >> (1 * 16)) |
                ((guid & 0x000000000000FFFF) << (2 * 16));

            guid ^= (*byte * seed) ^ 0x82F63B78EB31D82E;
        }

        return guid;
    }

    uint64_t generate_guid64 (file::note const &note)
    {
        std::map<std::string,std::string>::const_iterator title = note.data.find ("title");

        return generate_guid64 ((title != note.data.end())? title->second : "");
    }

    uint64_t generate_guid64 (file::media const &media)
    {
        return generate_guid64 (media.filename);
    }

    std::string generate_guid (file::note const &note)
    {
        std::stringstream stream;
        stream << std::hex << generate_guid64 (note);
        return stream.str ();
    }

    std::string generate_guid (file::media const &media)
    {
        std::stringstream stream;
        stream << std::hex << generate_guid64 (media);
        return stream.str ();
    }

}
