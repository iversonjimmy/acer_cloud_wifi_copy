
#include "file.hpp"
#include "sync.hpp"
#include "sync-tasks.hpp"

namespace sync
{
    namespace task
    {
        worker::worker (sync::info &info) : 
            sync_info (info) {}

        update_app_state::update_app_state (sync::info &info) :
            sync::task::worker (info) {}

        bool update_app_state::operator() ()
        {
            ccd::UpdateAppStateOutput response;
            ccd::UpdateAppStateInput request;

            request.set_app_id ("com.acer.cloud.ccd_sdk.notes_example");
            request.set_app_type (ccd::CCD_APP_DEFAULT);
            request.set_foreground_mode (true);

            CCDIError result = CCDIUpdateAppState (request, response);
            bool proceed = result == CCD_OK;

            if (!proceed)
                error << "could not get app state (" << result << ")";

            return proceed;
        }

        read_sys_state::read_sys_state (sync::info &info) :
            sync::task::worker (info) {}

        bool read_sys_state::operator() ()
        {
            ccd::GetSystemStateOutput response;
            ccd::GetSystemStateInput request;

            request.set_get_players (true);
            request.set_get_device_id (true);

            CCDIError result = CCDIGetSystemState (request, response);
            bool proceed = result == CCD_OK;

            if (!proceed)
                error << "could not get system state (" << result << ")";
            else
            {
                sync_info.user_id = response.players().players(0).user_id();
                sync_info.device_id = response.device_id();
            }

            return proceed;
        }

        update_sync_settings::update_sync_settings (sync::info &info) :
            sync::task::worker (info) {}

        bool update_sync_settings::operator() ()
        {
            ccd::UpdateSyncSettingsOutput response;
            ccd::UpdateSyncSettingsInput request;

            sync_info.path = file::get_sync_path ();

            request.set_user_id (sync_info.user_id);
            request.mutable_configure_notes_sync()->set_enable_sync_feature (true);
            request.mutable_configure_notes_sync()->set_set_sync_feature_path (sync_info.path);

            CCDIError result = CCDIUpdateSyncSettings (request, response);
            bool proceed = result == CCD_OK;

            if (!proceed)
                error << "failed to update sync settings";

            return proceed;
        }

        read_sync_state::read_sync_state (sync::info &info) :
            sync::task::worker (info) {}

        bool read_sync_state::operator() ()
        {
            ccd::GetSyncStateOutput response;
            ccd::GetSyncStateInput request;

            request.set_only_use_cache (true);

            CCDIError result = CCDIGetSyncState (request, response);
            bool proceed = result == CCD_OK;

            if (!proceed)
                error << "failed to get sync path";

            return proceed;
        }
    }
}
