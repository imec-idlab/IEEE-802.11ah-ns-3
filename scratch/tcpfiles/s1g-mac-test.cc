/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "s1g-mac-test.h"

NS_LOG_COMPONENT_DEFINE ("s1g-wifi-network-le");

uint32_t AssocNum = 0;
int64_t AssocTime=0;
uint32_t StaNum=0;
string traffic_filepath;
uint32_t payloadLength;
NetDeviceContainer staDeviceCont;

Configuration config;
Statistics stats;
SimulationEventManager eventManager;

class assoc_record
{
public:
    assoc_record ();
    bool GetAssoc ();
    void SetAssoc (std::string context, Mac48Address address);
    void UnsetAssoc (std::string context, Mac48Address address);
    void setstaid (uint16_t id);
 private:
    bool assoc;
    uint16_t staid;
};

assoc_record::assoc_record ()
{
    assoc = false;
    staid = 65535;
}

void
assoc_record::setstaid (uint16_t id)
{
    staid = id;
}

void
assoc_record::SetAssoc (std::string context, Mac48Address address)
{
   assoc = true;
}

void
assoc_record::UnsetAssoc (std::string context, Mac48Address address)
{
  assoc = false;
}

bool
assoc_record::GetAssoc ()
{
    return assoc;
}

typedef std::vector <assoc_record * > assoc_recordVector;
assoc_recordVector assoc_vector;


uint32_t
GetAssocNum ( )
{
  AssocNum = 0;
  for (assoc_recordVector::const_iterator index = assoc_vector.begin (); index != assoc_vector.end (); index++)
    {
      if ((*index)->GetAssoc ())
        {
            AssocNum++;
        }
    }
  return AssocNum;
}


void
UdpTraffic ( uint16_t num, uint32_t Size, string path, NetDeviceContainer staDevice)
{
  StaNum = num;
  payloadLength = Size;
  traffic_filepath = path;
  staDeviceCont = staDevice;
}

ApplicationContainer serverApp;
uint32_t AppStartTime = 0;
uint32_t ApStopTime = 0;

std::map<uint16_t, float> traffic_sta;




void CheckAssoc (uint32_t Nsta, double simulationTime, NodeContainer wifiApNode, NodeContainer  wifiStaNode, Ipv4InterfaceContainer apNodeInterface)
{


    if  (GetAssocNum () == Nsta)
      {
        std::cout << "Assoc Finished, AssocNum=" << AssocNum << ", time = " << Simulator::Now ().GetMicroSeconds () << std::endl;
        //Application start time
        Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
        //UDP flow
        UdpServerHelper myServer (9);
        serverApp = myServer.Install (wifiApNode);
        serverApp.Start (Seconds (0));

        UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9); //address of remote node
        myClient.SetAttribute ("MaxPackets", config.maxNumberOfPackets);
        myClient.SetAttribute ("PacketSize", UintegerValue (payloadLength));

          traffic_sta.clear ();
          
          ifstream trafficfile (traffic_filepath);
          
         
           if (trafficfile.is_open())
                 {
                  uint16_t sta_id;
                  float  sta_traffic;
                  for (uint16_t kk=0; kk< Nsta; kk++)
                    {
                      trafficfile >> sta_id;
                      trafficfile >> sta_traffic;
                      traffic_sta.insert (std::make_pair(sta_id,sta_traffic)); //insert data
                      //cout << "sta_id = " << sta_id << " sta_traffic = " << sta_traffic << "\n";
                    }
                  trafficfile.close();
                }
           else cout << "Unable to open file \n";

          
          double randomStart = 0.0;
          
          for (std::map<uint16_t,float>::iterator it=traffic_sta.begin(); it!=traffic_sta.end(); ++it)
              {
                  std::ostringstream intervalstr;
                  intervalstr << (payloadLength*8)/(it->second * 1000000);
                  std::string intervalsta = intervalstr.str();

                  myClient.SetAttribute ("Interval", TimeValue (Time (intervalsta))); //packets/s
                  randomStart = m_rv->GetValue (0, (payloadLength*8)/(it->second * 1000000));
                  ApplicationContainer clientApp = myClient.Install (wifiStaNode.Get(it->first));

                  clientApp.Start (Seconds (1 + randomStart));
                  clientApp.Stop (Seconds (simulationTime+1)); //
               }
          
              AppStartTime=Simulator::Now ().GetSeconds () + 1;
              Simulator::Stop (Seconds (simulationTime+1));
      }
    else
      {
        Simulator::Schedule(Seconds(1.0), &CheckAssoc, Nsta, simulationTime, wifiApNode, wifiStaNode, apNodeInterface);
      }

}



