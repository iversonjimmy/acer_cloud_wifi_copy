= Build on Windows =

== Via GUI ==
* Set source code directory to the location of CMakeLists.txt
* Set binary build directory to desired build directory
* Open the generated solution in explorer to launch Visual Studio
* Ensure the generated binaries use "Visual Studio 9 2008" to match the CCD client compiler version
* Ensure the start-up project is set and build
* To install build the INSTALL project

== Via Command line ==
* change working directory to your chosen build directory
* let PROJECTROOT be the location of the CMakeLists.txt file
* run $ cmake.exe -G "Visual Studio 9 2008" -DCMAKE_INSTALL_PREFIX=$BUILDROOT $PROJECTROOT
* run $ cmake.exe --build . --config Release
** or $ devenv.com notes.sln -build Release
* run $ cmake.exe --build . --target install --config Release
** or $ devenv.com notes.sln -build Release -project INSTALL

=== Caveats ===
* Cygwin is a unix emulation layer, and as such CMake detects the configuration as UNIX, not WIN32
** To build with VS, run cmake under mingw or command.com
* Some case sensitive shells define duplicate tmp/temp environment variables; these should be unset

= Starting CCD on Windows =

* run $ cd ${BUILDROOT}/bin
* run $ dxshell.exe StopCCD
* run $ dxshell.exe SetDomain pc-int.igware.net
* run $ dxshell.exe StartCCD

= Starting the Notes Application on Windows =

* run notes.exe in the directory you wish to host notes.db

= Command Argument Formatting =

== User Commands ==
* > quit
* > dump-notes
* > find-note "$field" "$value"
* > create-note "title : $title" "location : $location" "body : $body"
** note: space around colon is required
* > update-note $noteguid "$field : $value"
* > delete-note $noteguid
* > attach-media $guid "C:\Current\File.Name"
* > remove-media $mediaguid

= Walk-through =

== Overview ==

CCD is a powerful client-side program that provides all services necessary to synchronize files with other authorized clients. Primarily the main interface for interacting with CCD will be the filesystem: modify files in the specified sync directory, and those modifications will be synchronized on all authorized and available devices automatically.

However there is basic set-up that must happen, and direct calls to CCD are made through message passing (implemented through pprotobufs), which communicate with CCD as a running process, or a linked library. All calls are blocking, and replies are immediately available when the call returns.

Also, CCD will asynchronously notify you of important events, such as when CCD begins syncing a new remote file, and when syncing is complete and the file is visible in the sync directory.

== Setting up CCD ==

The application should inform CCD when it is active by setting the app state to "foreground":
<code>
    ccd::UpdateAppStateOutput response;
    ccd::UpdateAppStateInput request;

    request.set_app_id ("com.acer.cloud.ccd_sdk.notes_example");
    request.set_foreground_mode (true);

    CCDIError result = CCDIUpdateAppState (request, response);
</code>

Next the application should query CCD to find out if anyone is logged in:
<code>
    ccd::GetSystemStateOutput response;
    ccd::GetSystemStateInput request;

    request.set_get_players (true);

    CCDIError result = CCDIGetSystemState (request, response);
    
    if (result == CCD_OK)
        user_id = response.players().players(0).user_id();
</code>

CCD should already have a logged-in user, but if this call fails, be prepared to have the user re-authenticate.

Next we must ensure notes syncing feature of CCD is enabled:
<code>
    ccd::UpdateSyncSettingsOutput response;
    ccd::UpdateSyncSettingsInput request;

    request.set_user_id (user_id);
    request.set_enable_notes_sync (true);

    CCDIError result = CCDIUpdateSyncSettings (request, response);
</code>

This should only need to be done once.

Lastly, we must discover the sync directory for notes:
<code>
    ccd::GetSyncStateOutput response;
    ccd::GetSyncStateInput request;

    request.set_only_use_cache (true);
    request.set_get_notes_sync_path (true);

    CCDIError result = CCDIGetSyncState (request, response);

    if (result == CCD_OK)
        sync_path = response.notes_sync_path();
</code>

Any files we write to this directory will be synced by CCD automatically.

== Listening to CCD Notifications ==

CCD can save your application having to poll for status updates by creating an event queue:
<code>
    ccd::EventsCreateQueueOutput response;
    ccd::EventsCreateQueueInput request;

    CCDIError result = CCDIEventsCreateQueue (request, response);
    
    if (error == CCD_OK)
        queue_handle = response.queue_handle ();
</code>

Now you can ask the queue for events:
<code>
    ccd::EventsDequeueOutput response;
    ccd::EventsDequeueInput request;

    request.set_queue_handle (queue_handle);
    request.set_timeout (timeout);

    CCDIError result = CCDIEventsDequeue (request, response);
</code>
 
The dequeue routine will block until timeout; you can configure it to block indefinitely by setting the timeout value to -1. When blocking, the application won't progress, so in this case you'll need to run it on a separate thread.

Of course, don't forget to destroy the queue on exit:
<code>
    ccd::EventsDestroyQueueInput request;

    request.set_queue_handle (queue_handle);

    CCDIError result = CCDIEventsDestroyQueue (request);
</code>
