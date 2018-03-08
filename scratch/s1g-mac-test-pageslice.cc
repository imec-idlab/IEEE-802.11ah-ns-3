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


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include "ns3/rps.h"
#include <utility> // std::pair
#include <map>

using namespace std;
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("s1g-wifi-network-le");

uint32_t AssocNum = 0;
int64_t AssocTime=0;
uint32_t StaNum=0;
string traffic_filepath;
uint32_t payloadLength;
NetDeviceContainer staDeviceCont;


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
Ipv4InterfaceContainer staNodeInterface;

int getStaAidFromAddress (Ipv4Address address)
{
	int staAid = -1;
	for (uint32_t i = 0; i < staNodeInterface.GetN(); i++){
		if (staNodeInterface.GetAddress(i) == address){
			staAid = i + 1;
			break;
		}
	}
	return staAid;
}

std::vector<uint32_t> clientSentList;
std::vector<uint32_t> clientReceivedList;
std::vector<uint32_t> serverReceivedList;
uint32_t clientSent = 0;
uint32_t serverReceived = 0;
uint32_t clientReceived = 0;
void udpPacketReceivedAtServer (Ptr<const Packet> packet, Address from)
{
	serverReceived++;
	int id = getStaAidFromAddress(InetSocketAddress::ConvertFrom(from).GetIpv4());
	if (id != -1)
		serverReceivedList[--id]++;
	else
		std::cout << "Packet not included in final statistics. Node could not be determined from address" << std::endl;
}

void udpPacketSentByClient (Ptr<const Packet> packet)
{
	clientSent++;
}
void udpPacketReceivedAtClient (Ptr<const Packet> packet, Address from)
{
	clientReceived++;
}

void CheckAssoc (uint32_t Nsta, double simulationTime, NodeContainer wifiApNode, NodeContainer  wifiStaNode, Ipv4InterfaceContainer apNodeInterface)
{


    if  (GetAssocNum () == Nsta)
      {
        std::cout << "Assoc Finished, AssocNum=" << AssocNum << ", time = " << Simulator::Now ().GetMicroSeconds () << std::endl;
        //Application start time
        Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
        //UDP flow
        /*UdpEchoServerHelper myServer (9);
        //ApplicationContainer serverApp;
        serverApp = myServer.Install (wifiStaNode.Get(0));
        serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&udpPacketReceivedAtServer));
        serverApp.Start (Seconds (0));

        UdpEchoClientHelper echoClient (apNodeInterface.GetAddress (0), 9);
         echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
         echoClient.SetAttribute ("PacketSize", UintegerValue (payloadLength));

        UdpClientHelper myClient (apNodeInterface.GetAddress (0), 9); //address of remote node
        myClient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
        myClient.SetAttribute ("PacketSize", UintegerValue (payloadLength));*/

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
           else cout << "Unable to open traffic file \n";

          
          double randomStart = 0.0;
          
          for (std::map<uint16_t,float>::iterator it=traffic_sta.begin(); it!=traffic_sta.end(); ++it)
              {
                  std::ostringstream intervalstr;
                  intervalstr << (payloadLength*8)/(it->second * 1000000);
                  std::string intervalsta = intervalstr.str();

                  UdpEchoServerHelper myServer (9);
                          //ApplicationContainer serverApp;
                          serverApp = myServer.Install (wifiStaNode.Get(it->first));
                          serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&udpPacketReceivedAtServer));
                          serverApp.Start (Seconds (0));

                          UdpEchoClientHelper echoClient (staNodeInterface.GetAddress (it->first), 9);
                           echoClient.SetAttribute ("MaxPackets", UintegerValue (10));
                           echoClient.SetAttribute ("PacketSize", UintegerValue (payloadLength));
                  it++;
                  echoClient.SetAttribute ("Interval", TimeValue (Time (intervalsta)));
                  //myClient.SetAttribute ("Interval", TimeValue (Time (intervalsta))); //packets/s
                  randomStart = m_rv->GetValue (0, (payloadLength*8)/(it->second * 1000));
                  ApplicationContainer clientApps = echoClient.Install (wifiStaNode.Get(it->first));
                  clientApps.Get(0)->TraceConnectWithoutContext("Tx", MakeCallback(&udpPacketSentByClient));
                  clientApps.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&udpPacketReceivedAtClient));

                  clientApps.Start (Seconds (1 + randomStart));
                  clientApps.Stop (Seconds (simulationTime+1));
                  /*ApplicationContainer clientApp = myClient.Install (wifiStaNode.Get(it->first));

                  clientApp.Start (Seconds (1 + randomStart));
                  clientApp.Stop (Seconds (simulationTime+1)); //*/
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
        for (uint16_t kk=0; kk< NRPS; kk++)
        {
            RPS *m_rps = new RPS;
            myfile >> NRAWPERBEACON;

            for (uint16_t i = 0; i < NRAWPERBEACON; i++)
            {
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
        }
        myfile.close();
    }
    else cout << "Unable to open RAW config file \n";
    
    return rpslist;
}