void
PopulateArpCache ()
{
    Ptr<ArpCache> arp = CreateObject<ArpCache> ();
    arp->SetAliveTimeout (Seconds(3600 * 24 * 365));
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=
            interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            NS_ASSERT(ipIface != 0);
            Ptr<NetDevice> device = ipIface->GetDevice();
            NS_ASSERT(device != 0);
            Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress ());
            for(uint32_t k = 0; k < ipIface->GetNAddresses (); k ++)
            {
                Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
                if(ipAddr == Ipv4Address::GetLoopback())
                    continue;
                ArpCache::Entry * entry = arp->Add(ipAddr);
                entry->MarkWaitReply(0);
                entry->MarkAlive(addr);
                std::cout << "Arp Cache: Adding the pair (" << addr << "," << ipAddr << ")" << std::endl;
            }
        }
    }
    for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i)
    {
        Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
        NS_ASSERT(ip !=0);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for(ObjectVectorValue::Iterator j = interfaces.Begin(); j !=interfaces.End (); j ++)
        {
            Ptr<Ipv4Interface> ipIface = (j->second)->GetObject<Ipv4Interface> ();
            ipIface->SetAttribute("ArpCache", PointerValue(arp));
        }
    }
}

uint16_t ngroup;
uint16_t nslot;
RPSVector configureRAW (RPSVector rpslist, string RAWConfigFile)
{
    uint16_t NRPS = 0;
    uint16_t NRAWPERBEACON = 0;
    uint16_t Value = 0;
    uint32_t page = 0;
    uint32_t aid_start = 0;
    uint32_t aid_end = 0;
    uint32_t rawinfo = 0;

    ifstream myfile (RAWConfigFile);
    //1. get info from config file

    //2. define RPS
    if (myfile.is_open())
    {
        myfile >> NRPS;

        for (uint16_t kk=0; kk< NRPS; kk++)   // number of beacons covering all raw groups
        {
            RPS *m_rps = new RPS;
            myfile >> NRAWPERBEACON;
            ngroup = NRAWPERBEACON;
            for (uint16_t i=0; i< NRAWPERBEACON; i++)  // raw groups in one beacon
            {
                //RPS *m_rps = new RPS;
                RPS::RawAssignment *m_raw = new RPS::RawAssignment;

                myfile >> Value;
                m_raw->SetRawControl (Value);//support paged STA or not
                myfile >> Value;
                m_raw->SetSlotCrossBoundary (Value);
                myfile >> Value;
                m_raw->SetSlotFormat (Value);
                myfile >> Value;
                m_raw->SetSlotDurationCount (Value);
                myfile >> Value;
                nslot = Value;
                m_raw->SetSlotNum (Value);
                myfile >> page;
                myfile >> aid_start;
                myfile >> aid_end;
                rawinfo = (aid_end << 13) | (aid_start << 2) | page;
                m_raw->SetRawGroup (rawinfo);

                m_rps->SetRawAssignment(*m_raw);
                delete m_raw;
            }
            rpslist.rpsset.push_back (m_rps);
            //config.nRawGroupsPerRpsList.push_back(NRAWPERBEACON);
        }
        myfile.close();
        config.NRawSta = rpslist.rpsset[rpslist.rpsset.size()-1]->GetRawAssigmentObj(NRAWPERBEACON-1).GetRawGroupAIDEnd();
    }
    else cout << "Unable to open file \n";

    return rpslist;
}


