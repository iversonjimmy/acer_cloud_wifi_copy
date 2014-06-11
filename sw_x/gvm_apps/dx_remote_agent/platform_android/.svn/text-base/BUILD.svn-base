# IMPORTANT: if you add or remove a library from under 'libs', you must touch this file to force the update.

android_app(
    name = "dx_remote_agent_apk",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'src2': '/tests/dxshell/types/java',
        'res': 'project/res',
        'iGware.keystore': '/gvm_core/demos/iGware.keystore',
    },
    deps = [
        '/tests/dxshell/types:dx_remote_agent_proto',
        '/gvm_apps/android_cc_sdk:android_cc_sdk',
    ],
)
