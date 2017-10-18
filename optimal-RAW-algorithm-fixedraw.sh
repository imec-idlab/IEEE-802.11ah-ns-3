#!/bin/bash

if [ $# -ne 9 ]
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
poissonrate=$4
Nactive=$5


payloadSize=$6

trafficPoissonrate=$7
trafficMaxRate=$8

NRawGroups=1 #one raw group occupying the whole beacon
NumSlot=$9



simTime=100
BeaconInterval=102400
DataMode="OfdmRate7_8MbpsBW2MHz"
datarate=7.8
bandWidth=2
S1g1MfieldEnabled=false




rho="50"





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

if [ ! -d OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate ]
then
mkdir OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate
fi


if [ ! -d OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot ]
then
mkdir OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot
fi




if [ ! -d OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed ]
then
    mkdir OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/
fi

if [ ! -d OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate ]
then
mkdir OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/
else
rm -rf OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/
mkdir OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/
fi

#Outputpath="./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/aid-"
Outputpath="./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate"

TrafficPath="./OptimalRawGroup/traffic/data-$NumSta-$traffic.txt"


touch ./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/simlog.txt


RAWConfigPath="./OptimalRawGroup/RawConfig-$NumSta-$NRawGroups-$NumSlot.txt"
RAWConfiglogPath="./OptimalRawGroup/RawConfiglog.txt"

#./waf --run "RAW-optimal-algorithm-notraffic --NRawSta=$NumSta --NGroup=$NRawGroups --NumSlot=$NumSlot --RAWConfigPath=$RAWConfigPath "
#--RAWConfiglogPath=$RAWConfiglogPath



#./waf --run "s1g-mac-raw-optimal-changestation-poisson-interfaceupdpwn-or-changetraffic-circle-circle-fixecRawConfig --seed=$seed --simulationTime=$simTime --payloadSize=$payloadSize --Nsta=$totalSta --NRawSta=$totalRawstas --BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth --Totaltraffic=$traffic  --rho=$rho --Outputpath=$Outputpath --TrafficPath=$TrafficPath --Nactive=$Nactive --poissonrate=$poissonrate --S1g1MfieldEnabled=$S1g1MfieldEnabled --trafficPoissonrate=$trafficPoissonrate --trafficMaxRate=$trafficMaxRate --RAWConfigPath=$RAWConfigPath" > ./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/simlog.txt 2>&1

./waf --run "s1g-mac-test --seed=$seed --simulationTime=$simTime --payloadSize=$payloadSize --Nsta=$totalSta --NRawSta=$totalRawstas --BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth --Totaltraffic=$traffic  --rho=$rho --Outputpath=$Outputpath --TrafficPath=$TrafficPath --Nactive=$Nactive --poissonrate=$poissonrate --S1g1MfieldEnabled=$S1g1MfieldEnabled --trafficPoissonrate=$trafficPoissonrate --trafficMaxRate=$trafficMaxRate --RAWConfigPath=$RAWConfigPath" > ./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/NRawGroups$NRawGroups-NumSlot$NumSlot/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/simlog.txt 2>&1