void sendStatistics(bool schedule) {
	eventManager.onUpdateStatistics(stats);
	eventManager.onUpdateSlotStatistics(transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval, transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval);
	// reset
	std::fill(transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.begin(), transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval.end(), 0);
	std::fill(transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.begin(), transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval.end(), 0);

	if(schedule)
		Simulator::Schedule(Seconds(config.visualizerSamplingInterval), &sendStatistics, true);
}

void onSTADeassociated(int i) {
	eventManager.onNodeDeassociated(*nodes[i]);
}

void updateNodesQueueLength() {
	for (uint32_t i = 0; i < config.Nsta; i++) {
		nodes[i]->UpdateQueueLength();
		stats.get(i).EDCAQueueLength = nodes[i]->queueLength;
	}
	Simulator::Schedule(Seconds(0.5), &updateNodesQueueLength);
}

void onSTAAssociated(int i) {
	cout << "Node " << std::to_string(i) << " is associated and has aid " << nodes[i]->aId << endl;

	uint32_t nrOfSTAAssociated (0);
	for (uint32_t i = 0; i < config.Nsta; i++) {
		if (nodes[i]->isAssociated)
			nrOfSTAAssociated++;
	}
	eventManager.onNodeAssociated(*nodes[i]);

	// RPS, Raw group and RAW slot assignment
	if (nrOfSTAAssociated == config.Nsta) {
		cout << "All stations associated, configuring clients & server" << endl;
        // association complete, start sending packets
    	stats.TimeWhenEverySTAIsAssociated = Simulator::Now();

    	/*if(config.trafficType == "udp") {
    		configureUDPServer();
    		configureUDPClients();
    	}
    	else if(config.trafficType == "udpecho") {
    		configureUDPEchoServer();
    		configureUDPEchoClients();
    	}
    	else if(config.trafficType == "tcpecho") {
			configureTCPEchoServer();
			configureTCPEchoClients();
    	}
    	else if(config.trafficType == "tcppingpong") {
    		configureTCPPingPongServer();
			configureTCPPingPongClients();
		}
    	else if(config.trafficType == "tcpipcamera") {
			configureTCPIPCameraServer();
			configureTCPIPCameraClients();
		}
    	else if(config.trafficType == "tcpfirmware") {
			configureTCPFirmwareServer();
			configureTCPFirmwareClients();
		}
    	else if(config.trafficType == "tcpsensor") {
			configureTCPSensorServer();
			configureTCPSensorClients();
    	}
    	else if (config.trafficType == "coap") {
			configureCoapServer();
			configureCoapClients();
		}*/
        updateNodesQueueLength();
    }
}

