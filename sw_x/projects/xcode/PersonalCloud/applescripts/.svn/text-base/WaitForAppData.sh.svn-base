#!/bin/bash

if [ -z "$1" ]; then 
    echo "Please input ios app data path"
    exit -1
else
    App_Data_Path=$1
    echo "App_Data_Path is $App_Data_Path"
fi

sleep 1

if [ -d "$App_Data_Path" ]; then
    echo "App Data is exist"
else
    echo "App Data is not exist, please check save path"
    exit -1
fi

Timeout=60

Old_Size=$(du -sk "$App_Data_Path" | awk '{ print $1 }')
New_Size=0

while [ $Timeout -gt 0 ]; do
    sleep 2
    New_Size=$(du -sk "$App_Data_Path" | awk '{ print $1 }')
    if [ "$Old_Size" -lt "$New_Size" ]; then
        echo "Old_Size is $Old_Size"
        echo "New_Size is $New_Size"
        let Timeout=Timeout-2
        let Old_Size=New_Size
        echo "App Data is downloading"
        echo "Timeout for downloading left: $Timeout"
        if [ $Timeout -eq 0 ]; then
            echo "Timeout reach for downloading app data"
        fi
    else
        echo "Download app data is completed"
        break
    fi
done

exit 0
