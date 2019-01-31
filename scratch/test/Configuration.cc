#include "Configuration.h"

Configuration::Configuration() {
    
}

Configuration::Configuration(int argc, char *argv[]) {
    CommandLine cmd;
    /*cmd.AddValue("useIpv6", "Use Ipv6 (true/false)", useV6);
    cmd.AddValue("nControlLoops", "Number of control loops. If -1 all the stations will be in the loops if NSta is even", nControlLoops);
    cmd.AddValue("CoapPayloadSize", "Size of CoAP payload",coapPayloadSize);
     */
    cmd.AddValue("seed", "random seed", seed);
    cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
    cmd.AddValue("Nsta", "number of total stations", Nsta);
    cmd.AddValue("NRawSta", "number of stations supporting RAW. If -1 it will be based on NSta, should be divisible by NGroup", NRawSta);
    cmd.AddValue ("payloadSize", "Size of payload to send in bytes", payloadSize);
    cmd.AddValue("BeaconInterval", "Beacon interval time in us", BeaconInterval);
    cmd.AddValue("DataMode", "Date mode (check MCStoWifiMode for more details) (format: MCSbw_mcs, e.g. MCS1_0 is OfdmRate300KbpsBW1Mhz)", DataMode);
    cmd.AddValue("datarate", "data rate in Mbps", datarate);
    cmd.AddValue("bandWidth", "bandwidth in MHz", bandWidth);
    cmd.AddValue("rho", "maximal distance between AP and stations", rho);
    cmd.AddValue ("folder", "folder where result files are placed", folder);
    cmd.AddValue ("file", "files containing reslut information", file);
    cmd.AddValue ("totaltraffic", "totaltraffic", totaltraffic);
    cmd.AddValue ("TrafficPath", "files path of traffic file", TrafficPath);
    cmd.AddValue ("S1g1MfieldEnabled", "S1g1MfieldEnabled", S1g1MfieldEnabled);
    cmd.AddValue ("RAWConfigFile", "RAW Config file Path", RAWConfigFile);
    cmd.AddValue("TrafficType", "Kind of traffic (udp, -udpecho, -tcpecho, tcpipcamera, -tcpfirmware, -tcpsensor, -coap)", trafficType);
    cmd.AddValue("NGroup", "number of RAW groups", NGroup);
    cmd.AddValue("NRawSlotNum", "number of slots per RAW", NRawSlotNum);
    cmd.AddValue("pagePeriod", "Number of Beacon Intervals between DTIM beacons that carry Page Slice element for the associated page", pagePeriod);
    cmd.AddValue("pageSliceLength", "Number of blocks in each TIM for the associated page except for the last TIM (1-31)", pageSliceLength);
    cmd.AddValue("pageSliceCount", "Number of TIMs in a single page period (0-31; value 0 for whole page to be encoded in a single (only) TIM)", pageSliceCount);
    cmd.AddValue("blockOffset", "The 1st page slice starts with the block with blockOffset", blockOffset);
    cmd.AddValue("timOffset", "Offset in number of Beacon Intervals from the DTIM that carries the first page slice of the page", timOffset);
    cmd.AddValue("TrafficInterval", "Traffic interval time in ms", trafficInterval);
    cmd.AddValue("Outputpath", "files path of each stations", OutputPath);

/*
    cmd.AddValue("SlotFormat", "format of NRawSlotCount, -1 will auto calculate based on raw slot num", SlotFormat);
    cmd.AddValue("NRawSlotCount", "RAW slot duration, , -1 will auto calculate based on raw slot num", NRawSlotCount);
    cmd.AddValue("NRawSlotNum", "number of slots per RAW", NRawSlotNum);
    cmd.AddValue("NGroup", "number of RAW groups", NGroup);

    cmd.AddValue("ContentionPerRAWSlot", "Calculate the NSta and NRawSta based on the amount of contention there has to be in a RAW slot. A contention of 0 means stations are uncontended. If -1 then the specified NSta will be used", ContentionPerRAWSlot);
    cmd.AddValue("ContentionPerRAWSlotOnlyInFirstGroup", "If true only the first TIM group will contain stations, this is to prevent huge simulation times while every TIM group behaves the same (true/false)", ContentionPerRAWSlotOnlyInFirstGroup);

    cmd.AddValue("MaxTimeOfPacketsInQueue", "Max nr of seconds packets can remain in the DCA queue", MaxTimeOfPacketsInQueue);

    cmd.AddValue("APAlwaysSchedulesForNextSlot", "AP Always schedules for next slot (true/false)", APAlwaysSchedulesForNextSlot);
    cmd.AddValue("APScheduleTransmissionForNextSlotIfLessThan", "AP schedules transmission for next slot if slot time is less than (microseconds)", APScheduleTransmissionForNextSlotIfLessThan);

    cmd.AddValue("TrafficInterval", "Traffic interval time in ms", trafficInterval);
    cmd.AddValue("TrafficIntervalDeviation", "Traffic interval deviation time in ms, each interval will have a random deviation between - dev/2 and + dev/2", trafficIntervalDeviation);



    cmd.AddValue("MinRTO", "Minimum retransmission timeout for TCP sockets in microseconds", MinRTO);
    cmd.AddValue("TCPConnectionTimeout", "TCP Connection timeout to use for all Tcp Sockets", TCPConnectionTimeout);

    cmd.AddValue("TCPSegmentSize", "TCP Segment size in bytes", TCPSegmentSize);
    cmd.AddValue("TCPInitialSlowStartThreshold", "TCP Initial slow start threshold in segments", TCPInitialSlowStartThreshold);
    cmd.AddValue("TCPInitialCwnd", "TCP Initial congestion window in segments", TCPInitialCwnd);


    cmd.AddValue("IpCameraMotionPercentage", "Probability the ip camera detects motion each second [0-1]", ipcameraMotionPercentage);
    cmd.AddValue("IpCameraMotionDuration", "Time in seconds to stream data when motion was detected", ipcameraMotionDuration);
    cmd.AddValue("IpCameraDataRate", "Data rate of the captured stream in kbps", ipcameraDataRate);

    cmd.AddValue("FirmwareSize", "Size of the firmware that will be sent to clients for update", firmwareSize);
    cmd.AddValue("FirmwareBlockSize", "The chunk size of a piece of firmware. The client has to acknowledge each chunk before the next will be sent", firmwareBlockSize);
    cmd.AddValue("FirmwareCorruptionProbability", "The probability that a firmware chunk gets corrupted and the client has to ask for it again", firmwareNewUpdateProbability);
    cmd.AddValue("FirmwareNewUpdateProbability", "The probability that a newer firmware version is available when the client polls the server (every trafficinterval)", firmwareNewUpdateProbability);

    cmd.AddValue("SensorMeasurementSize", "The size of the measurements taken by the sensor", sensorMeasurementSize);





    cmd.AddValue("PropagationLossExponent", "exponent gamma of the log propagation model, default values are outdoors", propagationLossExponent);
    cmd.AddValue("PropagationLossReferenceLoss", "reference loss of the log propagation model", propagationLossReferenceLoss);

    cmd.AddValue("VisualizerIP", "IP or hostname for the visualizer server, leave empty to not send data", visualizerIP);
    cmd.AddValue("VisualizerPort", "Port for the visualizer server", visualizerPort);
    cmd.AddValue("VisualizerSamplingInterval", "Sampling interval of statistics in seconds", visualizerSamplingInterval);


    cmd.AddValue("APPcapFile", "Name of the pcap file to generate at the AP, leave empty to omit generation", APPcapFile);

    cmd.AddValue("NSSFile", "Path of the nss file to write. Note: if a visualizer is active it will also save the nss file", NSSFile);
    cmd.AddValue("Name", "Name of the simulation", name);

    cmd.AddValue("CoolDownPeriod", "Period of no more traffic generation after simulation time (to allow queues to be processed) in seconds", CoolDownPeriod);
*/

    cmd.Parse(argc, argv);
}
