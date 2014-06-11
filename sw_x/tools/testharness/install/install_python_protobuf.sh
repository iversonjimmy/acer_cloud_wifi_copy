#!/bin/sh

# NOTE: This script must be executed with root permission!

TOOL_HOST="pcstore.ctbg.acer.com"
HTTP_PORT="8001"
TOOL_PATH="pc/tools"
COMPONENT_SRC="//${TOOL_HOST}:${HTTP_PORT}/${TOOL_PATH}/third_party/platform_linux"
PROTOBUF="protobuf-2.3.0"
PROTOBUF_TAR="${PROTOBUF}.tar"
PROTOBUF_ZIP="${PROTOBUF_TAR}.bz2"

CMD_DIR=`dirname $0`
cd $CMD_DIR
CMD_DIR=`pwd`

# get components
echo ""
echo "##########"
echo "# GET COMPONENT"
echo "##########"
rm -f ${PROTOBUF_ZIP}*
rm -rf ${PROTOBUF}
wget --progress=dot:mega http:${COMPONENT_SRC}/${PROTOBUF_ZIP}

# unpack
echo ""
echo "##########"
echo "# UNPACK"
echo "##########"
bunzip2 ${PROTOBUF_ZIP}
tar xvf ${PROTOBUF_TAR}

# install protobuf
echo ""
echo "##########"
echo "# INSTALL"
echo "##########"
cd ${PROTOBUF}/python
python setup.py install

# cleanup
cd $CMD_DIR
rm -rf ${PROTOBUF} ${PROTOBUF_TAR}
