time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --TrafficType=\"coap\" --TrafficInterval=1078 --nControlLoops=0 --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-200-1-5-102400-0-31.txt\" " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --TrafficType=\"coap\" --TrafficInterval=931 --nControlLoops=0 --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-200-1-5-102400-0-31.txt\" " -j4

time ./waf --run "test --BeaconInterval=102400 --pagePeriod=1 --pageSliceLength=1 --pageSliceCount=0 --simulationTime=500 --payloadSize=64 --TrafficType=\"coap\" --TrafficInterval=683 --nControlLoops=0 --PrintStats=true --RAWConfigFile=\"./OptimalRawGroup/RawConfig-200-1-5-102400-0-31.txt\" " -j4
