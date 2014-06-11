
import sys
import xml.etree.ElementTree as xml

def main():

    if (len(sys.argv) < 2):
        print 'Must specify which AndroidManifest file to update'
        exit(1)
    
    f = open('product.build.number',"r")
    build_number = int(f.read().strip())
    f.close()

    f = open('product.version.number','r')
    product_version = f.read().strip()
    f.close()
    
    version_name = product_version + '.' + str(build_number)
    print 'versionName:' + version_name
    
    version_array = version_name.split('.')
    
    # 2^31 = 2147483648 values
    # 20.99.99.9999
    if (build_number > 9999) or (int(version_array[2]) > 99) or (int(version_array[1]) > 99) or (int(version_array[0]) > 20):
        print 'A portion of the version string does not fit within "20.99.99.9999"'
        exit(1)
    
    encoded_version = build_number
    encoded_version += int(version_array[2]) * 10000
    encoded_version += int(version_array[1]) * 10000 * 100
    encoded_version += int(version_array[0]) * 10000 * 100 * 100
    
    print 'versionCode:' + str(encoded_version)

    path = sys.argv[1]
    f = open(path,'r')
    blah = f.read()

    prefix = 'android'
    uri = 'http://schemas.android.com/apk/res/android'
    namespace = '{'+uri+'}'

    try:
        xml.register_namespace(prefix, uri)
    except AttributeError:
        def register_namespace(prefix, uri):
            xml._namespace_map[uri] = prefix

    register_namespace(prefix, uri)

    tree = xml.parse(path)
    root = tree.getroot()

    root.attrib[namespace+'versionCode'] = str(encoded_version)
    root.attrib[namespace+'versionName'] = version_name

    
    f = open(path,'w')
    xml.ElementTree(root).write(f)
    f.close()
    
    print 'Updated "' + path + '"'


if __name__ == '__main__':
    main()
