
#include <cstdio>

#include "file.hpp"
#include "file-tasks.hpp"

namespace file
{
    namespace task
    {
        create_note::create_note (std::string const &path, std::vector<std::string> fields) : 
            file::task::worker (path), note_fields (fields) {}

        bool create_note::operator() ()
        {
            bool proceed = !sync_path.empty() && note_fields.size();

            if (!proceed)
                error << "bad or missing arguments";

            if (proceed)
            {
                file::note outgoing;
                proceed = outgoing && outgoing.update (note_fields);

                std::string note_path = sync_path + "/" + generate_guid (outgoing) + ".note";

                if (!proceed)
                    error << "failed to update " << note_path;

                if (proceed)
                {
                    outgoing.flush (note_path);
                    proceed = outgoing == true;

                    if (!proceed)
                        error << "failed to write " << note_path;
                }
            }

            return proceed;
        }

        update_note::update_note (std::string const &path, std::string guid, std::vector<std::string> fields) :
            file::task::worker (path), note_guid (guid), note_fields (fields) {}

        bool update_note::operator() ()
        {
            bool proceed = !sync_path.empty() && !note_guid.empty() && note_fields.size();
            
            if (!proceed)
                error << "bad or missing arguments";

            if (proceed)
            {
                std::string note_path = sync_path + "/" + note_guid + ".note";

                file::note parsed (note_path);
                proceed = parsed == true;
                
                if (!proceed)
                    error << "failed to parse " << note_path;

                if (proceed)
                {
                    parsed.update (note_fields);

                    if (!proceed)
                        error << "failed to update " << note_path;
                }
                
                if (proceed)
                {
                    parsed.flush (note_path);
                    proceed = parsed == true;

                    if (!proceed)
                        error << "failed to write " << note_path;
                }
            }

            return proceed;
        }

        delete_note::delete_note (std::string const &path, std::string guid) : 
            file::task::worker (path), note_guid (guid) {}

        bool delete_note::operator() ()
        {
            bool proceed = !sync_path.empty() && !note_guid.empty();

            if (!proceed)
                error << "bad or missing arguments";

            if (proceed)
            {
                std::string note_path = sync_path + "/" + note_guid + ".note";
                proceed = std::remove (note_path.c_str ()) == 0;

                if (!proceed)
                    error << "could not remove " << note_path;
            }

            return proceed;
        }

        attach_media::attach_media (std::string const &path, std::string guid, std::string media, std::string type) : 
            file::task::worker (path), note_guid (guid), media_path (media), media_type (type) {}

        bool attach_media::operator() ()
        {
            bool proceed = !sync_path.empty() && !note_guid.empty() && !media_path.empty();

            if (!proceed)
                error << "bad or missing arguments";

            if (proceed)
            {
                file::directory basedir (sync_path + "/media");
                proceed = basedir.exists()? true : basedir.create();

                if (!proceed)
                    error << "unable to open " << basedir.path;

                if (proceed)
                {
                    std::string note_path = sync_path + "/" + note_guid + ".note";

                    if (media_type.empty()) 
                        media_type = "application/octet-stream";

                    file::media media (media_path, media_type);
                    proceed = media == true;

                    if (!proceed)
                        error << "unable to open " << media.filename;

                    if (proceed)
                    {
                        std::string name = generate_guid (media) + "." + media.extension_type;
                        std::string path = basedir.path + "/" + name;

                        proceed = std::rename (media.filename.c_str (), path.c_str ()) == 0;

                        if (!proceed)
                            error << "unable to rename (" << errno << ") " << media.filename << " -> " << path;
                    
                        if (proceed)
                        {
                            file::note attached (note_path);
                            proceed = attached == true;

                            if (!proceed)
                                error << "unable to parse note " << note_path;

                            if (proceed)
                            {
                                std::string link = "<media type='" + media.mime_type + "'>" + name + "</media>";

                                attached.data["body"] += link;
                                attached.flush (note_path);
                                proceed = attached == true;

                                if (!proceed)
                                    error << "unable to flush note " << note_path;
                            }
                        }
                    }
                }
            }

            return proceed;
        }

        remove_media::remove_media (std::string const &path, std::string guid) : 
            file::task::worker (path), media_guid (guid) {}

        bool remove_media::operator() ()
        {
            bool proceed = !sync_path.empty() && !media_guid.empty();

            if (!proceed)
                error << "bad or missing arguments";

            if (proceed)
            {
                file::directory media (sync_path + "/media");
                proceed = media.exists()? true : media.create();

                if (!proceed)
                    error << "unable to open " << media.path;

                if (proceed)
                {
                    std::string path = media.path + "/" + media_guid;
                    proceed = std::remove (path.c_str ()) == 0;

                    if (!proceed)
                        error << "unable to remove " << path;
                }
            }

            return proceed;
        }

        parse_sync_path::parse_sync_path (std::string const &path, std::vector< std::map<std::string, std::string> > &data) : 
            file::task::worker (path), sync_data (data) {}
            
        bool parse_sync_path::operator() ()
        {
            bool proceed = !sync_path.empty ();

            if (!proceed)
                error << "bad or missing arguments";
            
            if (proceed)
            {
                file::directory notes (sync_path);

                proceed = notes.exists()? true : notes.create();
                proceed = proceed && notes.list ("/*.note");

                if (!notes)
                    error << "unable to open sync directory (" << notes.error << ")";

                if (notes.files.empty())
                    error << "no sync files to index";

                std::vector<std::string>::iterator file = notes.files.begin();
                for (; proceed && file != notes.files.end(); ++file)
                {
                    std::string guid (file->substr (0, file->size()-5));
                    file::note parsed (sync_path + "/" + *file);
                    proceed = parsed == true;

                    if (!proceed)
                        error << "unable to parse note (" << *file << ")";

                    if (proceed)
                    {
                        proceed =
                            parsed.data.find ("title") != parsed.data.end () &&
                            parsed.data.find ("location") != parsed.data.end () &&
                            parsed.data.find ("body") != parsed.data.end ();

                        parsed.data["guid"] = guid;

                        if (!proceed)
                            error << "parsed note is not complete (" << *file << ")";
                    }
                    
                    if (proceed)
                        sync_data.push_back (parsed.data);
                }
            }

            return proceed;
        }
    }
}
