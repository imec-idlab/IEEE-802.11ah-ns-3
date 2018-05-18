#!/bin/bash

if [ $# -ne 4 ]
then
echo "parameters missing"
exit 1
fi

NumSta=$1
NRawGroups=$2
NumSlot=$3
beaconinterval=$4








RAWConfigPath="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups-$NumSlot-$beaconinterval.txt"
#-$pagePeriod-$pageSliceLength-$pageSliceCount

./waf --run "RAW-generate --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath --beaconinterval=$beaconinterval"

# --pagePeriod=$pagePeriod --pageSliceLength=$pageSliceLength --pageSliceCount=$pageSliceCount



