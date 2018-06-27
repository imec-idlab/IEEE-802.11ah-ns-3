#time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-199-1.txt\"  --TrafficInterval=1324 --CycleTime=128 --nControlLoops=1  " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-198-2.txt\"  --TrafficInterval=1470 --CycleTime=128 --nControlLoops=2  " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-196-4.txt\"  --TrafficInterval=1894 --CycleTime=128 --nControlLoops=4  " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-194-6.txt\"  --TrafficInterval=2685 --CycleTime=128 --nControlLoops=6  " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-192-8.txt\"  --TrafficInterval=4682 --CycleTime=128 --nControlLoops=8  " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --rho=400 --TrafficType=\"coap\" --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-loops-wired/RawConfig-190-10.txt\"  --TrafficInterval=19456 --CycleTime=128 --nControlLoops=10  " -j4
