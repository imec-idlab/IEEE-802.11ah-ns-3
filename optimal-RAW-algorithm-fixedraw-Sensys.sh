#!/bin/bash

if [ $# -ne 4 ]
then
echo "parameters missing"
exit 1
fi


# main function (loop)
#  0. balance station with one RAW group and run ns-3 simluation.
#  1. read file get throughput, delay and idle time,
#   increase/decrease RAW groups, or get the optimal one and stop programming
#  2. reblance RAW
#  3. run ns-3 simulation, go back to function 1
#  ./waf  --run "s1g-mac-test --seed=$seed --simulationTime=$simTime --payloadSize=256 --Nsta=$totalSta --NRawSta=$totalRawstas BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth --rho=$rho --folder=$folder --file=$file --pcapfile=$pcapf" #> ./TestMacresult-s1g/mode0/test$trial/traffic-$UdpInterval/pcap-sta-$NumSta/$NGroup-groups-$NRawSlotNum-slots/record-$NumSta-$NGroup.txt 2>&1

seed=$1
NumSta=$2
traffic=$3

payloadSize=64


NRawGroups=$4 #one raw group occupying the whole beacon
NumSlot=4



simTime=100
BeaconInterval=102400
DataMode="MCS1_1"
datarate=0.6
bandWidth=1
S1g1MfieldEnabled=false



udpStart=0
udpEnd=$(($NumSta - 9))
tcpipcameraStart=$(($NumSta - 8))
tcpipcameraEnd=$(($NumSta - 1))
#at least one udp

rho="200"





echo $seed
echo $NumSlot
totalSta=$NumSta
totalRawstas=$NumSta


if [ ! -d OptimalRawGroup/ ]
then
    mkdir OptimalRawGroup/
fi

if [ ! -d OptimalRawGroup/traffic-$traffic/ ]
then
    mkdir OptimalRawGroup/traffic-$traffic/
fi

if [ ! -d OptimalRawGroup/traffic-$traffic/ ]
then
mkdir OptimalRawGroup/traffic-$traffic/
fi


if [ ! -d OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot ]
then
mkdir OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot
fi




if [ ! -d OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed ]
then
    mkdir OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/
fi

if [ ! -d OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/ ]
then
mkdir OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/
else
rm -rf OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/
mkdir OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/
fi

Outputpath="./OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/aid-"
TrafficPath="./OptimalRawGroup/traffic/data-$NumSta-$traffic.txt"


touch ./OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/simlog.txt


RAWConfigFile="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups.txt"



./waf --run "tcpfiles --seed=$seed --simulationTime=$simTime --payloadSize=$payloadSize --Nsta=$totalSta --NRawSta=$totalRawstas --BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth  --rho=$rho  --TrafficPath=$TrafficPath --S1g1MfieldEnabled=$S1g1MfieldEnabled  --RAWConfigFile=$RAWConfigFile --NRawSlotNum=$NumSlot --NGroup=$NRawGroups --totaltraffic=$traffic --tcpipcameraStart=$tcpipcameraStart --tcpipcameraEnd=$tcpipcameraEnd --udpStart=$udpStart --udpEnd=$udpEnd"  #> ./OptimalRawGroup/traffic-$traffic/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/simlog.txt 2>&1





