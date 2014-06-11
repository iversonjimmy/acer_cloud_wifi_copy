#!/bin/bash

if [[ -z "${SRCROOT}" ]]; then
    echo "ERROR: SRCROOT must be defined"
    exit 1
fi

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 image-directory"
    exit 0
fi
imagedir=$1

cd ${SRCROOT}/${imagedir}
IMAGEFILES=
for f in *.png; do
    if [[ -z "${IMAGEFILES}" ]]; then
	IMAGEFILES="$f"
    else
	IMAGEFILES="${IMAGEFILES};$f"
    fi
done

echo "${IMAGEFILES}"
