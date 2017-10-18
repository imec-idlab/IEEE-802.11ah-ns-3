#!/bin/bash

if [ $# -ne 6 ]
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
#poissonrate=$4
#Nactive=$5


payloadSize=$4

#trafficPoissonrate=$7
#trafficMaxRate=$8

numGroup=$5

numSlot=$6



simTime=100
BeaconInterval=102400
DataMode="OfdmRate7_8MbpsBW2MHz"
datarate=7.8
bandWidth=2
S1g1MfieldEnabled=false


rho="50"

echo $seed
totalSta=$NumSta
totalRawstas=$NumSta


if [ ! -d OptimalRawGroup/ ]
then
    mkdir OptimalRawGroup/
fi

if [ ! -d OptimalRawGroup/datarate-$datarate/ ]
then
mkdir OptimalRawGroup/datarate-$datarate//
fi

if [ ! -d OptimalRawGroup/datarate-$datarate/traffic-$traffic/ ]
then
    mkdir OptimalRawGroup/datarate-$datarate/traffic-$traffic/
fi

if [ ! -d OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/ ]
then
mkdir OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/
fi

if [ ! -d OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/ ]
then
mkdir OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/
fi

if [ ! -d OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/simResult-$NumSta-seed$seed ]
then
    mkdir OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/simResult-$NumSta-seed$seed/
fi

Outputpath="OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/simResult-$NumSta-seed$seed/"

TrafficPath="./OptimalRawGroup/traffic/data-$NumSta-$traffic.txt"
RAWConfigPath="./OptimalRawGroup/RawConfig-$NumSta-$numGroup-$numSlot.txt"


touch ./OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/simResult-$NumSta-seed$seed/simlog.txt

#./waf --run "s1g-mac-raw-optimal-changestation-poisson-interfaceupdpwn-or-changetraffic-circle-circle --seed=$seed --simulationTime=$simTime --payloadSize=$payloadSize --Nsta=$totalSta --NRawSta=$totalRawstas --BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth --Totaltraffic=$traffic  --rho=$rho --Outputpath=$Outputpath --TrafficPath=$TrafficPath --Nactive=$Nactive --poissonrate=$poissonrate --S1g1MfieldEnabled=$S1g1MfieldEnabled --trafficPoissonrate=$trafficPoissonrate --trafficMaxRate=$trafficMaxRate" > ./OptimalRawGroup/traffic-$traffic/poissonrate-$poissonrate/simResult-$NumSta-seed$seed/trafficPoissonrate$trafficPoissonrate-trafficMaxRate$trafficMaxRate/simlog.txt 2>&1


#./waf --run "s1g-mac-test --seed=1 --simulationTime=100 --payloadSize=256 --Nsta=8 --NRawSta=8 --BeaconInterval=100000 --DataMode="OfdmRate7_8MbpsBW2MHz" --datarate=7.8  --bandWidth=2 --rho="50" --TrafficPath="./OptimalRawGroup/traffic/data-8-0.82.txt" --S1g1MfieldEnabled=false --RAWConfigFile="./OptimalRawGroup/RawConfig-8-2-2.txt" "

#./waf --run "s1g-mac-test --seed=1 --simulationTime=100 --payloadSize=256 --Nsta=32 --NRawSta=32 --BeaconInterval=100000 --DataMode="OfdmRate7_8MbpsBW2MHz" --datarate=7.8  --bandWidth=2 --rho="50" --TrafficPath="./OptimalRawGroup/traffic/data-32-0.82.txt" --S1g1MfieldEnabled=false --RAWConfigFile="./OptimalRawGroup/RawConfig-32.txt" "


./waf --run "s1g-mac-test --seed=$seed --simulationTime=$simTime --payloadSize=$payloadSize --Nsta=$totalSta --NRawSta=$totalRawstas --BeaconInterval=$BeaconInterval --DataMode=$DataMode --datarate=$datarate  --bandWidth=$bandWidth --rho=$rho --Outputpath=$Outputpath --TrafficPath=$TrafficPath  --S1g1MfieldEnabled=$S1g1MfieldEnabled --RAWConfigPath=$RAWConfigPath " > ./OptimalRawGroup/datarate-$datarate/traffic-$traffic/numGroup-$numGroup/numSlot-$numSlot/simResult-$NumSta-seed$seed/simlog.txt 2>&1
