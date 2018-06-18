/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
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
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "qos-tag.h"
#include "wifi-phy.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mgt-headers.h"
//#include "extension-headers.h"
#include "mac-low.h"
#include "amsdu-subframe-header.h"
#include "msdu-aggregator.h"
#include "ns3/uinteger.h"
#include "wifi-mac-queue.h"
#include <map>



namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (ApWifiMac);

#define LOG_TRAFFIC(msg)	if(true) NS_LOG_DEBUG(Simulator::Now().GetMicroSeconds() << " " << msg << std::endl);

TypeId
ApWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ApWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<ApWifiMac> ()
    .AddAttribute ("BeaconInterval", "Delay between two beacons",
                   TimeValue (MicroSeconds (102400)),
                   MakeTimeAccessor (&ApWifiMac::GetBeaconInterval,
                                     &ApWifiMac::SetBeaconInterval),
                   MakeTimeChecker ())
    .AddAttribute ("BeaconJitter", "A uniform random variable to cause the initial beacon starting time (after simulation time 0) "
                   "to be distributed between 0 and the BeaconInterval.",
                   StringValue ("ns3::UniformRandomVariable"),
                   MakePointerAccessor (&ApWifiMac::m_beaconJitter),
                   MakePointerChecker<UniformRandomVariable> ())
    .AddAttribute ("Outputpath", "output path to store info of sensors and offload stations",
                   StringValue ("stationfile"),
                   MakeStringAccessor (&ApWifiMac::m_outputpath),
                   MakeStringChecker ())
    .AddAttribute ("EnableBeaconJitter", "If beacons are enabled, whether to jitter the initial send event.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ApWifiMac::m_enableBeaconJitter),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconGeneration", "Whether or not beacons are generated.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::SetBeaconGeneration,
                                        &ApWifiMac::GetBeaconGeneration),
                   MakeBooleanChecker ())
    .AddAttribute ("ChannelWidth", "Channel width of the stations ",
                    UintegerValue (0),
                     MakeUintegerAccessor (&ApWifiMac::GetChannelWidth,
                                           &ApWifiMac::SetChannelWidth),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NRawGroupStas", "Number of stations in one Raw Group",
                   UintegerValue (6000),
                   MakeUintegerAccessor (&ApWifiMac::GetRawGroupInterval,
                                         &ApWifiMac::SetRawGroupInterval),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("NRawStations", "Number of total stations support RAW",
                   UintegerValue (100),
                   MakeUintegerAccessor (&ApWifiMac::GetTotalStaNum,
                                         &ApWifiMac::SetTotalStaNum),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SlotFormat", "Slot format",
                   UintegerValue (1),
                   MakeUintegerAccessor (&ApWifiMac::GetSlotFormat,
                                         &ApWifiMac::SetSlotFormat),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SlotCrossBoundary", "cross slot boundary or not",
                   UintegerValue (1),
                   MakeUintegerAccessor (&ApWifiMac::GetSlotCrossBoundary,
                                         &ApWifiMac::SetSlotCrossBoundary),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SlotDurationCount", "slot duration count",
                   UintegerValue (1000),
                   MakeUintegerAccessor (&ApWifiMac::GetSlotDurationCount,
                                         &ApWifiMac::SetSlotDurationCount),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("SlotNum", "Number of slot",
                   UintegerValue (2),
                   MakeUintegerAccessor (&ApWifiMac::GetSlotNum,
                                         &ApWifiMac::SetSlotNum),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("RPSsetup", "configuration of RAW",
                   RPSVectorValue (),
                   MakeRPSVectorAccessor (&ApWifiMac::m_rpsset),
                   MakeRPSVectorChecker ())
	.AddTraceSource ("S1gBeaconBroadcasted", "Fired when a beacon is transmitted",
	                 MakeTraceSourceAccessor(&ApWifiMac::m_transmitBeaconTrace),
	                 "ns3::ApWifiMac::S1gBeaconTracedCallback")
	.AddTraceSource ("RpsIndex", "Fired when RPS index changes",
					 MakeTraceSourceAccessor(&ApWifiMac::m_rpsIndexTrace),
					 "ns3::TracedValueCallback::Uint16")
	.AddTraceSource ("RawGroup", "Fired when RAW group index changes",
					 MakeTraceSourceAccessor(&ApWifiMac::m_rawGroupTrace),
					 "ns3::TracedValueCallback::Uint8")
	.AddTraceSource ("RawSlot", "Fired when RAW slot index changes",
					 MakeTraceSourceAccessor(&ApWifiMac::m_rawSlotTrace),
					 "ns3::TracedValueCallback::Uint8")
	/*.AddTraceSource("RAWSlotStarted",
					"Fired when a RAW slot has started",
					MakeTraceSourceAccessor(&ApWifiMac::m_rawSlotStarted),
					"ns3::S1gApWifiMac::RawSlotStartedCallback")*/
	.AddTraceSource("PacketToTransmitReceivedFromUpperLayer",
					"Fired when packet is received from the upper layer",
					MakeTraceSourceAccessor(
					&ApWifiMac::m_packetToTransmitReceivedFromUpperLayer),
					"ns3::S1gApWifiMac::PacketToTransmitReceivedFromUpperLayerCallback")
    .AddAttribute ("PageSliceSet", "configuration of PageSlice",
                   pageSliceValue (),
                   MakepageSliceAccessor (&ApWifiMac::m_pageslice),
                   MakepageSliceChecker ())
	.AddAttribute ("PageSlicingActivated", "Whether or not page slicing is activated.",
				    BooleanValue (true),
				    MakeBooleanAccessor (&ApWifiMac::SetPageSlicingActivated,
				                         &ApWifiMac::GetPageSlicingActivated),
				    MakeBooleanChecker ())
     .AddAttribute ("TIMSet", "configuration of TIM",
                   TIMValue (),
                   MakeTIMAccessor (&ApWifiMac::m_TIM),
                   MakeTIMChecker ());
     /*
       .AddAttribute ("DTIMPeriod", "TIM number in one of DTIM",
                   UintegerValue (4),
                   MakeUintegerAccessor (&ApWifiMac::GetDTIMPeriod,
                                         &ApWifiMac::SetDTIMPeriod),
                   MakeUintegerChecker<uint8_t> ()); 
       */
  return tid;
}

ApWifiMac::ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = CreateObject<DcaTxop> ();
  m_beaconDca->SetAifsn (1);
  m_beaconDca->SetMinCw (0);
  m_beaconDca->SetMaxCw (0);
  m_beaconDca->SetLow (m_low);
  m_beaconDca->SetManager (m_dcfManager);
  m_beaconDca->SetTxMiddle (m_txMiddle);

  //Let the lower layers know that we are acting as an AP.
  SetTypeOfStation (AP);

  m_enableBeaconGeneration = false;
  AuthenThreshold = 0;
  currentRawGroup = 0;
  //m_SlotFormat = 0;
  m_AidToMacAddr.clear ();
  m_accessList.clear ();
  m_supportPageSlicingList.clear();
  m_sleepList.clear ();
  m_DTIMCount = 0;
  //m_DTIMOffset = 0;
}

ApWifiMac::~ApWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
ApWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_beaconDca = 0;
  m_enableBeaconGeneration = false;
  m_beaconEvent.Cancel ();
  RegularWifiMac::DoDispose ();
}

