# IMPORTANT: if you add or remove a library from under 'libs', you must touch this file to force the update.

android_app(
    name = "actool_app",
    target = "android-8",
    links = {
        'AndroidManifest.xml': 'project/AndroidManifest.xml',
        'build.properties': 'build.properties',
        'src': 'project/src',
        'res': 'project/res',
        'iGware.keystore': '$(SRCROOT)/gvm_core/demos/iGware.keystore',
        'assets/ccd.conf.tmpl': '$(SRCROOT)/gvm_core/conf/ccd.conf.tmpl',
    },
    deps = []
)
