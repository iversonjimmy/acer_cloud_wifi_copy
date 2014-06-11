#!/bin/sh -x

TEMPLATE_FILE="winrt.execute_build.template"
OUTPUT_FILE="winrt.execute_build"

sed \
    -e "s|@@WORKDIR@@|$WORKDIR|" \
    -e "s|@@BUILD_BRANCH@@|$BUILD_BRANCH|" \
    -e "s|@@BUILD_DATE@@|$BUILD_DATE|" \
    -e "s|@@BUILD_SVN_DATE@@|$BUILD_SVN_DATE|" \
    -e "s|@@BUILD_CVS_DATE@@|$BUILD_CVS_DATE|" \
    -e "s|@@BUILD_CVS_CO_TAG@@|$BUILD_CVS_CO_TAG|" \
    -e "s|@@IS_MTV_BUILD@@|$IS_MTV_BUILD|" \
    -e "s|@@MTV_SVN_PATH@@|$MTV_SVN_PATH|" \
    $TEMPLATE_FILE > $OUTPUT_FILE

if [ ! -s $OUTPUT_FILE ]; then
    echo "The generated file ($OUTPUT_FILE) either does not exist or is empty!"
    exit 1
else
   echo "Generated file $OUTPUT_FILE"
   exit 0
fi
