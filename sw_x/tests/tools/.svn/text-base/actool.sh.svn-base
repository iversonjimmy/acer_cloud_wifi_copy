#!/bin/bash

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 domain [ccd-data-dir]"
    exit 0
fi

domain=$1
ccd_data_dir=${2:-/temp/igware}

ccd_conf_tmpl="${SRCROOT:-$WORKAREA/sw_x}/gvm_core/conf/ccd.conf.tmpl"

if [[ ! -r "$ccd_conf_tmpl" ]]; then
    echo "ERROR: cannot read from $ccd_conf_tmpl"
    exit 1
fi

ccd_conf=$ccd_data_dir/conf/ccd.conf
ccd_conf1=$ccd_data_dir/conf/ccd.conf.1
echo "Writing '$ccd_conf' based on '$ccd_conf_tmpl'" 

mkdir -p $(dirname $ccd_conf)
sed -e 's/\${DOMAIN}/'"$domain"'/g' $ccd_conf_tmpl > $ccd_conf1
sed -e 's/\${GROUP}/'""'/g' $ccd_conf1 > $ccd_conf
rm $ccd_conf1
