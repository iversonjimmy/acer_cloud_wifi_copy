=============================================================================
* iGware Cloud Client SDK for Android *
=============================================================================

===========================================================
Quick start:
===========================================================

To use the example projects in Eclipse:
1. Make sure that the Android plugin (ADT) is installed.
  * See http://developer.android.com/sdk/eclipse-adt.html
2. Add a Java Classpath Variable "CLOUD_CLIENT_ANDROID_SDK" to your workspace.
  * Window -> Preferences -> Java -> Build Path -> Classpath Variables
  * Click New...
  * Name: CLOUD_CLIENT_ANDROID_SDK
  * Path: <Pick the location of this SDK>
3. Import the projects into your workspace.
  * File -> Import -> Existing Projects Into Workspace
  * Pick the location of this SDK
  * Select the projects
  * Finish
4. For each example project:
  * Right-click on the project, choose Android Tools -> Fix Project Properties
5. Project -> Clean...
  * Select all of the example projects.
6. Project -> Build All...

===========================================================
Example project overviews:
===========================================================

1. "example_service_only" is an application that wraps the native cloud client libraries (.so files)
as Android Services.  The application does not include any Activities (UI).

2. "example_remote_panel" is an application consisting of Activities (UI) that make requests to
CcdiService.  This application uses remote binding and AIDL to send requests to the services in the
other application (example_service_only).  The application does not include any Services.

3. "example_panel_and_service" combines the above Services and Activities into a single application.
It uses local binding to send the requests since the Activities are in the same process as the
Service. 

Notes:
* "example_service_only" and "example_panel_and_service" are mutually exclusive; you should only
install one or the other at any given time.  This is because they both declare the same
"intent-filter" entries, and it may be confusing which one is actually servicing the requests.

* "example_remote_panel" will work with either "example_service_only" or "example_panel_and_service".

===========================================================
Other notes:
===========================================================

* Because they interface with JNI, the provided libccd-jni.so and libmca-jni.so libraries require
that the names of the corresponding Java classes and native method declarations are not changed.
Specifically, be careful if you need to make changes to any of:
- com.igware.android_services.CcdiService
- com.igware.android_services.McaService
- com.igware.android_services.ServiceSingleton

=============================================================================