void
ApWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  //As an AP, our MAC address is also the BSSID. Hence we are
  //overriding this function and setting both in our parent class.
  RegularWifiMac::SetAddress (address);
  RegularWifiMac::SetBssid (address);
}

void
ApWifiMac::SetPageSlicingActivated (bool activate)
{
	m_pageSlicingActivated = activate;
}

bool
ApWifiMac::GetPageSlicingActivated (void) const
{
	return m_pageSlicingActivated;
}

void
ApWifiMac::SetBeaconGeneration (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  if (!enable)
    {
      m_beaconEvent.Cancel ();
    }
  else if (enable && !m_enableBeaconGeneration)
    {
      m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
    }
  m_enableBeaconGeneration = enable;
}

bool
ApWifiMac::GetBeaconGeneration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_enableBeaconGeneration;
}

Time
ApWifiMac::GetBeaconInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_beaconInterval;
}
    
uint32_t
ApWifiMac::GetRawGroupInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rawGroupInterval;
}
  
uint32_t
ApWifiMac::GetTotalStaNum (void) const
{
  NS_LOG_FUNCTION (this);
  return m_totalStaNum;
}
    
uint32_t
ApWifiMac::GetSlotFormat (void) const
{
    return m_SlotFormat;
}
    
uint32_t
ApWifiMac::GetSlotCrossBoundary (void) const
{
    return  m_slotCrossBoundary;
}
    
uint32_t
ApWifiMac::GetSlotDurationCount (void) const
{
    return m_slotDurationCount;
}
    
uint32_t
ApWifiMac::GetSlotNum (void) const
{
    return m_slotNum;
}


uint8_t 
ApWifiMac::GetDTIMPeriod (void) const
{
    return m_DTIMPeriod;
}

void 
ApWifiMac::SetDTIMPeriod (uint8_t period)
{
    m_DTIMPeriod = period;
    //m_DTIMOffset = m_DTIMPeriod - 1;
}
    

void
ApWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_beaconDca->SetWifiRemoteStationManager (stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
}

void
ApWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this << &linkUp);
  RegularWifiMac::SetLinkUpCallback (linkUp);

  //The approach taken here is that, from the point of view of an AP,
  //the link is always up, so we immediately invoke the callback if
  //one is set
  linkUp ();
}

void
ApWifiMac::SetBeaconInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  if ((interval.GetMicroSeconds () % 1024) != 0)
    {
      NS_LOG_WARN ("beacon interval should be multiple of 1024us (802.11 time unit), see IEEE Std. 802.11-2012");
    }
  m_beaconInterval = interval;
  //NS_LOG_UNCOND("beacon interval = " << m_beaconInterval);
}

void
ApWifiMac::SetRawGroupInterval (uint32_t interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_rawGroupInterval = interval;
}
    
void
ApWifiMac::SetTotalStaNum (uint32_t num)
{
  NS_LOG_FUNCTION (this << num);
  m_totalStaNum = num;
  //m_S1gRawCtr.RAWGroupping (m_totalStaNum, 1, m_beaconInterval.GetMicroSeconds ());
  //m_S1gRawCtr.configureRAW ();
    
}
    
void
ApWifiMac::SetSlotFormat (uint32_t format)
{
    NS_ASSERT (format <= 1);
    m_SlotFormat = format;
}
    
void
ApWifiMac::SetSlotCrossBoundary (uint32_t cross)
{
    NS_ASSERT (cross <= 1);
    m_slotCrossBoundary = cross;
}
    
void
ApWifiMac::SetSlotDurationCount (uint32_t count)
{
    NS_ASSERT((!m_SlotFormat & (count < 256)) || (m_SlotFormat & (count < 2048)));
    m_slotDurationCount = count;
}
    
void
ApWifiMac::SetSlotNum (uint32_t count)
{
    NS_ASSERT((!m_SlotFormat & (count < 64)) || (m_SlotFormat & (count < 8)));
    m_slotNum = count;
}

void
ApWifiMac::SetChannelWidth (uint32_t width)
{
   m_channelWidth = width;
}

uint32_t
ApWifiMac::GetChannelWidth (void) const
{
    NS_LOG_UNCOND (GetAddress () << ", GetChannelWidth " << m_channelWidth );
   return m_channelWidth;
}

void
ApWifiMac::StartBeaconing (void)
{
  NS_LOG_FUNCTION (this);
  SendOneBeacon ();
}

int64_t
ApWifiMac::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_beaconJitter->SetStream (stream);
  return 1;
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

  //If we are a QoS AP then we attempt to get a TID for this packet
  if (m_qosSupported)
    {
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
    }

  ForwardDown (packet, from, to, tid);
}

void
ApWifiMac::ForwardDown (Ptr<const Packet> packet, Mac48Address from,
                        Mac48Address to, uint8_t tid)
{
  NS_LOG_FUNCTION (this << packet << from << to << static_cast<uint32_t> (tid));
  WifiMacHeader hdr;

  //For now, an AP that supports QoS does not support non-QoS
  //associations, and vice versa. In future the AP model should
  //support simultaneously associated QoS and non-QoS STAs, at which
  //point there will need to be per-association QoS state maintained
  //by the association state machine, and consulted here.
  if (m_qosSupported)
    {
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
      hdr.SetQosNoEosp ();
      hdr.SetQosNoAmsdu ();
      //Transmission of multiple frames in the same TXOP is not
      //supported for now
      hdr.SetQosTxopLimit (0);
      //Fill in the QoS control field in the MAC header
      hdr.SetQosTid (tid);
    }
  else
    {
      hdr.SetTypeData ();
    }

  if (m_htSupported)
    {
      hdr.SetNoOrder ();
    }
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (from);
  hdr.SetDsFrom ();
  hdr.SetDsNotTo ();

  int aid = 0;
  if (!to.IsBroadcast ())
  {
	  /*for (auto it=m_AidToMacAddr.begin(); it != m_AidToMacAddr.end(); ++it){
		  if (m_AidToMacAddr.find(aid)->second == to)
			  break;
		  else if (it == m_AidToMacAddr.end())
			  aid = -1;
	  }
	  if (aid == -1)
		  NS_LOG_INFO (Simulator::Now().GetMicroSeconds() << " ms: AP cannot forward down data because There is no RAW for this MAC address.");
	  */
	  do {aid++;}
	  while (m_AidToMacAddr.find(aid)->second != to); //TODO optimize search

	  NS_LOG_INFO (Simulator::Now().GetMicroSeconds() << " ms: AP to forward data for [aid=" << aid << "]");

	  uint8_t block = (aid >> 6 ) & 0x001f;
	  uint8_t page = (aid >> 11 ) & 0x0003;
	  NS_ASSERT (block >= m_pageslice.GetBlockOffset());
	  uint8_t toTim = 0;
	  	//= (block - m_pageslice.GetBlockOffset()) % m_pageslice.GetPageSliceLen(); //TODO make config alignment between TIM and RAW e.g. if AID belongs to TIM0 it cannot belong to RAW located in TIM3

	  	for (uint32_t i = 0; i < m_pageslice.GetPageSliceCount(); i++)
	  	{
	  		if (i == m_pageslice.GetPageSliceCount() - 1)
	  		{
	  			//last page slice
	  			if ( i * m_pageslice.GetPageSliceLen() <= block && block <= 31)
	  				toTim = i;
	  		}
	  		else
	  		{
	  			if (i * m_pageslice.GetPageSliceLen() <= block && block < (i + 1) * m_pageslice.GetPageSliceLen())
	  			{
	  				if (i == 0)
	  					continue;
	  				toTim++;
	  			}
	  		}
	  		//if (i * m_pageslice.GetPageSliceLen() <= block && block <= )
	  	}
	  //uint8_t toTim = (block - m_pageslice.GetBlockOffset()) % m_pageslice.GetPageSliceLen();
	  Time wait;
	  // station needs to receive DTIM beacon with indication there is downlink data first
	  if (toTim >= m_TIM.GetDTIMCount())
		 wait = (m_TIM.GetDTIMPeriod() + toTim - m_TIM.GetDTIMCount ()) * this->GetBeaconInterval();
	  else
		 wait = (m_TIM.GetDTIMPeriod() - m_TIM.GetDTIMCount () + toTim) * this->GetBeaconInterval();

	  // deduce the offset from the last beacon until now
	  wait -= Simulator::Now() - this->m_lastBeaconTime;
	  // downlink data needs to be scheduled in corresponding RAW slot for the station

	  wait += GetSlotStartTimeFromAid (aid) + MicroSeconds(5600);
	  NS_LOG_DEBUG ("At " << Simulator::Now().GetSeconds() << " s AP scheduling transmission for [aid=" << aid << "] " << wait.GetMicroSeconds() << " us from now, at " << Simulator::Now().GetSeconds() + wait.GetSeconds() << ".");
	  /*
	  void (ApWifiMac::*fp) (Ptr<const Packet>, Mac48Address, Mac48Address) = &ApWifiMac::ForwardDown;
	  Simulator::Schedule(wait, fp, this, packet, from, to);*/

	  Simulator::Schedule(wait, &EdcaTxopN::StartAccessIfNeeded, m_edca[QosUtilsMapTidToAc (tid)]);
  }

  if (m_qosSupported)
    {
      //Sanity check that the TID is valid
      NS_ASSERT (tid < 8);
      m_edca[QosUtilsMapTidToAc (tid)]->Queue (packet, hdr);
    }
  else
    {
      m_dca->Queue (packet, hdr);
    }

}