void RpsIndexTrace (uint16_t oldValue, uint16_t newValue)
{
	currentRps = newValue;
	//cout << "RPS: " << newValue << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawGroupTrace (uint8_t oldValue, uint8_t newValue)
{
	currentRawGroup = newValue;
	//cout << "	group " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void RawSlotTrace (uint8_t oldValue, uint8_t newValue)
{
	currentRawSlot = newValue;
	//cout << "		slot " << std::to_string(newValue) << " at " << Simulator::Now().GetMicroSeconds() << endl;
}

void configureNodes(NodeContainer& wifiStaNode, NetDeviceContainer& staDevice) {
	cout << "Configuring STA Node trace sources" << endl;

    for (uint32_t i = 0; i < config.Nsta; i++) {

    	cout << "Hooking up trace sources for STA " << i << endl;

        NodeEntry* n = new NodeEntry(i, &stats, wifiStaNode.Get(i), staDevice.Get(i));

        n->SetAssociatedCallback([ = ]{onSTAAssociated(i);});
        n->SetDeassociatedCallback([ = ]{onSTADeassociated(i);});

        nodes.push_back(n);
        // hook up Associated and Deassociated events
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc", MakeCallback(&NodeEntry::SetAssociation, n)); //happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc", MakeCallback(&NodeEntry::UnsetAssociation, n)); //never
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/NrOfTransmissionsDuringRAWSlot", MakeCallback(&NodeEntry::OnNrOfTransmissionsDuringRAWSlotChanged, n)); //NEVER

        //Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/S1gBeaconMissed", MakeCallback(&NodeEntry::OnS1gBeaconMissed, n));

        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/PacketDropped", MakeCallback(&NodeEntry::OnMacPacketDropped, n)); //never
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Collision", MakeCallback(&NodeEntry::OnCollision, n)); //never
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/TransmissionWillCrossRAWBoundary", MakeCallback(&NodeEntry::OnTransmissionWillCrossRAWBoundary, n)); //never

        // hook up TX
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback(&NodeEntry::OnPhyTxBegin, n)); //happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxEnd", MakeCallback(&NodeEntry::OnPhyTxEnd, n)); // never happens TODO
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyTxDropWithReason", MakeCallback(&NodeEntry::OnPhyTxDrop, n)); // never happens TODO

        // hook up RX
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxBegin", MakeCallback(&NodeEntry::OnPhyRxBegin, n)); //happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback(&NodeEntry::OnPhyRxEnd, n)); //happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason", MakeCallback(&NodeEntry::OnPhyRxDrop, n)); //happens

        // hook up MAC traces
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxRtsFailed", MakeCallback(&NodeEntry::OnMacTxRtsFailed, n)); //never happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxDataFailed", MakeCallback(&NodeEntry::OnMacTxDataFailed, n)); //happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalRtsFailed", MakeCallback(&NodeEntry::OnMacTxFinalRtsFailed, n)); //never happens
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/RemoteStationManager/MacTxFinalDataFailed", MakeCallback(&NodeEntry::OnMacTxFinalDataFailed, n)); //never happens

        // hook up PHY State change
        Config::Connect("/NodeList/" + std::to_string(i) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/State/State", MakeCallback(&NodeEntry::OnPhyStateChange, n));

    }
}

int getBandwidth(string dataMode) {
	if(dataMode == "MCS1_0" ||
	   dataMode == "MCS1_1" || dataMode == "MCS1_2" ||
		dataMode == "MCS1_3" || dataMode == "MCS1_4" ||
		dataMode == "MCS1_5" || dataMode == "MCS1_6" ||
		dataMode == "MCS1_7" || dataMode == "MCS1_8" ||
		dataMode == "MCS1_9" || dataMode == "MCS1_10")
			return 1;

	else if(dataMode == "MCS2_0" ||
	    dataMode == "MCS2_1" || dataMode == "MCS2_2" ||
		dataMode == "MCS2_3" || dataMode == "MCS2_4" ||
		dataMode == "MCS2_5" || dataMode == "MCS2_6" ||
		dataMode == "MCS2_7" || dataMode == "MCS2_8")
			return 2;

	return 0;
}

