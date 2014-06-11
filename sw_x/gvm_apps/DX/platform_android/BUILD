# IMPORTANT: if you add or remove a library from under 'libs', you must touch this file to force the update.

android_app(
    name = "DX",
    target = "android-12",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'res': 'project/res',
        'assets': 'project/assets',
        'iGware.keystore': '$(SRCROOT)/gvm_core/demos/iGware.keystore',
    },
    deps = [
        '/gvm_apps/android_cc_sdk:android_cc_sdk',
        '/gvm_core/external/acra:acra-java',
    ],
)
