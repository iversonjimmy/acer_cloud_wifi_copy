#!/bin/sh
# Use this script to install python 2.6.5.

TOOL_HOST="pcstore.ctbg.acer.com"
HTTP_PORT="8001"
TOOL_PATH="pc/tools"
COMPONENT_SRC="//${TOOL_HOST}:${HTTP_PORT}/${TOOL_PATH}/third_party/platform_linux"
PYTHON=Python-2.6.5

# see if python is installed
type python > /dev/null 2>&1
if [ $? -ne 0 ]; then
    # no python installed
    echo "python not found"
    echo ""
else
    # show python version
    echo "Your version of python:"
    python -V
    echo ""
fi

echo "Continue with install?"
echo -n "Y/N: "
read RESPONSE
if [ $RESPONSE = "n" -o $RESPONSE = "N" ]; then
    exit
fi

ERR=0
rm -f ${PYTHON}.tgz
wget --progress=dot:mega http:${COMPONENT_SRC}/${PYTHON}.tgz
ERR=`expr $ERR + $?`
tar fxvz ${PYTHON}.tgz
ERR=`expr $ERR + $?`
cd ${PYTHON}
./configure
ERR=`expr $ERR + $?`
make
ERR=`expr $ERR + $?`
make install 
ERR=`expr $ERR + $?`

echo ""
if [ $ERR -eq 0 ]; then
    echo "install was successful"
else
    echo "install NOT successful"
fi
exit $ERR
