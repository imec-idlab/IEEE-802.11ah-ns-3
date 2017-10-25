[Installation instructions for CoAP support](../../wiki/coap)

### Installation and usage instructions ###

* Download source code from our repository.
* Change into ./802.11ah-ns3/ns-3 directory.  
* Build and run:

    ./waf --run "s1g-mac-test --seed=1 --simulationTime=60 --payloadSize=256 --Nsta=32 --NRawSta=32 --BeaconInterval=100000 --DataMode="OfdmRate7_8MbpsBW2MHz" --datarate=7.8  --bandWidth=2 --rho="50" --TrafficPath="./OptimalRawGroup/traffic/data-32-0.82.txt" --S1g1MfieldEnabled=false --RAWConfigFile="./OptimalRawGroup/RawConfig-32-2-2.txt" "

Note: if errors related to nullptr arise when compiling, CXXFLAGS should be included into the "./waf configure" command,as follows:   

CXXFLAGS="-std=c++0x" ./waf configure --disable-examples --disable-tests

  
### RAW related parameters: ###
* NRawSta:             Number of stations supporting RAW. 
* RAWConfigFile:       RAW configuration are stored in this file.

      The RAWConfigFile "./OptimalRawGroup/RawConfig-32-2-2.txt" used in the above command sets 2 RAW group for 32 stations, each RAW group contains 2 RAW slots. This file contains three lines:

    2                                                                                                          
    0	1	1	412	2	0	1	16	
    0	1	1	412	2	0	17	32

line 1: number of RAW Groups                                 
line 2 and line 3, information for each RAW groups, including

          * RawControl           Whether RAW can be accessed by any stations within the RAW group or only the paged stations within the RAW group.  
          * CrossSlotBoundary    Whether STAs are allowed to transmit after the assigned RAW slot boundary.
          * SlotFormat           Format of RAW slot count.                 
          * NRawSlotCount        Used to calculate number of RAW slot duration.   
          * NRawSlotNum          Number of slots per RAW group.                     
          * Page                 Page index of the subset of AIDs.
          * Aid_start            Station with the lowest AID allocated in the RAW.
          * Aid_end              Station with the highest AID allocated in the RAW.


Notes:   
          1. RawControl, currently set to 0, RAW can be accessed by any stations within the RAW group.            
          2. CrossSlotBoundary, currenty set to 1, only cross slot boundary are supported.                    
          3. RAW slot duration = 500 us + NRawSlotCount * 120 us, NRawSlotCount is y = 11(8) bits   length when SlotFormat is set to 1(0), and NRawSlotNum is (14 - y) bits length.                                     
          4. The above  RAWConfigFile assumes BeaconInterval is 100000 us and whole beacon is occupied by the RAW group. Users can adjust the parameters based on their own needs.                       


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