string getWifiMode(string dataMode) {
	if(dataMode == "MCS1_0") return "OfdmRate300KbpsBW1MHz";
	else if(dataMode == "MCS1_1") return "OfdmRate600KbpsBW1MHz";
	else if(dataMode == "MCS1_2") return "OfdmRate900KbpsBW1MHz";
	else if(dataMode == "MCS1_3") return "OfdmRate1_2MbpsBW1MHz";
	else if(dataMode == "MCS1_4") return "OfdmRate1_8MbpsBW1MHz";
	else if(dataMode == "MCS1_5") return "OfdmRate2_4MbpsBW1MHz";
	else if(dataMode == "MCS1_6") return "OfdmRate2_7MbpsBW1MHz";
	else if(dataMode == "MCS1_7") return "OfdmRate3MbpsBW1MHz";
	else if(dataMode == "MCS1_8") return "OfdmRate3_6MbpsBW1MHz";
	else if(dataMode == "MCS1_9") return "OfdmRate4MbpsBW1MHz";
	else if(dataMode == "MCS1_10") return "OfdmRate150KbpsBW1MHz";


	else if(dataMode == "MCS2_0") return "OfdmRate650KbpsBW2MHz";
	else if(dataMode == "MCS2_1") return "OfdmRate1_3MbpsBW2MHz";
	else if(dataMode == "MCS2_2") return "OfdmRate1_95MbpsBW2MHz";
	else if(dataMode == "MCS2_3") return "OfdmRate2_6MbpsBW2MHz";
	else if(dataMode == "MCS2_4") return "OfdmRate3_9MbpsBW2MHz";
	else if(dataMode == "MCS2_5") return "OfdmRate5_2MbpsBW2MHz";
	else if(dataMode == "MCS2_6") return "OfdmRate5_85MbpsBW2MHz";
	else if(dataMode == "MCS2_7") return "OfdmRate6_5MbpsBW2MHz";
	else if(dataMode == "MCS2_8") return "OfdmRate7_8MbpsBW2MHz";
	return "";
}

void OnAPPhyRxDrop(std::string context, Ptr<const Packet> packet, DropReason reason) {
	// THIS REQUIRES PACKET METADATA ENABLE!
	unused (context);
	auto pCopy = packet->Copy();
	auto it = pCopy->BeginItem();
	while(it.HasNext()) {

		auto item = it.Next();
		Callback<ObjectBase *> constructor = item.tid.GetConstructor ();

		ObjectBase *instance = constructor ();
		Chunk *chunk = dynamic_cast<Chunk *> (instance);
		chunk->Deserialize (item.current);

		if(dynamic_cast<WifiMacHeader*>(chunk)) {
			WifiMacHeader* hdr = (WifiMacHeader*)chunk;

			int staId = -1;
			if (!config.useV6){
				for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
					if(wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			}
			else
			{
				for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
					if(wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == hdr->GetAddr2()) {
						staId = i;
						break;
					}
				}
			}
			if(staId != -1) {
				stats.get(staId).NumberOfDropsByReasonAtAP[reason]++;
			}
			delete chunk;
			break;
		}
		else
			delete chunk;
	}


}