/*
pageslice element and TIM(DTIM) together accomplish page slicing.

Prior knowledge:
802.11ah support up to 8192 stations, they are constructed into: page, block,
 subblock, sta.
there are 13 bit represent the AID of stations.
 AID[11-12] represent page.
 AID[6-10] represent block.
 AID[3-5] represent subblock.
 AID[0-2] represent sta.
 
A TIM(DTIM) element only support one page
A Page slice element only support one page
 
 Concept of page slicing:
 Between two DTIM beacon, there are many TIM beacons, only allow a TIM beacon include some blocks of one page is called page slice. One TIM beacon is called a page slice.
 Page slcie element specify number of page slice between two DTIM, number of blocks in each
 page slice.
 Page slice element only appears together with DTIM.
 
 Details:
 Page slice element also indicates AP has buffered data for which block, if a station is in that block, the station should first sleep, then wake up at coresponding page slice(TIM beacon) which includes that block.
 
 When station wake up at that block, it check whether AP has data for itself. If has, keep awake to receive packets and go to sleep in the next beacon.
 */
pageSlice configurePageSlice (pageSlice m_page)
{
    m_page.SetPageindex (0);
    
    m_page.SetPagePeriod (4); //2 TIM groups between DTIMs
    
    m_page.SetPageSliceLen (7); //each TIM group has 1 block (2 blocks in 2 TIM groups)
    m_page.SetPageSliceCount (4); //
    m_page.SetBlockOffset (0); //
    m_page.SetTIMOffset (0);
    
    return m_page;
    // page 0
    // 8 TIM(page slice) for one page
    // 4 block (each page)
    // 8 page slice
    // both offset are 0
}

TIM configureTIM (TIM m_TIM)
{
    m_TIM.SetPageIndex (0);
    
    m_TIM.SetDTIMPeriod (4);
    // 8 pages between two DTIM
    return m_TIM;
}
uint64_t numDropped(0);
void packetDropped (string context, Ptr<const Packet> packet, DropReason reason)
{
	numDropped++;
	std::cout << "drop reason=" << reason << std::endl;
}
uint64_t numCollisions(0);
void onCollision (std::string context, uint32_t numBackoffSlots)
{
	// trace not tested, not sure if it works correctly
	std::cout << "COLLISION SENSED" << std::endl;
	numCollisions++;
}

