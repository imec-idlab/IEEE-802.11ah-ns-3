#!/bin/bash

if [ $# -ne 3 ]
then
echo "parameters missing"
exit 1
fi




NRawSta=$1
NGroup=$2
NumSlot=$3


RAWConfigPath="RawConfig-$NRawSta-$NGroup-$NumSlot.txt"
RAWConfiglogPath="RawConfig-$NRawSta-$NGroup-$NumSlot-log.txt"

#RAWConfigPath="RawConfig-$NRawSta-$NumSlot.txt"
#RAWConfiglogPath="RawConfig-$NRawSta-$NumSlot-log.txt"



./waf --run "RAW-optimal-algorithm-notraffic --NRawSta=$NRawSta --NGroup=$NGroup --NumSlot=$NumSlot  --RAWConfigPath=$RAWConfigPath --RAWConfiglogPath=$RAWConfiglogPath"