void OnAPPacketToTransmitReceived(string context, Ptr<const Packet> packet, Mac48Address to, bool isScheduled, bool isDuringSlotOfSTA, Time timeLeftInSlot) {
	unused(context);
	unused(packet);
	unused(isDuringSlotOfSTA);
	int staId = -1;
	if (!config.useV6){
		for (uint32_t i = 0; i < staNodeInterface.GetN(); i++) {
			if(wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	}
	else
	{
		for (uint32_t i = 0; i < staNodeInterface6.GetN(); i++) {
			if(wifiStaNode.Get(i)->GetDevice(0)->GetAddress() == to) {
				staId = i;
				break;
			}
		}
	}
	if(staId != -1) {
		if(isScheduled)
			stats.get(staId).NumberOfAPScheduledPacketForNodeInNextSlot++;
		else {
			stats.get(staId).NumberOfAPSentPacketForNodeImmediately++;
			stats.get(staId).APTotalTimeRemainingWhenSendingPacketInSameSlot += timeLeftInSlot;
		}
	}
}

void onChannelTransmission(Ptr<NetDevice> senderDevice, Ptr<Packet> packet) {
	int rpsIndex = currentRps - 1;
	int rawGroup = currentRawGroup - 1;
	int slotIndex = currentRawSlot - 1;
	//cout << rpsIndex << "		" << rawGroup << "		" << slotIndex << "		" << endl;

	uint64_t iSlot = slotIndex;
	if (rpsIndex > 0)
		for (int r = rpsIndex - 1; r >= 0; r--)
			for (int g = 0; g < config.rps.rpsset[r]->GetNumberOfRawGroups(); g++)
				iSlot += config.rps.rpsset[r]->GetRawAssigmentObj(g).GetSlotNum();

	if (rawGroup > 0)
		for (int i = rawGroup - 1; i >= 0; i--)
			iSlot += config.rps.rpsset[rpsIndex]->GetRawAssigmentObj(i).GetSlotNum();

	if (rpsIndex >= 0 && rawGroup >= 0 && slotIndex >= 0)
		if(senderDevice->GetAddress() == apDevice.Get(0)->GetAddress()) {
			// from AP
			transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval[iSlot]+= packet->GetSerializedSize();
		}
		else {
			// from STA
			transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval[iSlot]+= packet->GetSerializedSize();

		}
}

int main (int argc, char *argv[])
{
  //LogComponentEnable ("UdpServer", LOG_INFO);


  /*double simulationTime = 10;
  uint32_t seed = 1;
  uint32_t  payloadSize = 256;
  uint32_t Nsta =1;
  uint32_t NRawSta = 1;
  uint32_t BeaconInterval = 100000;*/
  bool OutputPosition = true;
  /*string DataMode = "OfdmRate7_8MbpsBW2MHz";
  double datarate = 7.8;
  double bandWidth = 2;
  string rho="250.0";
  string folder="./scratch/";
  string file="./scratch/mac-sta.txt";
  string TrafficPath;
  uint16_t Nactive;
  double poissonrate;
  bool S1g1MfieldEnabled;
  string RAWConfigFile;*/
  config = Configuration(argc, argv);

  config.rps = configureRAW (config.rps, config.RAWConfigFile);
  config.Nsta = config.NRawSta;

  stats = Statistics(config.Nsta);
  eventManager = SimulationEventManager(config.visualizerIP, config.visualizerPort, config.NSSFile);
  uint32_t totalRawGroups (0);
  for (int i = 0; i < config.rps.rpsset.size(); i++)
  {
	  int nRaw = config.rps.rpsset[i]->GetNumberOfRawGroups();
	  totalRawGroups += nRaw;
	  //cout << "Total raw groups after rps " << i << " is " << totalRawGroups << endl;
	  for (int j = 0; j < nRaw; j++){
		  config.totalRawSlots += config.rps.rpsset[i]->GetRawAssigmentObj(j).GetSlotNum();
		  //cout << "Total slots after group " << j << " is " << totalRawSlots << endl;
	  }

  }
  transmissionsPerTIMGroupAndSlotFromAPSinceLastInterval = vector<long>(config.totalRawSlots, 0);
  transmissionsPerTIMGroupAndSlotFromSTASinceLastInterval = vector<long>(config.totalRawSlots, 0);
  /*CommandLine cmd;
  cmd.AddValue ("seed", "random seed", config.seed);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", config.simulationTime);
  cmd.AddValue ("payloadSize", "Size of payload", config.payloadSize);
  cmd.AddValue ("Nsta", "number of total stations", config.Nsta);
  cmd.AddValue ("NRawSta", "number of stations supporting RAW", config.NRawSta);
  cmd.AddValue ("BeaconInterval", "Beacon interval time in us", config.BeaconInterval);
  cmd.AddValue ("DataMode", "Date mode", config.DataMode);
  cmd.AddValue ("datarate", "data rate in Mbps", config.datarate);
  cmd.AddValue ("bandWidth", "bandwidth in MHz", config.bandWidth);
  cmd.AddValue ("rho", "maximal distance between AP and stations", config.rho);
  cmd.AddValue ("folder", "folder where result files are placed", config.folder);
  cmd.AddValue ("file", "files containing reslut information", config.file);
  cmd.AddValue ("TrafficPath", "files path of traffic file", config.TrafficPath);
  cmd.AddValue ("S1g1MfieldEnabled", "S1g1MfieldEnabled", config.S1g1MfieldEnabled);
  cmd.AddValue ("RAWConfigFile", "RAW Config file Path", config.RAWConfigFile);


  cmd.Parse (argc,argv);*/

  RngSeedManager::SetSeed (config.seed);

  //NodeContainer wifiStaNode;
  wifiStaNode.Create (config.Nsta);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channelBuilder = YansWifiChannelHelper ();
  channelBuilder.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent", DoubleValue(3.76) ,"ReferenceLoss", DoubleValue(8.0), "ReferenceDistance", DoubleValue(1.0));
  channelBuilder.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  Ptr<YansWifiChannel> channel = channelBuilder.Create();
  channel->TraceConnectWithoutContext("Transmission", MakeCallback(&onChannelTransmission)); //TODO

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetErrorRateModel ("ns3::YansErrorRateModel");
  phy.SetChannel (channel);
  phy.Set ("ShortGuardEnabled", BooleanValue (false));
  phy.Set ("ChannelWidth", UintegerValue (getBandwidth(config.DataMode))); // changed
  phy.Set ("EnergyDetectionThreshold", DoubleValue (-110.0));
  phy.Set ("CcaMode1Threshold", DoubleValue (-113.0));
  phy.Set ("TxGain", DoubleValue (0.0));
  phy.Set ("RxGain", DoubleValue (0.0));
  phy.Set ("TxPowerLevels", UintegerValue (1));
  phy.Set ("TxPowerEnd", DoubleValue (0.0));
  phy.Set ("TxPowerStart", DoubleValue (0.0));
  phy.Set ("RxNoiseFigure", DoubleValue (6.8));
  phy.Set ("LdpcEnabled", BooleanValue (true));
  phy.Set ("S1g1MfieldEnabled", BooleanValue (config.S1g1MfieldEnabled));

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ah);
  S1gWifiMacHelper mac = S1gWifiMacHelper::Default ();



  Ssid ssid = Ssid ("ns380211ah");
  StringValue DataRate;
  DataRate = StringValue (getWifiMode(config.DataMode)); // changed

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate, "ControlMode", DataRate);

  mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid),
                 "BeaconInterval", TimeValue (MicroSeconds(config.BeaconInterval)),
                 "NRawStations", UintegerValue (config.NRawSta),
                 "RPSsetup", RPSVectorValue (config.rps));

  phy.Set ("TxGain", DoubleValue (3.0));
  phy.Set ("RxGain", DoubleValue (3.0));
  phy.Set ("TxPowerLevels", UintegerValue (1));
  phy.Set ("TxPowerEnd", DoubleValue (30.0));
  phy.Set ("TxPowerStart", DoubleValue (30.0));
  phy.Set ("RxNoiseFigure", DoubleValue (6.8));

  apDevice = wifi.Install (phy, mac, wifiApNode);

  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(10));
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));

  std::ostringstream oss;
  oss << "/NodeList/"
		  << wifiApNode.Get(0)->GetId()
		  << "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::ApWifiMac/";
  Config::ConnectWithoutContext(oss.str () + "RpsIndex", MakeCallback (&RpsIndexTrace));
  Config::ConnectWithoutContext(oss.str () + "RawGroup", MakeCallback (&RawGroupTrace));
  Config::ConnectWithoutContext(oss.str () + "RawSlot", MakeCallback(&RawSlotTrace));


  // mobility.
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                       "X", StringValue ("1000.0"),
                                       "Y", StringValue ("1000.0"),
                                       "rho", StringValue (config.rho));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(wifiStaNode);


  MobilityHelper mobilityAp;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (1000.0, 1000.0, 0.0));
  mobilityAp.SetPositionAllocator (positionAlloc);
  mobilityAp.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityAp.Install(wifiApNode);


   /* Internet stack*/
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNode);

  Ipv4AddressHelper address;

  address.SetBase ("192.168.0.0", "255.255.0.0");
  //Ipv4InterfaceContainer staNodeInterface;
  Ipv4InterfaceContainer apNodeInterface;

  staNodeInterface = address.Assign (staDevice);
  apNodeInterface = address.Assign (apDevice);

  //trace association
  for (uint16_t kk=0; kk< config.Nsta; kk++)
    {
      std::ostringstream STA;
      STA << kk;
      std::string strSTA = STA.str();

      assoc_record *m_assocrecord=new assoc_record;
      m_assocrecord->setstaid (kk);
      Config::Connect ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Assoc", MakeCallback (&assoc_record::SetAssoc, m_assocrecord));
      Config::Connect ("/NodeList/"+strSTA+"/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/DeAssoc", MakeCallback (&assoc_record::UnsetAssoc, m_assocrecord));
      assoc_vector.push_back (m_assocrecord);
    }



    Simulator::ScheduleNow(&UdpTraffic, config.Nsta, config.payloadSize, config.TrafficPath, staDevice);

    Simulator::Schedule(Seconds(1), &CheckAssoc, config.Nsta, config.simulationTime, wifiApNode, wifiStaNode,apNodeInterface);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    PopulateArpCache ();


    // configure tracing for associations & other metrics
    configureNodes(wifiStaNode, staDevice);

	Config::Connect("/NodeList/" + std::to_string(config.Nsta) + "/DeviceList/0/$ns3::WifiNetDevice/Phy/PhyRxDropWithReason", MakeCallback(&OnAPPhyRxDrop));
	Config::Connect("/NodeList/" + std::to_string(config.Nsta) + "/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::ApWifiMac/PacketToTransmitReceivedFromUpperLayer", MakeCallback(&OnAPPacketToTransmitReceived));

    Ptr<MobilityModel> mobility1 = wifiApNode.Get (0)->GetObject<MobilityModel>();
    Vector apposition = mobility1->GetPosition();
      if  (OutputPosition)
        {
          uint32_t i =0;
          while (i < config.Nsta)
            {
               Ptr<MobilityModel> mobility = wifiStaNode.Get (i)->GetObject<MobilityModel>();
               Vector position = mobility->GetPosition();
               nodes[i]->x = position.x;
               nodes[i]->y = position.y;
               std::cout << "Sta node#" << i << ", " << "position = " << position << std::endl;
               i++;
            }
          std::cout << "AP node, position = " << apposition << std::endl;
        }

      //eventManager.onStartHeader();
      eventManager.onStart(config);
      if (config.rps.rpsset.size() > 0)
    	  for (uint32_t i = 0; i < config.rps.rpsset.size(); i++)
    		  for (uint32_t j = 0; j < config.rps.rpsset[i]->GetNumberOfRawGroups(); j++)
    			  eventManager.onRawConfig(i, j, config.rps.rpsset[i]->GetRawAssigmentObj(j));

      for (uint32_t i = 0; i < config.Nsta; i++)
      	eventManager.onSTANodeCreated(*nodes[i]);

      eventManager.onAPNodeCreated(apposition.x, apposition.y);

      sendStatistics(true);

      Simulator::Stop(Seconds(config.simulationTime + config.CoolDownPeriod)); // allow up to a minute after the client & server apps are finished to process the queue
      Simulator::Run ();
      Simulator::Destroy ();

      double throughput = 0;
      //UDP
      uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
      throughput = totalPacketsThrough * config.payloadSize * 8 / (config.simulationTime * 1000000.0);
      std::cout << "datarate" << "\t" << "throughput" << std::endl;
      std::cout << config.datarate << "\t" << throughput << " Mbit/s" << std::endl;
      return 0;
}
