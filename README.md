If you use our work, you can use the following citation to refer to it:
> Le Tian, Amina Sljivo, Serena Santi, Eli De Poorter, Jeroen Hoebeke, Jeroen Famaey. **Extension of the IEEE 802.11ah NS-3 Simulation Module.** Workshop on ns-3 (WNS3), 2018.

You can find the paper on [ResearchGate](https://www.researchgate.net/publication/324910418_Extension_of_the_IEEE_80211ah_ns-3_Simulation_Module).

### IEEE 802.11ah ns-3 
> alpha version

This repository hosts an implementation of the IEEE802.11ah (Wi-Fi HaLow) protocol for the NS-3 network simulator. It comes pre-packaged with NS-3 version 3.23 but multiple modules are updated to the 3.25 version to incorporate congestion control fixes for TCP traffic.

This module includes support for:
* Restricted Access Window (RAW) with interface for dynamic configuration
* Traffic Indication Map (TIM) segmentation 
* Energy consumption model
* Adaptive Modulation and Coding Scheme (MCS)

### Installation and usage instructions ###
* Clone the project from git
* Follow the instructions on https://www.nsnam.org/wiki/Installation to prepare all dependencies. For Ubuntu, those are:
```
sudo apt-get update
sudo apt-get -y install gcc g++ python
sudo apt-get -y install gcc g++ python python-dev
sudo apt-get -y install qt4-dev-tools libqt4-dev
sudo apt-get -y install mercurial
sudo apt-get -y install bzr
sudo apt-get -y install cmake libc6-dev libc6-dev-i386 g++-multilib
sudo apt-get -y install gdb valgrind
sudo apt-get -y install gsl-bin libgsl2 libgsl2:i386
sudo apt-get -y install flex bison libfl-dev
sudo apt-get -y install tcpdump
sudo apt-get -y install sqlite sqlite3 libsqlite3-dev
sudo apt-get -y install libxml2 libxml2-dev
sudo apt-get -y install libgtk2.0-0 libgtk2.0-dev
sudo apt-get -y install vtun lxc
sudo apt -y install git
git clone https://github.com/imec-idlab/IEEE-802.11ah-ns-3.git
```
* Change into IEEE-802.11ah-ns-3 directory.  
* Configure waf:
`CXXFLAGS="-std=c++11" ./waf configure --disable-examples --disable-tests`
* Build:
`./waf`
* Run the simulation (if you use [ahVisualizer](https://github.com/imec-idlab/ahVisualizer) start it first):
`./waf --run test`
    * Pass the arguments for desired setup e.g.:
    
    `./waf --run "test --seed=1 --simulationTime=60 --payloadSize=256"`
  
  
### RAW related parameters: ###
* NRawSta:             Number of stations supporting RAW. NRawSta equals the largest AID specified in RAWConfigFile.
* RAWConfigFile:       RAW configuration is stored in this file.

The default RAWConfigFile "./OptimalRawGroup/RawConfig-32-2-2.txt" sets 2 RPSs (RAW Parameter Sets) to be broadcasted with beacons in cycles (e.g. Beacon1: RPS1, Beacon2: RPS2, Beacon3: RPS1, Beacon4: RPS2, ...). The first RPS contains 2 RAW groups for 63 stations, each RAW group contains 2 RAW slots. The second RPS contains 1 RAW group for 4 stations (64-67), where the RAW group contains 3 slots. This file consists of 6 lines:

```
2
2
0	  1	  1	  200	  2	  0	  1	29
0	  1	  1	  200	  2	  0	  30	63
1
0	  1	  1	  220	  3	  0	  64	67
```
line 1: NRPS = number of RPS elements

line x: NRAW_k = number of RAW Groups in the RPS k; x = (NRAW_0 + NRAW_1 + ... + NRAW_(k-1)) + 1 + k; NRAW_0 = 0 

        for RPS number k=1, line x=0+1+1=2 contains information on number of RAW groups in the RPS
        
        for RPS number k=2 and NRAW_1=2 (from line 2), line x=0+2+1+2=5 contains information on number of RAW groups in the RPS
        
line 3, line 4 and line 6: configuration of each individual RAW group, including

          * RawControl           Whether RAW can be accessed by any stations within the RAW group or only the paged stations within the RAW group.  
          * CrossSlotBoundary    Whether STAs are allowed to transmit after the assigned RAW slot boundary.
          * SlotFormat           Format of RAW slot count.                 
          * NRawSlotCount        Used for calculation of RAW slot duration.   
          * NRawSlotNum          Number of slots per RAW group.                     
          * Page                 Page index of the subset of AIDs.
          * Aid_start            Station with the lowest AID allocated in the RAW.
          * Aid_end              Station with the highest AID allocated in the RAW.


Notes:   
          1. RawControl, currently set to 0, RAW can be accessed by any stations within the RAW group.            
          2. CrossSlotBoundary, currenty set to 1, only cross slot boundary are supported.                    
          3. RAW slot duration = 500 us + NRawSlotCount * 120 us, NRawSlotCount is y = 11(8) bits   length when SlotFormat is set to 1(0), and NRawSlotNum is (14 - y) bits length.                                     
          4. The above  RAWConfigFile assumes BeaconInterval is 102400 us. Users can adjust the parameters based on their own needs.                       


### Wi-Fi mode parameters ###
* DataMode:           Data mode.  
* datarate:           Data rate of data mode.  
* bandWidth:          BandWidth of data mode.  

Note: Relation between the above 3 parameters and MCS is described in file "MCStoWifiMode".       

### Other parameters ###
* SimulationTime:     Simulatiom time in seconds after all stations get associated with AP.  
* payloadSize:        Size of payload.                   
* BeaconInterval:     Beacon interval time in us.    
* UdpInterval:        Traffic mode, station send one packet every UdpInterval seconds.  
* Nsta:               Number of total stations.  
* rho:                Maximal distance between AP and stations.   
* seed:               Seed of RandomVariableStream.
* TrafficPath:        Include traffic of each stations, packet sending interval can be automatically calcualted based on payloadSize. The above TrafficPath "./OptimalRawGroup/traffic/data-32-0.82.txt" contains traffic of 32 stations, and the total traffic is 0.82 Mbps.
* S1g1MfieldEnabled:     Packet using 1 Mhz bandwidth if set to "true".


### TIM and page slice parameters ###
* pagePeriod:       Number of Beacon Intervals between DTIM beacons that carry Page Slice element for the associated page
* pageIndex:        Index of the page (0-3) whose slices are served during beacon intervals within a page period, default value is 0
* pageSliceLength:  Number of blocks in each TIM for the associated page except for the last TIM (1-31)
* pageSliceCount:   Number of TIMs in a single page period (0-31), value 0 has special meaning
* blockOffset:      The 1st page slice starts with the block with blockOffset number, default value is 0
* timOffset:        Offset in number of Beacon Intervals from the DTIM that carries the first page slice of the page, default value is 0

    Example run: ./waf --run "test --pagePeriod=4 --pageSliceLength=8 --pageSliceCount=4"
    
    * pagePeriod=4: every 4th beacon is DTIM beacon that carries Page Slice element for pageIndex=0
    * pageSliceLength=8: each page slice cosists of 8 blocks, meaning that each page slice (PS) accomodates up to 512 stations (8 blocks * 8 subblocks * 8 stations)
      * (slice0: 1-512, slice1: 513-1024, slice2: 1025-1536, slice3: 1537-2048)
      * The last page slice can have different length.
   * pageSliceCount=4: 4 TIMs are scheduled in one page period.
    
    Note: RAW configuration must be in line with TIM and page configuration. If a RAW group is reserved for a station in beacon interval that does not correspond to its TIM, station will be asleep during that RAW.
    
    To configure a single page slice (whole page encoded in a single page slice), it is neccessary to set pageSliceCount to 0 and pageSliceLength to 1.

### Rate control Algorithm (RCA) ###
The 802.11ah nodes can always use the same MCS as specified by "Wi-Fi mode parameters" when "ConstantRateWifiManager" is used. The nodes can also adapt the MCSs dynamically when rata control algorithm is used. Details about Rate control Algorithms can be found on https://www.nsnam.org/docs/models/html/wifi-design.html#rate-control-algorithms.

An example of using MinstrelWifiManager is given in scratch/rca, where one mobile station continuously transmits 64-byte UDP packets to the AP, while moving from -500 m to 500 m relative to the centrally placed AP at the speed of 1 m/s.

`./waf --run "rca --seed=1 --simulationTime=60 --payloadSize=256"`


### Further reading
For more information on the implementation of the IEEE 802.11ah module for ns-3, check our [recent WNS3 paper on ResearchGate](https://www.researchgate.net/publication/324910418_Extension_of_the_IEEE_80211ah_ns-3_Simulation_Module).