Time
ApWifiMac::GetSlotStartTimeFromAid (uint16_t aid) const
{
	uint8_t block = (aid >> 6 ) & 0x001f;
	NS_ASSERT (block >= m_pageslice.GetBlockOffset());
	uint8_t toTim = 0;
	//= (block - m_pageslice.GetBlockOffset()) % m_pageslice.GetPageSliceLen(); //TODO make config alignment between TIM and RAW e.g. if AID belongs to TIM0 it cannot belong to RAW located in TIM3

	for (uint32_t i = 0; i < m_pageslice.GetPageSliceCount(); i++)
	{
		if (i == m_pageslice.GetPageSliceCount() - 1)
		{
			//last page slice
			if ( i * m_pageslice.GetPageSliceLen() <= block && block <= 31)
				toTim = i;
		}
		else
		{
			if (i * m_pageslice.GetPageSliceLen() <= block && block < (i + 1) * m_pageslice.GetPageSliceLen())
			{
				if (i == 0)
					continue;
				toTim++;
			}
		}
		//if (i * m_pageslice.GetPageSliceLen() <= block && block <= )
	}

	//std::cout << "aid=" << (int)aid << ", toTim=" << (int)toTim << std::endl;
	uint16_t raw_len = (*m_rpsset.rpsset.at(toTim)).GetInformationFieldSize();

	uint16_t rawAssignment_len = 6;
	if (raw_len % rawAssignment_len !=0)
	{
		NS_ASSERT ("RAW configuration incorrect!");
	}
	uint8_t RAW_number = raw_len/rawAssignment_len;

    uint16_t slotDurationCount=0;
    uint16_t slotNum=0;
    uint64_t currentRAW_start=0;
    Time lastRawDurationus = MicroSeconds(0);
    int x = 0;
	for (uint8_t raw_index=0; raw_index < RAW_number; raw_index++)
	{
		RPS::RawAssignment ass = m_rpsset.rpsset.at(toTim)->GetRawAssigmentObj(raw_index);
		currentRAW_start += (500 + slotDurationCount * 120) * slotNum;
		slotDurationCount = ass.GetSlotDurationCount();
		slotNum = ass.GetSlotNum();
		Time slotDuration = MicroSeconds(500 + slotDurationCount * 120);
		lastRawDurationus += slotDuration * slotNum;
		if (ass.GetRawGroupAIDStart() <= aid && aid <= ass.GetRawGroupAIDEnd()) {
			uint16_t statRawSlot = (aid & 0x03ff) % slotNum;
			Time start = MicroSeconds((500 + slotDurationCount * 120) * statRawSlot + currentRAW_start);
			NS_LOG_DEBUG ("[aid=" << aid << "] is located in RAW " << (int)raw_index + 1 << " in slot " << statRawSlot + 1 << ". RAW slot start time relative to the beacon = " << start.GetMicroSeconds() << " us.");
			x = 1;
			return start;
		}
	}
	// AIDs that are not assigned to any RAW group can sleep through all the RAW groups
	// For station that does not belong to anz RAW group, return the time after all RAW groups
	/*currentRAW_start += (500 + slotDurationCount * 120) * slotNum;
	NS_LOG_DEBUG ("[aid=" << aid << "] is located outside all RAWs. It can start contending " << currentRAW_start << " us after the beacon.");*/
	NS_ASSERT (x);
	return MicroSeconds (currentRAW_start);
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to, Mac48Address from)
{
  NS_LOG_FUNCTION (this << packet << to << from);
  if (to.IsBroadcast () || m_stationManager->IsAssociated (to))
    {
	  ForwardDown (packet, from, to);
    }
}

void
ApWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << to);
  //We're sending this packet with a from address that is our own. We
  //get that address from the lower MAC and make use of the
  //from-spoofing Enqueue() method to avoid duplicated code.
  Enqueue (packet, to, m_low->GetAddress ());
}

bool
ApWifiMac::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

