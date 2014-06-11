# IMPORTANT: if you add or remove a library from under 'libs', you must touch this file to force the update.

android_app(
    name = "example_remote_panel_apk",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'res': 'project/res',
        'iGware.keystore': '/gvm_core/demos/iGware.keystore',
    },
    deps = [
        '..:android_cc_sdk',
    ],
)
