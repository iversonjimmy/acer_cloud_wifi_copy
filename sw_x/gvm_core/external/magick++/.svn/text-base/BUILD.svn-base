external_static_cpp_lib(
    name = 'libgraphicsmagick',
    includes = [
            'usr/include/GraphicsMagick',
            ],
    outfile = 'usr/lib/libGraphicsMagick.a',
    deps = ['/:libpng', '/:libjpeg', ] + (['/:libtiff',] if platform == 'cloudnode' else [])
)

external_static_cpp_lib(
    name = 'libgraphicsmagick++',
    includes = [
            'usr/include/GraphicsMagick',
            ],
    outfile = 'usr/lib/libGraphicsMagick++.a',
    deps = [':libgraphicsmagick']
)