SupportedRates
ApWifiMac::GetSupportedRates (void) const
{
  NS_LOG_FUNCTION (this);
  SupportedRates rates;
  //If it is an HT-AP then add the BSSMembershipSelectorSet
  //which only includes 127 for HT now. The standard says that the BSSMembershipSelectorSet
  //must have its MSB set to 1 (must be treated as a Basic Rate)
  //Also the standard mentioned that at leat 1 element should be included in the SupportedRates the rest can be in the ExtendedSupportedRates
  if (m_htSupported)
    {
      for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          NS_LOG_UNCOND (GetAddress () << " GetSupportedRates ");
          rates.SetBasicRate (m_phy->GetBssMembershipSelector (i)); //not sure it's needed
        }
    }
  //Send the set of supported rates and make sure that we indicate
  //the Basic Rate set in this set of supported rates.
 // NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates  1 " ); //for test
  for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
    {
      WifiMode mode = m_phy->GetMode (i);
      rates.AddSupportedRate (mode.GetDataRate ());
      //Add rates that are part of the BSSBasicRateSet (manufacturer dependent!)
      //here we choose to add the mandatory rates to the BSSBasicRateSet,
      //exept for 802.11b where we assume that only the two lowest mandatory rates are part of the BSSBasicRateSet
      if (mode.IsMandatory ()
          && ((mode.GetModulationClass () != WIFI_MOD_CLASS_DSSS) || mode == WifiPhy::GetDsssRate1Mbps () || mode == WifiPhy::GetDsssRate2Mbps ()))
        {
          m_stationManager->AddBasicMode (mode);
        }
    }
  // NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates  2 " ); //for test
  //set the basic rates
  for (uint32_t j = 0; j < m_stationManager->GetNBasicModes (); j++)
    {
      WifiMode mode = m_stationManager->GetBasicMode (j);
      rates.SetBasicRate (mode.GetDataRate ());
    }
  //NS_LOG_LOGIC ("ApWifiMac::GetSupportedRates   " ); //for test
  return rates;
}

HtCapabilities
ApWifiMac::GetHtCapabilities (void) const
{
  HtCapabilities capabilities;
  capabilities.SetHtSupported (1);
  capabilities.SetLdpc (m_phy->GetLdpc ());
  capabilities.SetShortGuardInterval20 (m_phy->GetGuardInterval ());
  capabilities.SetGreenfield (m_phy->GetGreenfield ());
  for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
    {
      capabilities.SetRxMcsBitmask (m_phy->GetMcs (i));
    }
  return capabilities;
}

S1gCapabilities
ApWifiMac::GetS1gCapabilities (void) const
{
  S1gCapabilities capabilities;
  capabilities.SetS1gSupported (1);
  //capabilities.SetStaType (GetStaType ()); //do not need for AP
  capabilities.SetChannelWidth (GetChannelWidth ());

  return capabilities;
}