int main (int argc, char *argv[])
{

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("TIM", LOG_LEVEL_DEBUG);
  LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("ApWifiMac", LOG_LEVEL_INFO);

  LogComponentEnable ("EdcaTxopN", LOG_LEVEL_INFO);

  //LogComponentEnable ("DcaTxop", LOG_LEVEL_ALL);
  double simulationTime = 200;
  uint32_t seed = 1;
  uint32_t  payloadSize = 200;//256
  uint32_t Nsta = 10;
  uint32_t NRawSta = Nsta;
  uint32_t BeaconInterval = 100000;
  bool OutputPosition = true;
  string DataMode = "OfdmRate7_8MbpsBW2MHz";
  double datarate = 7.8;
  double bandWidth = 2;
  string rho="50.0";
  string folder="./scratch/";
  string file="./scratch/mac-sta.txt";
  string TrafficPath="./OptimalRawGroup/traffic/data-32-0.82.txt";
  uint16_t Nactive;
  double poissonrate;
  bool S1g1MfieldEnabled;
  string RAWConfigFile="./OptimalRawGroup/RawConfig-32-2-2.txt";
    

  CommandLine cmd;
  cmd.AddValue ("seed", "random seed", seed);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("payloadSize", "Size of payload", payloadSize);
  cmd.AddValue ("Nsta", "number of total stations", Nsta);
  cmd.AddValue ("NRawSta", "number of stations supporting RAW", NRawSta);
  cmd.AddValue ("BeaconInterval", "Beacon interval time in us", BeaconInterval);
  cmd.AddValue ("DataMode", "Date mode", DataMode);
  cmd.AddValue ("datarate", "data rate in Mbps", datarate);
  cmd.AddValue ("bandWidth", "bandwidth in MHz", bandWidth);
  cmd.AddValue ("rho", "maximal distance between AP and stations", rho);
  cmd.AddValue ("folder", "folder where result files are placed", folder);
  cmd.AddValue ("file", "files containing reslut information", file);
  cmd.AddValue ("TrafficPath", "files path of traffic file", TrafficPath);
  cmd.AddValue ("S1g1MfieldEnabled", "S1g1MfieldEnabled", S1g1MfieldEnabled);
  cmd.AddValue ("RAWConfigFile", "RAW Config file Path", RAWConfigFile);


  cmd.Parse (argc,argv);
  serverReceivedList.assign(Nsta,0);
  RngSeedManager::SetSeed (seed);

  NodeContainer wifiStaNode;
  wifiStaNode.Create (Nsta);
  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper ();
  channel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel","Exponent", DoubleValue(3.76) ,"ReferenceLoss", DoubleValue(8.0), "ReferenceDistance", DoubleValue(1.0));
  channel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetErrorRateModel ("ns3::YansErrorRateModel");
  phy.SetChannel (channel.Create ());
  phy.Set ("ShortGuardEnabled", BooleanValue (false));
  phy.Set ("ChannelWidth", UintegerValue (bandWidth));
  phy.Set ("EnergyDetectionThreshold", DoubleValue (-110.0));
  phy.Set ("CcaMode1Threshold", DoubleValue (-113.0));
  phy.Set ("TxGain", DoubleValue (0.0));
  phy.Set ("RxGain", DoubleValue (0.0));
  phy.Set ("TxPowerLevels", UintegerValue (1));
  phy.Set ("TxPowerEnd", DoubleValue (0.0));
  phy.Set ("TxPowerStart", DoubleValue (0.0));
  phy.Set ("RxNoiseFigure", DoubleValue (6.8));
  phy.Set ("LdpcEnabled", BooleanValue (true));
  phy.Set ("S1g1MfieldEnabled", BooleanValue (S1g1MfieldEnabled));

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ah);
  S1gWifiMacHelper mac = S1gWifiMacHelper::Default ();



  Ssid ssid = Ssid ("ns380211ah");
  StringValue DataRate;
  DataRate = StringValue (DataMode);

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", DataRate, "ControlMode", DataRate);

  mac.SetType ("ns3::StaWifiMac",
                "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevice;
  staDevice = wifi.Install (phy, mac, wifiStaNode);

  RPSVector rps;
  rps = configureRAW (rps, RAWConfigFile);
  pageSlice pageS;
  pageS = configurePageSlice (pageS);
  TIM m_TIM;
  m_TIM = configureTIM (m_TIM);
    
  mac.SetType ("ns3::ApWifiMac",
                 "Ssid", SsidValue (ssid),
                 "BeaconInterval", TimeValue (MicroSeconds(BeaconInterval)),
                 "NRawStations", UintegerValue (NRawSta),
                 "RPSsetup", RPSVectorValue (rps),
                 "PageSliceSet", pageSliceValue (pageS),
                 "TIMSet", TIMValue (m_TIM)
               );

  NetDeviceContainer apDevice;
  phy.Set ("TxGain", DoubleValue (3.0));
  phy.Set ("RxGain", DoubleValue (3.0));
  phy.Set ("TxPowerLevels", UintegerValue (1));
  phy.Set ("TxPowerEnd", DoubleValue (30.0));
  phy.Set ("TxPowerStart", DoubleValue (30.0));
  phy.Set ("RxNoiseFigure", DoubleValue (6.8));

  apDevice = wifi.Install (phy, mac, wifiApNode);

  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxPacketNumber", UintegerValue(10));
  Config::Set ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/MaxDelay", TimeValue (NanoSeconds (6000000000000)));
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/BE_EdcaTxopN/Queue/PacketDropped", MakeCallback (&packetDropped));
  Config::Connect ("/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/$ns3::StaWifiMac/Collision", MakeCallback (&onCollision));


  // mobility.
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator",
                                       "X", StringValue ("1000.0"),
                                       "Y", StringValue ("1000.0"),
                                       "rho", StringValue (rho));
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
  Ipv4InterfaceContainer apNodeInterface;

  staNodeInterface = address.Assign (staDevice);
  apNodeInterface = address.Assign (apDevice);

  //trace association
  for (uint16_t kk=0; kk< Nsta; kk++)
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



    Simulator::ScheduleNow(&UdpTraffic, Nsta,payloadSize, TrafficPath, staDevice);

    Simulator::Schedule(Seconds(1), &CheckAssoc, Nsta, simulationTime, wifiApNode, wifiStaNode,apNodeInterface);


    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
    PopulateArpCache ();

      if  (OutputPosition)
        {
          int i =0;
          while (i < Nsta)
            {
               Ptr<MobilityModel> mobility = wifiStaNode.Get (i)->GetObject<MobilityModel>();
               Vector position = mobility->GetPosition();
               std::cout << "Sta node#" << i << ", " << "position = " << position << std::endl;
               i++;
            }
          Ptr<MobilityModel> mobility1 = wifiApNode.Get (0)->GetObject<MobilityModel>();
          Vector position = mobility1->GetPosition();
          std::cout << "AP node, position = " << position << std::endl;
        }

      Simulator::Run ();
      Simulator::Destroy ();

      double throughput = 0;
      //UDP
      uint32_t a = serverReceived + clientReceived;
      //uint32_t totalPacketsThrough = DynamicCast<UdpServer> (serverApp.Get (0))->GetReceived ();
      throughput = a * payloadSize * 8 / (simulationTime * 1000000.0);
      std::cout << "datarate" << "\t" << "throughput" << std::endl;
      std::cout << datarate << "\t" << throughput << " Mbit/s" << std::endl;

      std::cout << "total sent packets = " << clientSent << std::endl;
      std::cout << "total received packets at server = " << serverReceived << std::endl;
      std::cout << "total succesfully echoed packets by server = " << clientReceived << std::endl;
      std::cout << "dropped from wifi mac queue = " << numDropped << std::endl;

    /*
      for (int i=0; i< Nsta; i++){
      std::cout << "[sta=" << i+1 << "] successfully sent packets = " << serverReceivedList[i] << std::endl;
      std::cout << "[sta=" << i+1 << "] successfully received packets = " << clientReceivedList[i] << std::endl;
      }*/
return 0;
}