void
ApWifiMac::SendProbeResp (Mac48Address to)
{
  NS_LOG_FUNCTION (this << to);
  WifiMacHeader hdr;
  hdr.SetProbeResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeResponseHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
  probe.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
  if (m_htSupported)
    {
      probe.SetHtCapabilities (GetHtCapabilities ());
      hdr.SetNoOrder ();
    }
  packet->AddHeader (probe);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

void
ApWifiMac::SendAssocResp (Mac48Address to, bool success, uint8_t staType)
{
  NS_LOG_FUNCTION (this << to << success);
  WifiMacHeader hdr;
  hdr.SetAssocResp ();
  hdr.SetAddr1 (to);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocResponseHeader assoc;
  
  uint8_t mac[6];
  to.CopyTo (mac);
  uint8_t aid_l = mac[5];
  uint8_t aid_h = mac[4] & 0x1f;
  uint16_t aid = (aid_h << 8) | (aid_l << 0); //assign mac address as AID
  assoc.SetAID(aid); //
  m_AidToMacAddr[aid]=to;

  StatusCode code;
  if (success)
    {
      code.SetSuccess ();
    }
  else
    {
      code.SetFailure ();
    }
  assoc.SetSupportedRates (GetSupportedRates ());
  assoc.SetStatusCode (code);

  if (m_htSupported)
    {
      assoc.SetHtCapabilities (GetHtCapabilities ());
      hdr.SetNoOrder ();
    }
    //NS_LOG_UNCOND ("ApWifiMac::SendAssocResp =" );

   
  if (m_s1gSupported && success)
    {
      assoc.SetS1gCapabilities (GetS1gCapabilities ());
      //assign AID based on station type, to do.
       if (staType == 1)
        {
          for (std::vector<uint16_t>::iterator it = m_sensorList.begin(); it != m_sensorList.end(); it++)
            {
              if (*it == aid)
                 goto Addheader;
            }
          m_sensorList.push_back (aid);
          NS_LOG_INFO ("m_sensorList =" << m_sensorList.size ());
  
        }
       else if (staType == 2)
        {
           for (std::vector<uint16_t>::iterator it = m_OffloadList.begin(); it != m_OffloadList.end(); it++)
            {
                if (*it == aid)
                  goto Addheader;
            }
          m_OffloadList.push_back (aid);
          NS_LOG_INFO ("m_OffloadList =" << m_OffloadList.size ());
        }
    }
Addheader:
  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

//For now, to avoid adjust pageslicecount and pageslicecount dynamicly,   page bitmap is always 4 bytes
uint32_t
ApWifiMac::HasPacketsToPage (uint8_t blockstart , uint8_t Page)
{
	uint8_t blockBitmap;
	uint32_t PageBitmap;
	PageBitmap = 0;
	uint32_t numBlocks;
	if (m_pageslice.GetPageSliceCount() == 0)
		numBlocks = 31;
	else
		numBlocks = m_pageslice.GetPageSliceLen();
	//printf("		ApWifiMac::HasPacketsToPage --- Page Bitmap includes blocks from %d to %d\n", blockstart, blockstart + numBlocks - 1);
	for (uint32_t i=blockstart; i< blockstart + numBlocks ; i++ )
	{
		blockBitmap = HasPacketsToBlock (i,  Page);
		if (blockBitmap != 0)
		{
			PageBitmap = PageBitmap | (1 << i);
		}
		//printf("		ApWifiMac::HasPacketsToPage --- Block Bitmap = %x\n", blockBitmap);
	}
	//printf("		ApWifiMac::HasPacketsToPage --- Page Bitmap before >> blockstart = %x\n", PageBitmap);
	PageBitmap = PageBitmap >> blockstart;
	//printf("		ApWifiMac::HasPacketsToPage --- Page Bitmap after >> blockstart = %x\n", PageBitmap);
	return PageBitmap;
}

uint8_t
ApWifiMac::HasPacketsToBlock (uint16_t blockInd , uint16_t PageInd)
{
    uint16_t sta_aid, subblock, block;
    uint8_t blockBitmap;
    
    blockBitmap = 0;
    block = (PageInd << 11) | (blockInd << 6); // TODO check
   
    for (uint16_t i = 0; i <= 7; i++) //8 subblock in each block.
     {
       subblock = block | (i << 3);
       for (uint16_t j = 0; j <= 7; j++) //8 stations in each subblock
        {
           sta_aid = subblock | j;
           if (m_stationManager->IsAssociated (m_AidToMacAddr.find(sta_aid)->second) && HasPacketsInQueueTo(m_AidToMacAddr.find(sta_aid)->second) )
            {
        	   blockBitmap = blockBitmap | (1 << i);
        	   NS_LOG_DEBUG ("[aid=" << sta_aid << "] " << "paged");
        	   // if there is at least one station associated with AP that has FALSE for PageSlicingImplemented within this page then m_PageSliceNum = 31
        	   if (!m_supportPageSlicingList.at(m_AidToMacAddr[sta_aid]))
        		   m_PageSliceNum = 31;
        	   break;
            }
        }
     }
  
    return blockBitmap;
}

uint8_t
ApWifiMac::HasPacketsToSubBlock (uint16_t subblockInd, uint16_t blockInd , uint16_t PageInd)
{
    uint16_t sta_aid, subblock;
    uint8_t subblockBitmap;
    subblockBitmap = 0;
  
    subblock = (PageInd << 11) | (blockInd << 6) | (subblockInd << 3);
    for (uint16_t j = 0; j <= 7; j++) //8 stations in each subblock
        {
           sta_aid = subblock | j;
           if (m_stationManager->IsAssociated (m_AidToMacAddr.find(sta_aid)->second) && HasPacketsInQueueTo(m_AidToMacAddr.find(sta_aid)->second) )
             {
               subblockBitmap = subblockBitmap | (1 << j); 
               m_sleepList[m_AidToMacAddr.find(sta_aid)->second]=false;
             } 
        }
    return subblockBitmap;
}
    

bool 
ApWifiMac::HasPacketsInQueueTo(Mac48Address dest) 
{           
    //check also if ack received
    Ptr<const Packet> peekedPacket_VO, peekedPacket_VI, peekedPacket_BE, peekedPacket_BK;
    WifiMacHeader peekedHdr;
    Time tstamp;
        
    peekedPacket_VO = m_edca.find(AC_VO)->second->GetEdcaQueue()->PeekByAddress (WifiMacHeader::ADDR1, dest);
    peekedPacket_VI = m_edca.find(AC_VI)->second->GetEdcaQueue()->PeekByAddress (WifiMacHeader::ADDR1, dest);
    peekedPacket_BE = m_edca.find(AC_BE)->second->GetEdcaQueue()->PeekByAddress (WifiMacHeader::ADDR1, dest);
    peekedPacket_BK = m_edca.find(AC_BK)->second->GetEdcaQueue()->PeekByAddress (WifiMacHeader::ADDR1, dest);
        
    if (peekedPacket_VO != 0 || peekedPacket_VI != 0 || peekedPacket_BE != 0 || peekedPacket_BK != 0 )
       {
         return true;
       }
    else
      {
         return false;
      }
}
 
uint16_t ApWifiMac::RpsIndex = 0;
void
ApWifiMac::SetaccessList (std::map<Mac48Address, bool> list)
{
        Mac48Address stasAddr;

            
        if (list.size () == 0)
         {
            return;
         }
              
        for (uint32_t k = 1; k <= m_totalStaNum; k++)
          {   
            stasAddr = m_AidToMacAddr.find(k)->second;
            //NS_LOG_UNCOND ( "aid "  << k << ", send " << list.find(stasAddr)->second << ", at " << Simulator::Now () << ", size " << list.size ());
          }     
               
        m_edca.find (AC_VO)->second->SetaccessList (list);
        m_edca.find (AC_VI)->second->SetaccessList (list);
        m_edca.find (AC_BE)->second->SetaccessList (list);
        m_edca.find (AC_BK)->second->SetaccessList (list);
}

  
void
ApWifiMac::SendOneBeacon (void)
{  
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
    
  m_lastBeaconTime = Simulator::Now();

    if (m_s1gSupported)
     {
      hdr.SetS1gBeacon ();
      hdr.SetAddr1 (Mac48Address::GetBroadcast ());
      hdr.SetAddr2 (GetAddress ()); // for debug, not accordance with draft, need change
      hdr.SetAddr3 (GetAddress ()); // for debug
      Ptr<Packet> packet = Create<Packet> ();
      S1gBeaconHeader beacon;
      S1gBeaconCompatibility compatibility;
      compatibility.SetBeaconInterval (m_beaconInterval.GetMicroSeconds ());
      beacon.SetBeaconCompatibility (compatibility);
     
      RPS *m_rps;
      if (RpsIndex < m_rpsset.rpsset.size())
         {
            m_rps = m_rpsset.rpsset.at(RpsIndex);
            NS_LOG_INFO ("< RpsIndex =" << RpsIndex);
            RpsIndex++;
          }
      else
         {
            m_rps = m_rpsset.rpsset.at(0);
            NS_LOG_INFO ("RpsIndex =" << RpsIndex);
            RpsIndex = 1;
          }
      beacon.SetRPS (*m_rps);

    Mac48Address stasleepAddr;
    for (auto i=m_AidToMacAddr.begin(); i != m_AidToMacAddr.end() ; ++i)
    {
    	// assume all station sleep, then change some to awake state based on downlink data
    	//This implementation is temporary, should be removed if ps-poll is supported

    	stasleepAddr = i->second;
    	if (m_stationManager->IsAssociated (stasleepAddr))
    	{
    		m_sleepList[stasleepAddr]=true;
    	}
    }

    if (m_DTIMCount == 0 && GetPageSlicingActivated ()) // TODO filter when GetPageSlicingActivated() is false
      {
    	NS_LOG_DEBUG ("***DTIM*** starts at " << Simulator::Now().GetSeconds() << " s");
        m_pagebitmap = HasPacketsToPage (m_pageslice.GetBlockOffset (), m_pageslice.GetPageindex()); //TODO check set m_PageSliceNum = 31
        if (m_pagebitmap)//for now, only configure Page Bit map based on real-time traffic, other parameters configured beforehand.
        	NS_LOG_DEBUG("	Page bitmap (0-4 bytes): " << m_pagebitmap);
        m_pageslice.SetPageBitmap (m_pagebitmap);
        NS_LOG_DEBUG("	Page bitmap is " << (int)m_pageslice.GetPageBitmapLength() << " bytes long.");
        //For now, page bitmap is always 4 bytes
        beacon.SetpageSlice (m_pageslice);
      }
    else if (m_DTIMCount != 0 && GetPageSlicingActivated ())
    {
    	NS_LOG_DEBUG ("***TIM" << (int)m_DTIMCount << "*** starts at " << Simulator::Now().GetSeconds() << " s");
    }
    
      /*
      RPS m_rps;
      NS_LOG_UNCOND ("send beacon at" << Simulator::Now ());
      m_S1gRawCtr.deleteRps ();
      m_rps = m_S1gRawCtr.UpdateRAWGroupping (m_sensorList, m_OffloadList, m_receivedAid, m_beaconInterval.GetMicroSeconds (), m_outputpath);
      m_receivedAid.clear (); //release storage
      //m_rps = m_S1gRawCtr.GetRPS ();
      beacon.SetRPS (m_rps); */
      

    m_DTIMPeriod = m_TIM.GetDTIMPeriod ();
    m_TIM.SetDTIMCount (m_DTIMCount);
    NS_ASSERT (m_pageslice.GetTIMOffset () +  m_pageslice.GetPageSliceCount() <= m_DTIMPeriod);
    m_TrafficIndicator = 0; //for group addressed MSDU/MMPDU, not supported.
    m_TIM.SetTafficIndicator (m_TrafficIndicator); //from page slice
    m_PageSliceNum = 0;
    if (m_pageslice.GetPageSliceCount() == 0)
      {
       m_PageSliceNum = 31;
      }
    else
    if (m_PageSliceNum != 31 && m_pageslice.GetPageSliceCount() != 0)
      {
		if (m_DTIMCount == m_pageslice.GetTIMOffset ()) //first page slice start at TIM offset
		  {
			 m_PageSliceNum = 0;
		  }
		else
		  {
			m_PageSliceNum++;
		  }
	    NS_ASSERT(m_PageSliceNum < m_pageslice.GetPageSliceCount()); //or do not use m_PageSliceNum when it's larger (equal) than slice count.
      }

    
    m_PageIndex = m_pageslice.GetPageindex();
    //m_TIM.SetPageIndex (m_PageIndex);
    uint64_t numPagedStas (0);
    for (auto it = m_supportPageSlicingList.begin(); it != m_supportPageSlicingList.end(); ++it){
    	if (m_stationManager->IsAssociated (it->first) && HasPacketsInQueueTo(it->first) )
    	{
    		numPagedStas++;
    	}
    }
    //if (!m_DTIMCount && numPagedStas) NS_LOG_DEBUG ("Paged stations: " << (int)numPagedStas);
	/*if (m_pageslice.GetPageSliceCount() == 0 && numPagedStas > 0)// special case
	{
		if (m_pageslice.GetPageSliceLen() > 1)
		{
			// 32nd TIM in this DTIM can contain DL information for (1) STAs that do not support page slicing or (2)STAs that support page slicing and
			// whose AID is within 32nd block of this page
			m_PageSliceNum = 31;
			// TODO
		}
		else if (m_pageslice.GetPageSliceLen() == 1)
		{
			m_PageSliceNum = 31;
			// Standard 10.47 page 325-326
		}
	}
  else if (m_pageslice.GetPageSliceCount () == 0 && numPagedStas == 0)
   {
      m_PageSliceNum = 0;
   }*/

    uint8_t NumEncodedBlock;
    if (m_PageSliceNum != (m_pageslice.GetPageSliceCount() - 1) && m_PageSliceNum != 31) // convenient overflow if count==0
      {
    		NumEncodedBlock = m_pageslice.GetPageSliceLen();
      }
    else if (m_pageslice.GetPageBitmapLength() > 0)//Last Page slice has max value of 31 (5 bits)
      {
    	// PSlast = 8 * PageBitmap_length - (PScount-1) * PSlength
    	if (m_pageslice.GetPageSliceCount() > 0)
    		NumEncodedBlock = 0x1f & (8 * 4 - (m_pageslice.GetPageSliceCount() - 1) * m_pageslice.GetPageSliceLen());
    	else if (m_pageslice.GetPageSliceCount() == 0)
    		NumEncodedBlock = 31;
         //As page bitmap of page slice element is fixed to 4 bytes for now, "m_pageslice.GetInformationFieldSize ()" is alwawys 8.
         //Section 9.4.2.193 oage slice element, Draft 802.11ah_D9.0
        NS_LOG_DEBUG ("Last page slice has " << (int)NumEncodedBlock << " blocks.");
      }
    m_TIM.SetPageSliceNum (m_PageSliceNum); //from page slice

   if (m_PageSliceNum == 0 || m_PageSliceNum == 31)
      {
        m_blockoffset = m_pageslice.GetBlockOffset ();
      }

   m_TIM.m_length = 0; // every beacon can have up to NumEncodedBlock encoded blocks
    if (m_pageslice.GetPageBitmapLength()){
    	//uint8_t numBlocksToEncode = m_pageslice.GetPageBitmapLength();

    for (uint8_t i = 0; i< NumEncodedBlock; i++)
      {
        TIM::EncodedBlock * m_encodedBlock = new TIM::EncodedBlock;
        m_encodedBlock->SetBlockOffset (m_blockoffset & 0x1f); //TODO check
        uint8_t m_blockbitmap = HasPacketsToBlock (m_blockoffset & 0x1f, m_PageIndex);
        m_encodedBlock->SetBlockBitmap (m_blockbitmap);

        uint8_t subblocklength = 0;
        uint8_t *  m_subblock; 
        m_subblock = new uint8_t [8]; //can be released after SetPartialVBitmap
        for (uint16_t j = 0; j <= 7; j++ ) // at most 8 subblock
          {
             if (m_blockbitmap & (1 << j))
             {
                *m_subblock = HasPacketsToSubBlock (j, m_blockoffset & 0x1f, m_PageIndex);
                subblocklength++; 
                m_subblock++;
             }
          }
        for (uint32_t j = 0; j < subblocklength; j++)
        	m_subblock--;
        m_encodedBlock->SetEncodedInfo(m_subblock, subblocklength);
             
        m_blockoffset++; //actually block id
        NS_ASSERT (m_blockoffset <= m_pageslice.GetBlockOffset () + m_pageslice.GetInformationFieldSize () * 8);
        //block id cannot exceeds the max defined in the page slice  element
        m_TIM.SetPartialVBitmap (*m_encodedBlock);
        if (m_encodedBlock)
        	delete m_encodedBlock;
      }

    }

    beacon.SetTIM (m_TIM);
   /* if (m_DTIMOffset == m_DTIMPeriod - 1)
      {
        m_DTIMOffset = 0;
      }
    else
      {
         m_DTIMOffset++;
      }
    */
    if (m_DTIMCount == m_DTIMPeriod - 1)
      {
        m_DTIMCount = 0;
      }
    else
      {
        m_DTIMCount++;
      }
    //NS_ASSERT (m_DTIMPeriod - m_DTIMCount + m_DTIMOffset == m_DTIMPeriod || (m_DTIMCount == 0 && m_DTIMOffset == 0));
    
    //set sleep list, temporary, removed if ps-poll supported 
    m_edca.find (AC_VO)->second->SetsleepList (m_sleepList);
    m_edca.find (AC_VI)->second->SetsleepList (m_sleepList);
    m_edca.find (AC_BE)->second->SetsleepList (m_sleepList);
    m_edca.find (AC_BK)->second->SetsleepList (m_sleepList);
    
    
   
      
      
      AuthenticationCtrl  AuthenCtrl;
      AuthenCtrl.SetControlType (false); //centralized
      Ptr<WifiMacQueue> MgtQueue = m_dca->GetQueue ();
      uint32_t MgtQueueSize= MgtQueue->GetSize ();
      if  (MgtQueueSize < 10 )
        {
          if (AuthenThreshold <= 950)
           {
             AuthenThreshold += 50;
           }
        }
      else
        {
          if (AuthenThreshold > 50)
           {
               AuthenThreshold -=50;
           }
        }
      AuthenCtrl.SetThreshold (AuthenThreshold); //centralized
      beacon.SetAuthCtrl (AuthenCtrl);
      packet->AddHeader (beacon);
      m_beaconDca->Queue (packet, hdr);

      m_transmitBeaconTrace(beacon, m_rps->GetRawAssigmentObj());

      MacLowTransmissionParameters params;
      params.DisableRts();
      params.DisableAck();
      params.DisableNextData();
      Time txTime = m_low->CalculateOverallTxTime(packet, &hdr, params);
      NS_LOG_DEBUG(
    		  "Transmission of beacon will take " << txTime << ", delaying RAW start for that amount");
      Time bufferTimeToAllowBeaconToBeReceived = txTime;
      //bufferTimeToAllowBeaconToBeReceived = MicroSeconds (5600);
      auto nRaw = m_rps->GetNumberOfRawGroups();
      currentRawGroup = (currentRawGroup + 1) % nRaw;

      uint16_t startaid;
      uint16_t endaid;
      Mac48Address stasAddr;
      //uint16_t offset;
      uint16_t statsPerSlot;
      uint16_t statRawSlot;

      //NS_LOG_UNCOND ("ap send beacon at " << Simulator::Now ());

      m_accessList.clear ();
      for (uint16_t i=1; i<= m_totalStaNum;i++)
      {
    	  // assume all station sleeps, then change some to awake state based on downlink data
    	  //This implementation is temporary, should be removed if ps-poll is supported
    	  if (m_AidToMacAddr.size () == 0)
    	  {
    		  break;
    	  }
    	  stasAddr = m_AidToMacAddr.find(i)->second;

    	  if (m_stationManager->IsAssociated (stasAddr))
    	  {
    		  m_accessList[stasAddr]=false;
    	  }
     }
      //NS_LOG_UNCOND ("m_accessList.size " << m_accessList.size ());


      // schedule the slot start
      Time timeToSlotStart = Time ();
      for (uint32_t g = 0; g < nRaw; g++)
      {
    	  if (m_AidToMacAddr.size () == 0)
    	  {
    		  break;
    	  }


    	  startaid = m_rps->GetRawAssigmentObj(g).GetRawGroupAIDStart();
    	  endaid = m_rps->GetRawAssigmentObj(g).GetRawGroupAIDEnd();



    	  //offset =0; // for test
    	  //m_slotNum=m_rps->GetRawAssigmentObj(g).GetSlotNum();
    	  statsPerSlot = (endaid - startaid + 1)/m_rps->GetRawAssigmentObj(g).GetSlotNum();

    	  for (uint32_t i = 0; i < m_rps->GetRawAssigmentObj(g).GetSlotNum(); i++)
    	  {
    		  for (uint32_t k = startaid; k <= endaid; k++)
    		  {

    			  statRawSlot = (k & 0x03ff) % m_rps->GetRawAssigmentObj(g).GetSlotNum(); //slot that the station k will be
    			  // station is in sot i
    			  if (statRawSlot == i )
    			  {
    				  stasAddr = m_AidToMacAddr.find(k)->second;
    				  if (m_stationManager->IsAssociated (stasAddr))
    				  {
    					  m_accessList[stasAddr]=true;
    				  }
    			  }

    		  }
    		  Simulator::Schedule(bufferTimeToAllowBeaconToBeReceived + timeToSlotStart,
    				  &ApWifiMac::SetaccessList, this, m_accessList);

    		  Simulator::Schedule(
    				  bufferTimeToAllowBeaconToBeReceived + timeToSlotStart,
    				  &ApWifiMac::OnRAWSlotStart, this, RpsIndex, g + 1, i + 1);
    		  timeToSlotStart += MicroSeconds(500 + m_rps->GetRawAssigmentObj(g).GetSlotDurationCount() * 120);

    		  for (uint16_t i=1; i<= m_totalStaNum;i++)
    		  {
    			  stasAddr = m_AidToMacAddr.find(i)->second;

    			  if (m_stationManager->IsAssociated (stasAddr))
    			  {
    				  m_accessList[stasAddr]=false;
    			  }
    		  }
    	  }
      }
      //NS_LOG_UNCOND(GetAddress () << ", " << startaid << "\t" << endaid << ", at " << Simulator::Now () << ", bufferTimeToAllowBeaconToBeReceived " << bufferTimeToAllowBeaconToBeReceived);
     }
    else
     {
      m_receivedAid.clear (); //release storage
      hdr.SetBeacon ();
      hdr.SetAddr1 (Mac48Address::GetBroadcast ());
      hdr.SetAddr2 (GetAddress ());
      hdr.SetAddr3 (GetAddress ());
      hdr.SetDsNotFrom ();
      hdr.SetDsNotTo ();
      Ptr<Packet> packet = Create<Packet> ();
      MgtBeaconHeader beacon;
      beacon.SetSsid (GetSsid ());
      beacon.SetSupportedRates (GetSupportedRates ());
      beacon.SetBeaconIntervalUs (m_beaconInterval.GetMicroSeconds ());
      if (m_htSupported)
        {
          beacon.SetHtCapabilities (GetHtCapabilities ());
          hdr.SetNoOrder ();
        }
      packet->AddHeader (beacon);
         //The beacon has it's own special queue, so we load it in there
      m_beaconDca->Queue (packet, hdr);
      }
  m_beaconEvent = Simulator::Schedule (m_beaconInterval, &ApWifiMac::SendOneBeacon, this);
}

void ApWifiMac::OnRAWSlotStart(uint16_t rps, uint8_t rawGroup, uint8_t slot)
{
	LOG_TRAFFIC("AP RAW SLOT START FOR RAW GROUP " << (int)rawGroup << " SLOT " << (int)slot);
	m_rpsIndexTrace = rps;
	m_rawGroupTrace = rawGroup;
	m_rawSlotTrace = slot;

        //NS_LOG_UNCOND("AP RAW SLOT START FOR RAW GROUP " << std::to_string(rawGroup) << " SLOT " << std::to_string(slot));
}



void
ApWifiMac::TxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxOk (hdr);

  if (hdr.IsAssocResp ()
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("associated with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxOk (hdr.GetAddr1 ());
    }
}

void
ApWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxFailed (hdr);

  if (hdr.IsAssocResp ()
      && m_stationManager->IsWaitAssocTxOk (hdr.GetAddr1 ()))
    {
      NS_LOG_DEBUG ("assoc failed with sta=" << hdr.GetAddr1 ());
      m_stationManager->RecordGotAssocTxFailed (hdr.GetAddr1 ());
    }
}

void
ApWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  //uint16_t segg =  hdr->GetFrameControl (); // for test
  //NS_LOG_UNCOND ("AP waiting   " << segg); //for test
  Mac48Address from = hdr->GetAddr2 ();

  if (hdr->IsData ())
    {
      Mac48Address bssid = hdr->GetAddr1 ();
      if (!hdr->IsFromDs ()
          && hdr->IsToDs ()
          && bssid == GetAddress ()
          && m_stationManager->IsAssociated (from))
        {
          Mac48Address to = hdr->GetAddr3 ();
          if (to == GetAddress ())
            {
              NS_LOG_DEBUG ("frame for me from=" << from);
              if (hdr->IsQosData ())
                {
                  if (hdr->IsQosAmsdu ())
                    {
                      NS_LOG_DEBUG ("Received A-MSDU from=" << from << ", size=" << packet->GetSize ());
                      DeaggregateAmsduAndForward (packet, hdr);
                      packet = 0;
                    }
                  else
                    {
                      ForwardUp (packet, from, bssid);
                    }
                }
              else
                {
                  ForwardUp (packet, from, bssid);
                }
                uint8_t mac[6];
                from.CopyTo (mac);
                uint8_t aid_l = mac[5];
                uint8_t aid_h = mac[4] & 0x1f;
                uint16_t aid = (aid_h << 8) | (aid_l << 0); //assign mac address as AID
                m_receivedAid.push_back(aid); //to change
            }
          else if (to.IsGroup ()
                   || m_stationManager->IsAssociated (to))
            {
              NS_LOG_DEBUG ("forwarding frame from=" << from << ", to=" << to);
              Ptr<Packet> copy = packet->Copy ();

              //If the frame we are forwarding is of type QoS Data,
              //then we need to preserve the UP in the QoS control
              //header...
              if (hdr->IsQosData ())
                {
                  ForwardDown (packet, from, to, hdr->GetQosTid ());
                }
              else
                {
                  ForwardDown (packet, from, to);
                }
              ForwardUp (copy, from, to);
            }
          else
            {
              ForwardUp (packet, from, to);
            }
        }
      else if (hdr->IsFromDs ()
               && hdr->IsToDs ())
        {
          //this is an AP-to-AP frame
          //we ignore for now.
          NotifyRxDrop (packet, DropReason::MacAPToAPFrame);
        }
      else
        {
          //we can ignore these frames since
          //they are not targeted at the AP
          NotifyRxDrop (packet, DropReason::MacNotForAP);
          NS_LOG_UNCOND ("not assciate, drop, from=" << from );
        }
      return;
    }
  else if (hdr->IsMgt ())
    {
      if (hdr->IsProbeReq ())
        {
          NS_ASSERT (hdr->GetAddr1 ().IsBroadcast ());
          SendProbeResp (from);
          return;
        }
      else if (hdr->GetAddr1 () == GetAddress ())
        {
          if (hdr->IsAssocReq ())
            {
              if (m_stationManager->IsAssociated (from))
                {
                  return;  //test, avoid repeate assoc
                 }
               //NS_LOG_LOGIC ("Received AssocReq "); // for test
              //first, verify that the the station's supported
              //rate set is compatible with our Basic Rate set
              MgtAssocRequestHeader assocReq;

              packet->RemoveHeader (assocReq);

              SupportedRates rates = assocReq.GetSupportedRates ();
              bool problem = false;
              for (uint32_t i = 0; i < m_stationManager->GetNBasicModes (); i++)
                {
                  WifiMode mode = m_stationManager->GetBasicMode (i);
                  if (!rates.IsSupportedRate (mode.GetDataRate ()))
                    {
                      problem = true;
                      break;
                    }
                }

              if (m_htSupported)
                {
                  //check that the STA supports all MCSs in Basic MCS Set
                  HtCapabilities htcapabilities = assocReq.GetHtCapabilities ();
                  for (uint32_t i = 0; i < m_stationManager->GetNBasicMcs (); i++)
                    {
                      uint8_t mcs = m_stationManager->GetBasicMcs (i);
                      if (!htcapabilities.IsSupportedMcs (mcs))
                        {
                          problem = true;
                          break;
                        }
                    }

                }

                
              if (problem)
                {
                  //One of the Basic Rate set mode is not
                  //supported by the station. So, we return an assoc
                  //response with an error status.
                  SendAssocResp (hdr->GetAddr2 (), false, 0);
                }
              else
                {
                  //station supports all rates in Basic Rate Set.
                  //record all its supported modes in its associated WifiRemoteStation
                  for (uint32_t j = 0; j < m_phy->GetNModes (); j++)
                    {
                      WifiMode mode = m_phy->GetMode (j);
                      if (rates.IsSupportedRate (mode.GetDataRate ()))
                        {
                          m_stationManager->AddSupportedMode (from, mode);
                        }
                    }
                  if (m_htSupported)
                    {
                      HtCapabilities htcapabilities = assocReq.GetHtCapabilities ();
                      m_stationManager->AddStationHtCapabilities (from,htcapabilities);
                      for (uint32_t j = 0; j < m_phy->GetNMcs (); j++)
                        {
                          uint8_t mcs = m_phy->GetMcs (j);
                          if (htcapabilities.IsSupportedMcs (mcs))
                            {
                              m_stationManager->AddSupportedMcs (from, mcs);
                            }
                        }
                    }
                    
                  m_stationManager->RecordWaitAssocTxOk (from);
                  
                  if (m_s1gSupported)
                    {
                      S1gCapabilities s1gcapabilities = assocReq.GetS1gCapabilities ();
                      m_stationManager->AddStationS1gCapabilities (from,s1gcapabilities);
                      uint8_t sta_type = s1gcapabilities.GetStaType ();
                      bool pageSlicingSupported = s1gcapabilities.GetPageSlicingSupport() != 0;
                      m_supportPageSlicingList[hdr->GetAddr2 ()] = pageSlicingSupported;
                      SendAssocResp (hdr->GetAddr2 (), true, sta_type);
                    }
                  else
                    {
                      //send assoc response with success status.
                      SendAssocResp (hdr->GetAddr2 (), true, 0);
                    }

                }
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              m_stationManager->RecordDisassociated (from);
              uint8_t mac[6];
              from.CopyTo (mac);
              uint8_t aid_l = mac[5];
              uint8_t aid_h = mac[4] & 0x1f;
              uint16_t aid = (aid_h << 8) | (aid_l << 0);
              NS_LOG_UNCOND ("Disassociation request from aid " << aid);

             for (std::vector<uint16_t>::iterator it = m_sensorList.begin(); it != m_sensorList.end(); it++)
                {
                    if (*it == aid)
                    {   m_sensorList.erase (it); //remove from association list
                        NS_LOG_UNCOND ("erase aid " << aid << " by Ap from m_sensorList ");
                        break;
                    }
                }
              return;
            }
        }
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

void
ApWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket,
                                       const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets =
    MsduAggregator::Deaggregate (aggregatedPacket);

  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      if ((*i).second.GetDestinationAddr () == GetAddress ())
        {
          ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                     (*i).second.GetDestinationAddr ());
        }
      else
        {
          Mac48Address from = (*i).second.GetSourceAddr ();
          Mac48Address to = (*i).second.GetDestinationAddr ();
          NS_LOG_DEBUG ("forwarding QoS frame from=" << from << ", to=" << to);
          ForwardDown ((*i).first, from, to, hdr->GetQosTid ());
        }
    }
}

void
ApWifiMac::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  m_beaconDca->Initialize ();
  m_beaconEvent.Cancel ();
  if (m_enableBeaconGeneration)
    {
      if (m_enableBeaconJitter)
        {
          int64_t jitter = m_beaconJitter->GetValue (0, m_beaconInterval.GetMicroSeconds ());
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time " << jitter << " microseconds");
          m_beaconEvent = Simulator::Schedule (MicroSeconds (jitter), &ApWifiMac::SendOneBeacon, this);
        }
      else
        {
          NS_LOG_DEBUG ("Scheduling initial beacon for access point " << GetAddress () << " at time 0");
          m_beaconEvent = Simulator::ScheduleNow (&ApWifiMac::SendOneBeacon, this);
        }
    }
  m_edca.find(AC_VO)->second->GetEdcaQueue()->TraceConnect("PacketDropped", "", MakeCallback(&ApWifiMac::OnQueuePacketDropped, this));
  m_edca.find(AC_VI)->second->GetEdcaQueue()->TraceConnect("PacketDropped", "", MakeCallback(&ApWifiMac::OnQueuePacketDropped, this));
  m_edca.find(AC_BE)->second->GetEdcaQueue()->TraceConnect("PacketDropped", "", MakeCallback(&ApWifiMac::OnQueuePacketDropped, this));
  m_edca.find(AC_BK)->second->GetEdcaQueue()->TraceConnect("PacketDropped", "", MakeCallback(&ApWifiMac::OnQueuePacketDropped, this));

  RegularWifiMac::DoInitialize ();
}

} //namespace ns3
