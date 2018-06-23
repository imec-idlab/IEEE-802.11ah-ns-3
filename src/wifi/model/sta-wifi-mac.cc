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

#include "sta-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "qos-tag.h"
#include "mac-low.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "wifi-mac-header.h"
#include "extension-headers.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "mgt-headers.h"
#include "ht-capabilities.h"

#include "random-stream.h"

#define LOG_SLEEP(msg)	if(true) NS_LOG_DEBUG("[" << (GetAID (0)) << "] " << msg << std::endl);


/*
 * The state machine for this STA is:
 --------------                                          -----------
 | Associated |   <--------------------      ------->    | Refused |
 --------------                        \    /            -----------
    \                                   \  /
     \    -----------------     -----------------------------
      \-> | Beacon Missed | --> | Wait Association Response |
          -----------------     -----------------------------
                \                       ^
                 \                      |
                  \    -----------------------
                   \-> | Wait Probe Response |
                       -----------------------
 */

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StaWifiMac");

NS_OBJECT_ENSURE_REGISTERED (StaWifiMac);
//uint32_t al = 68, ah= 68;
std::vector<uint32_t> trackit {};
TypeId
StaWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StaWifiMac")
    .SetParent<RegularWifiMac> ()
    .SetGroupName ("Wifi")
    .AddConstructor<StaWifiMac> ()
    .AddAttribute ("ProbeRequestTimeout", "The interval between two consecutive probe request attempts.",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&StaWifiMac::m_probeRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("AssocRequestTimeout", "The interval between two consecutive assoc request attempts.",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&StaWifiMac::m_assocRequestTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("RawDuration", "The duration of one RAW group.",
                   TimeValue (MicroSeconds (102400)),
                   MakeTimeAccessor (&StaWifiMac::GetRawDuration,
                                     &StaWifiMac::SetRawDuration),
                   MakeTimeChecker ())
	.AddAttribute ("MaxTimeInQueue", "The max. time a packet stays in the DCA queue before it's dropped",
					TimeValue (MilliSeconds(10000)),
					MakeTimeAccessor(&StaWifiMac::m_maxTimeInQueue),
					MakeTimeChecker ())
    .AddAttribute ("MaxMissedBeacons",
                   "Number of beacons which much be consecutively missed before "
                   "we attempt to restart association.",
                   UintegerValue (10),
                   MakeUintegerAccessor (&StaWifiMac::m_maxMissedBeacons),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("StaType",
                   "Type of station, '1' sensor, '2' offload station ",
                   UintegerValue (1),
                   MakeUintegerAccessor (&StaWifiMac::GetStaType,
                                         &StaWifiMac::SetStaType),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ChannelWidth",
                   "Channel width of the stations ",
                   UintegerValue (0),
                   MakeUintegerAccessor (&StaWifiMac::GetChannelWidth,
                                         &StaWifiMac::SetChannelWidth),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ActiveProbing",
                   "If true, we send probe requests. If false, we don't."
                   "NOTE: if more than one STA in your simulation is using active probing, "
                   "you should enable it at a different simulation time for each STA, "
                   "otherwise all the STAs will start sending probes at the same time resulting in collisions. "
                   "See bug 1060 for more info.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&StaWifiMac::SetActiveProbing, &StaWifiMac::GetActiveProbing),
                   MakeBooleanChecker ())
    .AddTraceSource ("Assoc", "Associated with an access point.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_assocLogger),
                     "ns3::Mac48Address::TracedCallback")
    .AddTraceSource ("DeAssoc", "Association with an access point lost.",
                     MakeTraceSourceAccessor (&StaWifiMac::m_deAssocLogger),
                     "ns3::Mac48Address::TracedCallback")
	.AddTraceSource ("S1gBeaconMissed", "Fired when a beacon is missed.",
					MakeTraceSourceAccessor (&StaWifiMac::m_beaconMissed),
					"ns3::StaWifiMac::S1gBeaconMissedCallback")

	.AddTraceSource ("NrOfTransmissionsDuringRAWSlot",
					"Nr of transmissions during RAW slot",
					MakeTraceSourceAccessor (&StaWifiMac::nrOfTransmissionsDuringRAWSlot),
					"ns3::TracedValueCallback::Uint16")
  ;
  return tid;
}

StaWifiMac::StaWifiMac ()
  : m_state (BEACON_MISSED),
    m_probeRequestEvent (),
    m_assocRequestEvent (),
    m_disassocRequestEvent (),
    m_beaconWatchdogEnd (Seconds (0.0))
{
  NS_LOG_FUNCTION (this);
  m_rawStart = false;
  m_dataBuffered = false;
  //m_aids.assign(1, 8192);
  //m_aid = 8192;
  uint32_t cwmin = 15;
  uint32_t cwmax = 1023;
  m_pspollDca = CreateObject<DcaTxop> ();
  m_pspollDca->SetAifsn (2);
  m_pspollDca->SetMinCw ((cwmin + 1) / 4 - 1);
  m_pspollDca->SetMaxCw ((cwmin + 1) / 2 - 1);  //same as AC_VO
  m_pspollDca->SetLow (m_low);
  m_pspollDca->SetManager (m_dcfManager);
  m_pspollDca->SetTxMiddle (m_txMiddle);
  fasTAssocType = false; //centraied control
  fastAssocThreshold = 0; // allow some station to associate at the begining
    Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
  assocVaule = m_rv->GetValue (0, 999);
  
  firstBeacon = true;
  receivingBeacon = false;
  timeDifferenceBeacon = 0;
  timeBeacon = 0;
  outsideraw = false;
  stationrawslot = false;
  waitingack = false;

  //if (m_qosSupported)

      //m_edca->SetTxOkCallback(MakeCallback(StaWifiMac::SleepIfQueueIsEmpty), this);
        m_edca.find (AC_VO)->second->SetsleepCallback (MakeCallback (&StaWifiMac::SleepIfQueueIsEmpty,this));
        m_edca.find (AC_VI)->second->SetsleepCallback (MakeCallback (&StaWifiMac::SleepIfQueueIsEmpty,this));
        m_edca.find (AC_BE)->second->SetsleepCallback (MakeCallback (&StaWifiMac::SleepIfQueueIsEmpty,this));
        m_edca.find (AC_BK)->second->SetsleepCallback (MakeCallback (&StaWifiMac::SleepIfQueueIsEmpty,this));
  

  //else
  //    m_dca->SetTxOkCallback(MakeCallback(StaWifiMac::SleepIfQueueIsEmpty), this);

  //Let the lower layers know that we are acting as a non-AP STA in
  //an infrastructure BSS.
  SetTypeOfStation (STA);
  m_pagedInDtim = false;
}

StaWifiMac::~StaWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
StaWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_pspollDca = 0;
  RegularWifiMac::DoDispose ();
}

uint32_t
StaWifiMac::GetStaType (void) const
{
   return m_staType;
}

void
StaWifiMac::SetStaType (uint32_t statype)
{
   m_staType = statype;
}

void
StaWifiMac::SetChannelWidth (uint32_t width)
{
   m_channelWidth = width;
}

uint32_t
StaWifiMac::GetChannelWidth (void) const
{
   //NS_LOG_UNCOND (GetAddress () << ", GetChannelWidth " << m_channelWidth );
   return m_channelWidth;
}

uint32_t 
StaWifiMac::GetAID(uint32_t i) const
{
  NS_ASSERT (i < m_aids.size());
  NS_ASSERT(((1 <= m_aids[i]) && (m_aids[i] <= 8191)) || (m_aids[i] == 8192));
  return m_aids[i];
}

std::vector<uint32_t> 
StaWifiMac::GetAids (void) const
{
  for (auto&& aid : m_aids)
      NS_ASSERT(((1 <= aid) && (aid <= 8191)) || (aid == 8192));
  return m_aids;
}

Time 
StaWifiMac::GetRawDuration (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rawDuration;
}

bool
StaWifiMac::Is(uint8_t blockbitmap, uint8_t j)
{
  return (((blockbitmap >> j) & 0x01) == 0x01);
}

void
StaWifiMac::SetAID (uint32_t aid, uint32_t i)
{
  NS_ASSERT (i < m_aids.size());
  NS_ASSERT ((1 <= aid) && (aid <= 8191));
  m_aids[i] = aid;
}

void 
StaWifiMac::AddAID (uint32_t aid)
{
  NS_ASSERT((1 <= aid) && (aid <= 8191));
  m_aids.push_back(aid);
}

void
StaWifiMac::SetRawDuration(Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_rawDuration = interval;
}

void
StaWifiMac::SetDataBuffered()
{
  m_dataBuffered = true;
}

void
StaWifiMac::ClearDataBuffered()
{
  m_dataBuffered = false;
}

void
StaWifiMac::SetInRAWgroup()
{
  m_inRawGroup = true;
}

void
StaWifiMac::UnsetInRAWgroup()
{
  m_inRawGroup = false;
}

void
StaWifiMac::SetMaxMissedBeacons (uint32_t missed)
{
  NS_LOG_FUNCTION (this << missed);
  m_maxMissedBeacons = missed;
}

void
StaWifiMac::SetProbeRequestTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_probeRequestTimeout = timeout;
}

void
StaWifiMac::SetAssocRequestTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_assocRequestTimeout = timeout;
}

void
StaWifiMac::StartActiveAssociation (void)
{
  NS_LOG_FUNCTION (this);
  TryToEnsureAssociated ();
}

void
StaWifiMac::SetActiveProbing (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  if (enable)
    {
      Simulator::ScheduleNow (&StaWifiMac::TryToEnsureAssociated, this);
    }
  else
    {
      m_probeRequestEvent.Cancel ();
    }
  m_activeProbing = enable;
}

bool StaWifiMac::GetActiveProbing (void) const
{
  return m_activeProbing;
}

void
StaWifiMac::SendPspoll (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_PSPOLL);
  hdr.SetId(GetAID(0));
  hdr.SetAddr1 (GetBssid());
  hdr.SetAddr2 (GetAddress ());

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (hdr);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_pspollDca->Queue (packet, hdr);
}

void
StaWifiMac::SendPspollIfnecessary (void)
{
  //assume only send one beacon during RAW
  if ( m_rawStart & m_inRawGroup && m_pagedStaRaw && m_dataBuffered )
    {
     // SendPspoll ();  //pspoll not really send, just put ps-poll frame in m_pspollDca queue
    }
  else if (!m_rawStart && m_dataBuffered && !m_outsideRawEvent.IsRunning ()) //in case the next beacon coming during RAW, could it happen?
   {
     // SendPspoll ();
   }
}

	uint32_t StaWifiMac::GetSelfPageSliceNum (void)
	{

		return 0;
	}

	void StaWifiMac::S1gTIMReceived(S1gBeaconHeader beacon)
	{
		m_TIM = beacon.GetTIM();

		if (!m_pagedInDtim && !(m_TIM.GetDTIMCount() == 0)) //(!(m_pagedInDtim ^ m_TIM.GetDTIMCount() == 0))
			return;

		m_pageSlice = beacon.GetpageSlice();

		m_selfAid = GetAID(0) & 0x07;
		m_selfSubBlock = (GetAID(0) >> 3) & 0x07;
		m_selfBlock = (GetAID(0) >> 6) & 0x1f;
		m_selfPage = GetAID(0) >> 11;
		//Encoded block subfield

		m_PagePeriod = m_pageSlice.GetPagePeriod();
		m_Pageindex_slice = m_pageSlice.GetPageindex();
		m_PageSliceCount = m_pageSlice.GetPageSliceCount();
		m_TIMOffset = m_pageSlice.GetTIMOffset();

		m_DTIMCount = m_TIM.GetDTIMCount();
		//m_PageSliceNum = m_TIM.GetPageSliceNum ();


		if (m_TIM.GetDTIMCount() == 0)
		{
			m_pagedInDtim = false;
			if (testtrackit)
			NS_LOG_DEBUG("[aid=" << GetAID(0) << "] received DTIM beacon.");
			m_pageSlice = beacon.GetpageSlice();
			// TIM element length is 2 when both Partial Virtual Bitmap and Bitmap Control fields are 0 - they are not present in TIM
			// TIM element length is 3 when Partial Virtual Bitmap is 0 - it is not present in TIM
			// else, TIM element length is 3 + size of Encoded Blocks in Partial Virtual Bitmap
			if (m_TIM.GetInformationFieldSize() > 2)
				NS_ASSERT(m_pageSlice.GetPageindex() == m_TIM.GetPageIndex());
			else
			 {
				m_TIM.SetBitmapControl(0);
			 }

			//non first page slice, use info from TIM
			// m_PageSliceNum from TIM is not used, seems not necessary
			// This cannot happen in TIM beacons because station will never wake up for TIM that is not in its page
			if (m_selfPage != m_pageSlice.GetPageindex())
			{
				if (testtrackit)
				NS_LOG_DEBUG("[aid=" << GetAID(0) << "] is not located in page " << (int)m_Pageindex << ", but in page " << (int)m_selfPage << ". Scheduling sleep until the next DTIM beacon.");
				m_pagedInDtim = false;
				GoToSleepCurrentTIM(beacon); // schedule wakeup for next DTIM
				return;
			}

			m_PageBitmapLen = m_pageSlice.GetInformationFieldSize() - 4;

			uint8_t * map = m_pageSlice.GetPageBitmap();
			m_BlockOffset = m_pageSlice.GetBlockOffset();

			if (m_selfBlock < m_BlockOffset)
			{
				// My block is not included in this Page Slice element
				if (testtrackit)
				NS_LOG_DEBUG(
						"[aid=" << GetAID(0) << "] belongs to a block " << (int)m_selfBlock << " which is not included in current page (" << (int)m_selfPage << ") because block offset in Page Slice element is " << (int)m_BlockOffset << " Scheduling sleep until the next DTIM beacon.");
				m_pagedInDtim = false;
				GoToSleepCurrentTIM(beacon);
				//if (trackSleep && GetAID(0) > 62) std::cout << "aid=" << GetAID () << ", GoToSleepCurrentTIM from L417" << std::endl;
				return;
			}

			NS_ASSERT(m_selfBlock >= m_BlockOffset);
			for (uint8_t i = 0; i < m_PageBitmapLen; i++)
			{
				m_PageBitmap[i] = *map;
				map++;
			}

			if (m_pageSlice.GetPageBitmapLength() != 0 && IsInPagebitmap(m_selfBlock))
			{
				//NS_ASSERT(m_pageSlice.GetPageBitmapLength() != 0);
				m_pagedInDtim = true;
				SetDataBuffered();
			}
			else
			{
				ClearDataBuffered();
				m_pagedInDtim = false;
				if (testtrackit)
				NS_LOG_DEBUG("[aid=" << GetAID(0) << "] Page bitmap did not indicate traffic for me. Scheduling sleep until the next DTIM beacon.");
				GoToSleepCurrentTIM(beacon); // schedule wakeup for next DTIM
				//if (trackSleep && GetAID(0) > 62) std::cout << "aid=" << GetAID () << ", GoToSleepCurrentTIM from L440" << std::endl;
				return;
			}
			NS_ASSERT(m_dataBuffered);

			uint32_t selfPageSliceNumber = (m_selfBlock - m_BlockOffset) / m_pageSlice.GetPageSliceLen();
			if (m_selfBlock - m_BlockOffset > (m_pageSlice.GetPageSliceCount() - 1) * m_pageSlice.GetPageSliceLen() && m_pageSlice.GetPageSliceCount() != 0)
			{
				NS_ASSERT(m_pageSlice.GetPageSliceCount() == m_TIM.GetPageSliceNum());
				selfPageSliceNumber--;
			}

			if (selfPageSliceNumber != m_TIM.GetPageSliceNum() && m_TIM.GetPageSliceNum() != 31)
			{
				if (testtrackit)
				NS_LOG_DEBUG(
						"[AID: " << GetAID(0) << "]: This page slice (" << (int)m_TIM.GetPageSliceNum () << ") is not my page slice. My page slice number is " << selfPageSliceNumber);
				// sleep until my TIM
				if (testtrackit)
				NS_LOG_DEBUG(
						"Sleep until my TIM for " << selfPageSliceNumber + m_pageSlice.GetTIMOffset () << " * BI");
				GoToSleep(MicroSeconds((selfPageSliceNumber + m_pageSlice.GetTIMOffset()) * beacon.GetBeaconCompatibility().GetBeaconInterval()));
				//if (trackSleep && GetAID(0) > 62) std::cout << "aid=" << GetAID (0) << ", GoToSleepCurrentTIM from L460" << std::endl;
				return;
			}
			if (m_TIM.GetPageSliceNum() == 31)
			{
				// whole page is encoded here
			}
		}
		else
		{
			if (testtrackit)
			NS_LOG_DEBUG(
					"[aid=" << this->GetAID(0) << "] received TIM" << (int)m_TIM.GetDTIMCount () << " beacon.");


		}
		/*if ((m_DTIMCount == 0 && m_TIMOffset ==0) || (m_DTIMCount !=0 && (m_DTIMPeriod - m_DTIMCount) == m_TIMOffset) )
		 // page slice start from m_TIMOffset
		 {
		 if (m_selfBlock >= m_BlockOffset && m_selfPage == m_Pageindex_slice)
		 //stations smaller m_BlockOffset are not involved
		 {
		 m_selfBlockPos = m_selfBlock - m_BlockOffset;
		 if (IsInPagebitmap (m_selfBlockPos)) //has packets for the block
		 {
		 BeaconNumForTIM = m_selfBlockPos/m_PageSliceLen;
		 if (BeaconNumForTIM >= m_PageSliceCount)
		 {
		 BeaconNumForTIM--;
		 }
		 }
		 else //has no packets for the block
		 {
		 BeaconNumForTIM = m_PageSliceCount;
		 //wake up after the last page slice, this may happen before the next DTIM, depends on TIM offset and page slice count.
		 }
		 GoToSleep (MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval () * BeaconNumForTIM));
		 }
		 return;
		 }*/

		// extract TIM this is my page slice if I didn't return so far
		m_DTIMPeriod = m_TIM.GetDTIMPeriod();
		m_PageIndex = m_TIM.GetPageIndex();

		uint8_t * partialVBitmap = m_TIM.GetPartialVBitmap();
		//length =  m_TIM.GetInformationFieldSize ();
		NS_ASSERT(m_TIM.GetInformationFieldSize() >= 5);
		//NS_ASSERT (m_TIM.GetInformationFieldSize () >= 2);
		uint8_t pos = 0;
		do
		{
			uint8_t blockind = ((*partialVBitmap) >> 3) & 0x1f;
			partialVBitmap++;
			pos++;
			uint8_t blockbitmap = *partialVBitmap;

			if (blockind == m_selfBlock)
			{
				if ((blockbitmap & (1 << m_selfSubBlock)) == 0) //no packets in subblock
				{
					//if not included in the page slice element, wake up for DTIM
					// reset m_pagedInDtim;
					m_pagedInDtim = false;
					GoToSleepCurrentTIM(beacon);
					return;
				}
				else
				{
					for (uint8_t j = 0; j <= m_selfSubBlock; j++)
					{
						if ((blockbitmap & (0x01 << j)))//==1 is incorrect because sometimes it can be 0000 0010 e.g. 2
						{
							partialVBitmap++;
							pos++;
						}
					}
					uint8_t subblockind = *partialVBitmap;
					if ((subblockind & (1 << (m_selfAid & 0x0007))) == 0) //no packet for me
					{
						m_pagedInDtim = false;
						GoToSleepCurrentTIM(beacon);
						return;
					}
					else // if has packet, set station to sleep after this beacon
					{
						if (testtrackit)
						NS_LOG_DEBUG(
								"[aid=" << this->GetAID(0) << "]" << "Downlink packet indicated for me.");
						// reset m_pagedInDtim;
						// Go to sleep next TIM if next TIM is not DTIM
						if (m_TIM.GetDTIMCount() + 1 < m_TIM.GetDTIMPeriod()) //todo check
						{
							//std::cout << "+++++++ Go to SLEEP NEXT TIM UNTIL DTIM" << std::endl;
							GoToSleepNextTIM(beacon);
						}
						else
						{
							//std::cout << "+++++++ DONT SLEEP NEXT TIM BECAUSE NEXT IS DTIM" << std::endl;
							//SleepUntilMySlot();
						}
						return;
					}
				}
			}
			else
			{
				for (uint8_t k = 0; k <= 7; k++)
				{
					if ((blockbitmap & (1 << k)) == 1)
					{
						partialVBitmap++;
						pos++;
					}
				}
			}
		}
		while (pos < m_TIM.GetInformationFieldSize() - 3); //only partialVBitmap; exclude DTIM COUNT,DTIM period and bitmap control
		GoToSleepCurrentTIM(beacon);
	}

	void
	StaWifiMac::SleepUntilMySlot(void)
	{
		Time t = MicroSeconds(5);
		//Simulator::Schedule(t, &StaWifiMac::WakeUp, this);
	}

	void StaWifiMac::GoToSleepNextTIM(S1gBeaconHeader beacon) //to do, merge with GoToSleepCurrentTIM
	{
		uint8_t BeaconNumForTIM;
		if (m_selfBlock < m_BlockOffset) //not included in the page slice element
		{
			BeaconNumForTIM = m_DTIMPeriod - m_DTIMCount - 1;
		}
		else
		//if included in the page slice element, wake up for after last page slice, not sure that's the correct way.
		{
			BeaconNumForTIM = m_PagePeriod - m_TIM.GetPageSliceNum() - 1;
		}
		if (testtrackit)
		NS_LOG_DEBUG ("GoToSleepNextTIM wake after " << (int)BeaconNumForTIM << " BIs");
		Time sleepTime = MicroSeconds(
				beacon.GetBeaconCompatibility().GetBeaconInterval()
						* BeaconNumForTIM);
		Time timeUntilNextTim = MicroSeconds(beacon.GetBeaconCompatibility().GetBeaconInterval()) - MicroSeconds(Simulator::Now().GetMicroSeconds() % beacon.GetBeaconCompatibility().GetBeaconInterval());
		Simulator::Schedule(timeUntilNextTim, &StaWifiMac::GoToSleep, this, sleepTime);
		//std::cout << "BEACON NUM FOR TIM=" << (int)BeaconNumForTIM << std::endl;
		//std::cout << "BEACON Interval=" << (int)beacon.GetBeaconCompatibility().GetBeaconInterval() << std::endl;
		//std::cout << "+++At " << Simulator::Now().GetMicroSeconds() << "us: GoToSleepNextTIM:" << sleepTime.GetMicroSeconds() << " FULL" << std::endl;
		//std::cout << "At " << MicroSeconds(Simulator::Now().GetMicroSeconds() % beacon.GetBeaconCompatibility().GetBeaconInterval()) << " us OFFSET" << std::endl;
		//std::cout << "-*-*-*-*-*-*-*-*-TIME UNTIL NEXT TIM=" << timeUntilNextTim.GetMicroSeconds() << "us" << " --NOW: " << Simulator::Now().GetMicroSeconds() << std::endl;
		//std::cout << "-*-*-*-*-*-*-*-*-SLEEP TIME=" << sleepTime.GetMicroSeconds() << "us" << std::endl;

		//GoToSleep (MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval () * BeaconNumForTIM));
	}

	void StaWifiMac::GoToSleepCurrentTIM(S1gBeaconHeader beacon)
	{
		uint8_t BeaconNumForTIM;
		if (m_selfBlock < m_BlockOffset) //not included in the page slice element
		{
			BeaconNumForTIM = m_DTIMPeriod - m_DTIMCount;
		}
		else
		//if included in the page slice element, wake up for after last page slice, not sure that's the correct way.
		{
			BeaconNumForTIM = m_PagePeriod - m_TIM.GetPageSliceNum();
		}
		//std::cout << "BEACON NUM FOR TIM=" << (int)BeaconNumForTIM << std::endl;
		//std::cout << "BEACON Interval=" << (int)beacon.GetBeaconCompatibility().GetBeaconInterval() << std::endl;
		Time sleepTime = MicroSeconds(beacon.GetBeaconCompatibility().GetBeaconInterval() * BeaconNumForTIM);
		//std::cout << "+++At " << Simulator::Now().GetMicroSeconds() << "us: GoToSleepCurrentTIM:" << sleepTime.GetMicroSeconds() << " FULL" << std::endl;

		//reducing the sleep time for the time elapsed since last beacon until now
		sleepTime -= MicroSeconds(Simulator::Now().GetMicroSeconds() % beacon.GetBeaconCompatibility().GetBeaconInterval());
		//std::cout << sleepTime.GetMicroSeconds() << " DEDUCTED until my next beacon" << std::endl;

		//Time timeUntilNextTim = MicroSeconds(beacon.GetBeaconCompatibility().GetBeaconInterval()) - MicroSeconds(Simulator::Now().GetMicroSeconds() % beacon.GetBeaconCompatibility().GetBeaconInterval());
		//std::cout << "At " << MicroSeconds(Simulator::Now().GetMicroSeconds() % beacon.GetBeaconCompatibility().GetBeaconInterval()) << " us OFFSET" << std::endl;
		//std::cout << "TIME UNTIL NEXT TIM=" << timeUntilNextTim.GetMicroSeconds() << "us" << std::endl;
		GoToSleep(sleepTime);
	}

	Time StaWifiMac::GetEarlyWakeTime(void) const
	{
		NS_LOG_FUNCTION(this << 0); //allow station to switch states 2500
		return MicroSeconds(0);
	}

void StaWifiMac::GoToSleep (Time sleeptime)
	{
		// beacon latency is 2.36ms and station needs to be in sync with AP so reduce sleeptime
		if (sleeptime < GetEarlyWakeTime())
		{
			if (testtrackit)
			NS_LOG_DEBUG(
					"At " << Simulator::Now().GetSeconds() << " s AID " << this->GetAID(0) << " CANNOT SLEEP because sleeptime " << sleeptime.GetSeconds() << " s < EarlyWakeTime.");
			return;
		}
		sleeptime -= GetEarlyWakeTime();
		if (!m_low->GetPhy()->IsStateSleep()
				&& (sleeptime.GetMicroSeconds() > 0))
		{
			if (testtrackit)
				NS_LOG_DEBUG(
					"At " << Simulator::Now().GetSeconds() << " s AID " << this->GetAID(0) << " switches to SLEEP. Schedule wake-up after " << sleeptime.GetSeconds() << " s.");
			m_low->GetPhy()->SetSleepMode();
			Simulator::Schedule(sleeptime, &StaWifiMac::WakeUp, this);
			//std::cout << "+++At " << Simulator::Now().GetMicroSeconds() << "us: GoToSleep:" << sleeptime.GetMicroSeconds() << " FULL" << std::endl;

		}
	}

	void StaWifiMac::WakeUp(void)
	{
		if (m_low->GetPhy()->IsStateSleep())
		{
			if (testtrackit)
				NS_LOG_DEBUG(
					"At " << Simulator::Now().GetSeconds() << " s AID " << this->GetAID(0) << " switches to AWAKE");
			m_low->GetPhy()->ResumeFromSleep();
		}
	}

void
StaWifiMac::GoToSleepBinary (int value)
{
     if (IsAssociated() && !receivingBeacon)
     {
        if ( value == 0)
        {
            //called finish raw slot, after receiving beacon also if it's not my slot
            //if(!outsideraw && !stationrawslot && !waitingack)
            if(!waitingack && !m_low->GetPhy()->IsStateSleep())
            {
                m_low->GetPhy()->SetSleepMode();
                if (testtrackit)
                	NS_LOG_DEBUG (m_low->GetAddress() << " GoToSleepBinary sleepok. not wating ack");
            }
            else if (waitingack && (outsideraw || stationrawslot))
            {
                //NS_LOG_DEBUG (m_low->GetAddress() << " no sleep, wating ack and my slot");
            }
            else
            {
                //NS_LOG_DEBUG (m_low->GetAddress() << " no sleep, wating ack..");
            }
        }
        else if ( value == 1)
         {

             m_low->GetPhy()->SetSleepMode();
             if (testtrackit)
            	 NS_LOG_DEBUG (m_low->GetAddress() << " GoToSleepBinary go to sleep after beacon received");
         }
     }
 }

void
StaWifiMac::SleepIfQueueIsEmpty(bool value)
{
   waitingack = !value;
   
   if (IsAssociated() && !receivingBeacon)
     {
         if(!HasPacketsInQueue() && !waitingack)

            {
                m_low->GetPhy()->SetSleepMode();
            }
            
         if(HasPacketsInQueue() && !waitingack && !outsideraw && !stationrawslot)

            {
                m_low->GetPhy()->SetSleepMode();
            }
            
         //if (waitingack && !outsideraw && !stationrawslot)
         if (!outsideraw && !stationrawslot)
            {
                m_low->GetPhy()->SetSleepMode();
            }
            
         if (waitingack && (outsideraw || stationrawslot))

            {

            }
    }
}

bool StaWifiMac::HasPacketsInQueue() 
{           
    //NS_LOG_DEBUG ( GetAddress () << " Check packets in queue ");
    //check also if ack received
    return (m_edca.find(AC_VO)->second->GetEdcaQueue()->GetSize() > 0 || 
            m_edca.find (AC_VI)->second->GetEdcaQueue()->GetSize() > 0 || 
            m_edca.find (AC_BE)->second->GetEdcaQueue()->GetSize() > 0 ||
            m_edca.find (AC_BK)->second->GetEdcaQueue()->GetSize() > 0);    
}

//actions to do when station wakes up for beacon
void
StaWifiMac::BeaconWakeUp (void)
{
    //to be changed when finish shared slot is different to beginning raw interval
    //outsideraw = false;

    m_low->GetPhy()->ResumeFromSleep();
    //if (!this->IsAssociated() && receivingBeacon)
    m_beaconWakeUpEvent = Simulator::Schedule (beaconInterval, &StaWifiMac::BeaconWakeUp, this);
    receivingBeacon = true;
    //NS_LOG_DEBUG ( GetAddress () << ",Wake Up for beacon," << Simulator::Now().GetSeconds() << ",beacon interval," << beaconInterval.GetSeconds());
    //NS_LOG_DEBUG ( GetAddress () << ",Wake Up for beacon," << Simulator::Now().GetSeconds());
}

void
StaWifiMac::OnAssociated() {
	for (auto i : trackit)
		if (GetAID(0) == i)
			testtrackit = true;
	m_assocLogger(GetBssid());
	// start only allowing transmissions during specific slot periods
	//DenyDCAAccess();
}

void
StaWifiMac::OnDeassociated() {
    m_deAssocLogger (GetBssid ());
    // allow tranmissions until reassociated
    //GrantDCAAccess();
    TryToEnsureAssociated();

}

	bool StaWifiMac::IsInPagebitmap(uint8_t block)
	{
		uint8_t Ind, offset;
		bool inPage;
		Ind = block / 8;
		offset = block % 8;
		inPage = m_PageBitmap[Ind] & (1 << offset);
		return inPage;
	}


void
StaWifiMac::RawSlotStartBackoff (void)
{    
    if (m_insideBackoffEvent.IsRunning ())
     {
        m_insideBackoffEvent.Cancel ();
     } //a bug is fixed, prevent previous RAW from disabling current RAW.
    Time os = Simulator::Now() + m_currentslotDuration;
    //NS_LOG_DEBUG ( m_low->GetAddress () << " Inside backoff scheduled " << Simulator::Now() << m_currentslotDuration);
    
    m_insideBackoffEvent = Simulator::Schedule(m_currentslotDuration, &StaWifiMac::InsideBackoff, this);
    //m_pspollDca->AccessAllowedIfRaw (true);
    //m_dca->AccessAllowedIfRaw (true);
    //m_edca.find (AC_VO)->second->AccessAllowedIfRaw (true);
    //m_edca.find (AC_VI)->second->AccessAllowedIfRaw (true);
    //m_edca.find (AC_BE)->second->AccessAllowedIfRaw (true);
    //m_edca.find (AC_BK)->second->AccessAllowedIfRaw (true);
    Simulator::Schedule(MicroSeconds(160), &StaWifiMac::RawSlotStartBackoffPostpone, this);

    //during its slot, the station wakes up, checking if it has packets before
    stationrawslot = true;
    if(m_low->GetPhy()->IsStateSleep() && HasPacketsInQueue())
    {
       //NS_LOG_DEBUG ( m_low->GetAddress () << " Wake Up for my slot " << Simulator::Now().GetSeconds());
       WakeUp();
    }
    //stationrawslot = true;
    //StartRawbackoff();
    Simulator::Schedule(MicroSeconds(160), &StaWifiMac::StartRawbackoff, this);
}

void
StaWifiMac::RawSlotStartBackoffPostpone (void)
{
     m_pspollDca->AccessAllowedIfRaw (true);
     m_dca->AccessAllowedIfRaw (true);
     m_edca.find (AC_VO)->second->AccessAllowedIfRaw (true);
     m_edca.find (AC_VI)->second->AccessAllowedIfRaw (true);
     m_edca.find (AC_BE)->second->AccessAllowedIfRaw (true);
     m_edca.find (AC_BK)->second->AccessAllowedIfRaw (true);

}

void
StaWifiMac::InsideBackoff (void)
{
   m_pspollDca->AccessAllowedIfRaw (false);
   m_dca->AccessAllowedIfRaw (false);
   m_edca.find (AC_VO)->second->AccessAllowedIfRaw (false);
   m_edca.find (AC_VI)->second->AccessAllowedIfRaw (false);
   m_edca.find (AC_BE)->second->AccessAllowedIfRaw (false);
   m_edca.find (AC_BK)->second->AccessAllowedIfRaw (false);
   //go to sleep at the end of raw slot
   stationrawslot = false;
   //NS_LOG_DEBUG ( m_low->GetAddress () << " Go to sleep at end of RAW slot " << Simulator::Now().GetSeconds());
   GoToSleepBinary(0);
}

void
StaWifiMac::StartRawbackoff (void)
{
  m_pspollDca->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed); //not really start raw useless allowedAccessRaw is true;
  m_dca->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed);
  m_edca.find (AC_VO)->second->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed);
  m_edca.find (AC_VI)->second->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed);
  m_edca.find (AC_BE)->second->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed);
  m_edca.find (AC_BK)->second->RawStart (m_slotDuration, m_crossSlotBoundaryAllowed);

}

void
StaWifiMac::OutsideRawStartBackoff (void)
{
   if (m_insideBackoffEvent.IsRunning ())
     {
       m_insideBackoffEvent.Cancel ();
     }
   //stationrawslot = false;
   outsideraw = true;
  //wake up for shared slot
   if(m_low->GetPhy()->IsStateSleep() && HasPacketsInQueue())
    {
      //NS_LOG_DEBUG ( m_low->GetAddress () << " Wake Up for slot outside raw " << Simulator::Now().GetSeconds());
      WakeUp();
    }
  Simulator::Schedule(MicroSeconds(160), &StaWifiMac::RawSlotStartBackoffPostpone, this);
  StaWifiMac::m_pspollDca->OutsideRawStart ();
  m_dca->OutsideRawStart();
  m_edca.find (AC_VO)->second->OutsideRawStart();
  m_edca.find (AC_VI)->second->OutsideRawStart();
  m_edca.find (AC_BE)->second->OutsideRawStart();
  m_edca.find (AC_BK)->second->OutsideRawStart();
  /*
  It seems Simulator::ScheduleNow take longer time to execuate than without schedule.

  During debugging, we found that when OutsideRawStartBackoff and StaWifiMac::S1gBeaconReceived execuate at the
  same time, even  function OutsideRawStartBackoff() at first execute, and function S1gBeaconReceived() execuate
  later, the function OutsideRawStartBackoff calls executate later than functions called by S1gBeaconReceived ().

  Here is an example, (m_rawStart && !m_inRawGroup) execuated in S1gBeaconReceived. We log the info,

  first, OutsideRawStartBackoff calls
  OutsideRawStartBackoff ,20	+42302720123.0ns	00:00:00:00:00:14, start to schedule
  seconds, (m_rawStart && !m_inRawGroup) related functions is execuated,
    raw start but not inRAWgroup
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 0, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 0, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 0, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 0, 00:00:00:00:00:14
    m_rawStart && !m_inRawGroup, +42302720123.0ns, 00:00:00:00:00:14
  Last, functions called by OutsideRawStartBackoff is execuated.
    ousideRAW
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 1, 00:00:00:00:00:14
    EdcaTxopN::OutsideRawStart, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 1, 00:00:00:00:00:14
    EdcaTxopN::OutsideRawStart, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 1, 00:00:00:00:00:14
    EdcaTxopN::OutsideRawStart, 00:00:00:00:00:14
    EdcaTxopN::AccessAllowedIfRaw, +42302720123.0ns, 1, 00:00:00:00:00:14
    EdcaTxopN::OutsideRawStart, 00:00:00:00:00:14
  */
}

void
StaWifiMac::SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_pspollDca->SetWifiRemoteStationManager (stationManager);
  RegularWifiMac::SetWifiRemoteStationManager (stationManager);
}

void
StaWifiMac::SendProbeRequest (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetProbeReq ();
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (Mac48Address::GetBroadcast ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtProbeRequestHeader probe;
  probe.SetSsid (GetSsid ());
  probe.SetSupportedRates (GetSupportedRates ());
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

  if (m_probeRequestEvent.IsRunning ())
    {
      m_probeRequestEvent.Cancel ();
    }
  m_probeRequestEvent = Simulator::Schedule (m_probeRequestTimeout,
                                             &StaWifiMac::ProbeRequestTimeout, this);
}

void
StaWifiMac::TxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this);
  RegularWifiMac::TxOk (hdr);

  if (hdr.IsDisassociation ()
      && IsWaitDisAssocTxOk ())
    {
      NS_LOG_DEBUG ( GetAddress () << " disassociation request is received bt " << hdr.GetAddr1 () );
      SetState (REFUSED);  //use REFUSED
      if (m_disassocRequestEvent.IsRunning ())
        {
          m_disassocRequestEvent.Cancel ();
        }
    }
  //this->GoToSleepBinary(0);
}

void
StaWifiMac::SendDisAssociationRequest (void)
{
   NS_LOG_FUNCTION (this);
    WifiMacHeader hdr;
    hdr.SetDisAssocReq ();
    hdr.SetAddr1 (GetBssid ());
    hdr.SetAddr2 (GetAddress ());
    hdr.SetAddr3 (GetBssid ());
    hdr.SetDsNotFrom ();
    hdr.SetDsNotTo ();
    Ptr<Packet> packet = Create<Packet> ();
    MgtDisAssocRequestHeader disassoc;
    disassoc.SetSsid (GetSsid ());

    if (m_htSupported)
    {
        disassoc.SetHtCapabilities (GetHtCapabilities ());
        hdr.SetNoOrder ();
    }

    if (m_s1gSupported)
    {
        disassoc.SetS1gCapabilities (GetS1gCapabilities ());
        NS_LOG_DEBUG (GetAddress () << " StaWifiMac::SendDisAssociationRequest ");

    }

    SetState (WAIT_DISASSOC_ACK);  // temporary used, should create another state
    //m_aid = 8192; //ensure disassociated station is not affected by Raw
    m_aids.clear();
    packet->AddHeader (disassoc);
    m_dca->Queue (packet, hdr);

    //to do, check weather Disassociation request is received by Ap or not.
    if (m_disassocRequestEvent.IsRunning ())
      {
        m_disassocRequestEvent.Cancel ();
      }
    m_disassocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                             &StaWifiMac::SendDisAssociationRequest, this);
                                             //use same timeout as assocRequest

}

void
StaWifiMac::SendAssociationRequest (void)
{
  NS_LOG_FUNCTION (this << GetBssid ());

  if (!m_s1gSupported)
    {
        fastAssocThreshold = 1023;
    }

if (assocVaule < fastAssocThreshold)
{
  WifiMacHeader hdr;
  hdr.SetAssocReq ();
  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetBssid ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();
  Ptr<Packet> packet = Create<Packet> ();
  MgtAssocRequestHeader assoc;
  assoc.SetSsid (GetSsid ());
  assoc.SetSupportedRates (GetSupportedRates ());
  if (m_htSupported)
    {
      assoc.SetHtCapabilities (GetHtCapabilities ());
      hdr.SetNoOrder ();
    }

  if (m_s1gSupported)
    {
        assoc.SetS1gCapabilities (GetS1gCapabilities ());
    }

  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
  if (!this->IsAssociated())
	  SetState(WAIT_ASSOC_RESP);
  else
	  SetState(WAIT_ANOTHER_ASSOC_RESP);
}

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                           &StaWifiMac::AssocRequestTimeout, this);
}

void
StaWifiMac::SendAnotherAssociationRequest (void)
{
	std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~SendAnotherAssociationRequest~~~~~~~~~~~~~~~~~~~~~~~~~ "
			<< this->IsAssociated() << " my aid= " << (int)GetAID(0) << std::endl;
	SendAssociationRequest();
}

void
StaWifiMac::TryToEnsureAssociated (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state)
    {
    case ASSOCIATED:
      return;
      break;
    case WAIT_PROBE_RESP:
      /* we have sent a probe request earlier so we
         do not need to re-send a probe request immediately.
         We just need to wait until probe-request-timeout
         or until we get a probe response
       */
      break;
    case BEACON_MISSED:
      /* we were associated but we missed a bunch of beacons
       * so we should assume we are not associated anymore.
       * We try to initiate a probe request now.
       */
      m_linkDown ();
      if (m_activeProbing)
        {
          SetState (WAIT_PROBE_RESP);
          SendProbeRequest ();
        }
      break;
    case WAIT_ASSOC_RESP:
      /* we have sent an assoc request so we do not need to
         re-send an assoc request right now. We just need to
         wait until either assoc-request-timeout or until
         we get an assoc response.
       */
      break;
    case REFUSED:
    case WAIT_DISASSOC_ACK:
      /* we have sent an assoc request and received a negative
         assoc resp. We wait until someone restarts an
         association with a given ssid.
       */
      break;
    }
}
void
StaWifiMac::AssocRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_ASSOC_RESP);
  SendAssociationRequest ();
}

void
StaWifiMac::ProbeRequestTimeout (void)
{
  NS_LOG_FUNCTION (this);
  SetState (WAIT_PROBE_RESP);
  SendProbeRequest ();
}

void
StaWifiMac::MissedBeacons (void)
{
  NS_LOG_FUNCTION (this);
  if (m_beaconWatchdogEnd > Simulator::Now ())
    {
      if (m_beaconWatchdog.IsRunning ())
        {
          m_beaconWatchdog.Cancel ();
        }
      m_beaconWatchdog = Simulator::Schedule (m_beaconWatchdogEnd - Simulator::Now (),
                                              &StaWifiMac::MissedBeacons, this);
      return;
    }
  NS_LOG_DEBUG ("beacon missed");
  SetState (BEACON_MISSED);
  TryToEnsureAssociated ();
}

void
StaWifiMac::RestartBeaconWatchdog (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_beaconWatchdogEnd = std::max (Simulator::Now () + delay, m_beaconWatchdogEnd);
  if (Simulator::GetDelayLeft (m_beaconWatchdog) < delay
      && m_beaconWatchdog.IsExpired ())
    {
      NS_LOG_DEBUG ("really restart watchdog.");
      m_beaconWatchdog = Simulator::Schedule (delay, &StaWifiMac::MissedBeacons, this);
    }
}

bool
StaWifiMac::IsAssociated (void) const
{
  return m_state == ASSOCIATED;
}

bool
StaWifiMac::IsWaitAssocResp (void) const
{
  return m_state == WAIT_ASSOC_RESP;
}

bool
StaWifiMac::IsWaitDisAssocTxOk (void) const
{
  return m_state == WAIT_DISASSOC_ACK;
}

void
StaWifiMac::Enqueue (Ptr<const Packet> packet, Mac48Address to)
{    
	NS_LOG_FUNCTION (this << packet << to);

	//in case a packet is added in the queue, it is check if the station should be awake in that slot
	//in case of shared or own slot, the station wakes up
	if (testtrackit)
		NS_LOG_DEBUG (m_low->GetAddress () << ",enqueue," << Simulator::Now().GetSeconds() << "," << m_low->GetPhy()->IsStateSleep());

	//code RAW
	if (m_low->GetPhy()->IsStateSleep() && stationrawslot)
	{
		if (testtrackit)
			NS_LOG_DEBUG (m_low->GetAddress () << ",enqueue in stationrawslot," << Simulator::Now().GetSeconds());
		WakeUp();
	}
	if (m_low->GetPhy()->IsStateSleep() && outsideraw)
	{
		if (testtrackit)
			NS_LOG_DEBUG (m_low->GetAddress () << ",enqueue outsideraw," << Simulator::Now().GetSeconds());
		WakeUp();
	}

  if (!IsAssociated ())
    {
      NotifyTxDrop (packet);
      TryToEnsureAssociated ();
      NS_LOG_DEBUG (m_low->GetAddress () << "  cannot send since not associated");
      return;
    }
  WifiMacHeader hdr;

  //If we are not a QoS AP then we definitely want to use AC_BE to
  //transmit the packet. A TID of zero will map to AC_BE (through \c
  //QosUtilsMapTidToAc()), so we use that as our default here.
  uint8_t tid = 0;

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
      tid = QosUtilsGetTidForPacket (packet);
      //Any value greater than 7 is invalid and likely indicates that
      //the packet had no QoS tag, so we revert to zero, which'll
      //mean that AC_BE is used.
      if (tid > 7)
        {
          tid = 0;
        }
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

  hdr.SetAddr1 (GetBssid ());
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (to);
  hdr.SetDsNotFrom ();
  hdr.SetDsTo ();

  /*MacLowTransmissionParameters params;
  params.DisableRts();
  params.DisableAck();
  params.DisableNextData();
  Time txTime = m_low->CalculateOverallTxTime(packet, &hdr, params);
  std::cout << "Sta aid=" << this->GetAID(0) << " TX time=" << txTime.GetMicroSeconds() << " us, " << packet->GetSize() << std::endl;*/
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


void 
StaWifiMac::S1gBeaconReceived (S1gBeaconHeader beacon)
{
    //in case station is receiving beacon, it does not go to sleep
    receivingBeacon = false;		
    if (m_outsideRawEvent.IsRunning ())
     {
        m_outsideRawEvent.Cancel ();          //avoid error when actual beacon interval become shorter, otherwise, AccessAllowedIfRaw will set again after raw starting
     }

  if (!m_aids.size()) // send assoication request when Staion is not assoicated
    {
      m_dca->AccessAllowedIfRaw (true);
    }
  else if (m_rawStart & m_inRawGroup && m_pagedStaRaw && m_dataBuffered ) // if m_pagedStaRaw is true, only m_dataBuffered can access channel
    {
      m_outsideRawEvent = Simulator::Schedule(m_lastRawDurationus, &StaWifiMac::OutsideRawStartBackoff, this);

      m_pspollDca->AccessAllowedIfRaw (true);
      m_dca->AccessAllowedIfRaw (false);
      m_edca.find (AC_VO)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_VI)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BE)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BK)->second->AccessAllowedIfRaw (false);
      StartRawbackoff();
    }
  else if (m_rawStart && m_inRawGroup && !m_pagedStaRaw  )
    {
      m_outsideRawEvent = Simulator::Schedule(m_lastRawDurationus, &StaWifiMac::OutsideRawStartBackoff, this);

      m_pspollDca->AccessAllowedIfRaw (false);
      m_dca->AccessAllowedIfRaw (false);
      m_edca.find (AC_VO)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_VI)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BE)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BK)->second->AccessAllowedIfRaw (false);
	  Simulator::Schedule(m_statSlotStart, &StaWifiMac::RawSlotStartBackoff, this);
    }
 else if (m_rawStart && !m_inRawGroup) //|| (m_rawStart && m_inRawGroup && m_pagedStaRaw && !m_dataBuffered)
    {
      m_outsideRawEvent = Simulator::Schedule(m_lastRawDurationus, &StaWifiMac::OutsideRawStartBackoff, this);

      m_pspollDca->AccessAllowedIfRaw (false);
      m_dca->AccessAllowedIfRaw (false);
      m_edca.find (AC_VO)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_VI)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BE)->second->AccessAllowedIfRaw (false);
      m_edca.find (AC_BK)->second->AccessAllowedIfRaw (false);

      StartRawbackoff();
    }
   // else (!m_rawStart),  this case cannot happen, since we assume s1g beacon always indicating one raw
    m_rawStart = false;
}

void
StaWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);
  NS_ASSERT (!hdr->IsCtl ());
  if (hdr->GetAddr3 () == GetAddress ())
    {
      NS_LOG_LOGIC ("packet sent by us.");
      return;
    }
  else if (hdr->GetAddr1 () != GetAddress ()
           && !hdr->GetAddr1 ().IsGroup ())
    {
      NS_LOG_LOGIC ("packet is not for us");
      NotifyRxDrop (packet);
      return;
    }
  else if (hdr->IsData ())
    {
      if (!IsAssociated ())
        {
          NS_LOG_LOGIC ("Received data frame while not associated: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (!(hdr->IsFromDs () && !hdr->IsToDs ()))
        {
          NS_LOG_LOGIC ("Received data frame not from the DS: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->GetAddr2 () != GetBssid ())
        {
          NS_LOG_LOGIC ("Received data frame not from the BSS we are associated with: ignore");
          NotifyRxDrop (packet);
          return;
        }
      if (hdr->IsQosData ())
        {
    	  if (testtrackit)
    	  NS_LOG_DEBUG (GetAddress () << " received qos data from " << hdr->GetAddr3 () << " @ " << Simulator::Now().GetMicroSeconds());
          if (hdr->IsQosAmsdu ())
            {
              NS_ASSERT (hdr->GetAddr3 () == GetBssid ());
              DeaggregateAmsduAndForward (packet, hdr);
              packet = 0;
            }
          else
            {
              ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
            }
        }
      else
        {
    	  if (testtrackit)
    	  NS_LOG_DEBUG (GetAddress () << " received data from " << hdr->GetAddr3 () << " @ " << Simulator::Now().GetMicroSeconds());
          ForwardUp (packet, hdr->GetAddr3 (), hdr->GetAddr1 ());
        }
      return;
    }
  else if (hdr->IsProbeReq ()
           || hdr->IsAssocReq ())
    {
      //This is a frame aimed at an AP, so we can safely ignore it.
      NotifyRxDrop (packet);
      return;
    }
  else if (hdr->IsBeacon ())
    {
      MgtBeaconHeader beacon;
      packet->RemoveHeader (beacon);
      bool goodBeacon = false;
      if (GetSsid ().IsBroadcast ()
          || beacon.GetSsid ().IsEqual (GetSsid ()))
        {
          goodBeacon = true;
        }
      SupportedRates rates = beacon.GetSupportedRates ();
      for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          uint32_t selector = m_phy->GetBssMembershipSelector (i);
          if (!rates.IsSupportedRate (selector))
            {
              goodBeacon = false;
            }
        }
      if ((IsWaitAssocResp () || IsAssociated ()) && hdr->GetAddr3 () != GetBssid ())
        {
          goodBeacon = false;
        }
      if (goodBeacon)
        {
          Time delay = MicroSeconds (beacon.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
          SetBssid (hdr->GetAddr3 ());
        }
      if (goodBeacon && m_state == BEACON_MISSED)
        {
          SetState (WAIT_ASSOC_RESP);
          SendAssociationRequest ();
        }
      return;
    }
  else if (hdr->IsS1gBeacon ())
    {
      S1gBeaconHeader beacon;
      packet->RemoveHeader (beacon);
      bool goodBeacon = false;
    if ((IsWaitAssocResp () || IsAssociated ()) && hdr->GetAddr3 () != GetBssid ()) // for debug
     {
       goodBeacon = false;
     }
    else
     {
      goodBeacon = true;
     }
    if (goodBeacon)
     {
       Time delay = MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval () * m_maxMissedBeacons);
       RestartBeaconWatchdog (delay);
       //SetBssid (beacon.GetSA ());
       SetBssid (hdr->GetAddr3 ()); //for debug
     }
    if (goodBeacon && m_state == BEACON_MISSED)
     {
       SetState (WAIT_ASSOC_RESP);
       SendAssociationRequest ();
     }
    if (goodBeacon)
     {
        timeDifferenceBeacon = Simulator::Now().GetMicroSeconds() - timeBeacon;
        timeBeacon = Simulator::Now().GetMicroSeconds();
        if(firstBeacon)
        {
            beaconInterval = MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval ());
            int64x64_t interval = static_cast<int64x64_t> (beaconInterval);
            interval = interval / 10000000;
            interval = (interval).GetHigh();
            interval = interval * 10000000;
            Time intervalFirstBeacon = static_cast<Time> (interval);
            //std::cout << "++++++++++++++++++us beaconInterval = " << beaconInterval << "; Now=" << Simulator::Now().GetMicroSeconds() << std::endl;
            //Time intervallobeacon = MicroSeconds (98920);
            m_beaconWakeUpEvent = Simulator::Schedule (intervalFirstBeacon, &StaWifiMac::BeaconWakeUp, this);
            //m_beaconWakeUpEvent = Simulator::Schedule (Time(100000), &StaWifiMac::BeaconWakeUp, this);
            firstBeacon = false;
        }
        
        UnsetInRAWgroup ();
        if (IsAssociated())
        {
        	uint8_t * rawassign;
        	rawassign = beacon.GetRPS().GetRawAssignment();
        	uint16_t raw_len = beacon.GetRPS().GetInformationFieldSize();
        	uint16_t rawAssignment_len = 6;
        	if (raw_len % rawAssignment_len !=0)
        	{
        		NS_ASSERT ("RAW configuration incorrect!");
        	}
        	uint8_t RAW_number = raw_len/rawAssignment_len;

        	uint16_t m_slotDurationCount=0;
        	uint16_t m_slotNum=0;
        	uint64_t m_currentRAW_start=0;
        	uint64_t rawStartOffset = 0;
        	m_lastRawDurationus = MicroSeconds(0);
        	for (uint8_t raw_index=0; raw_index < RAW_number; raw_index++)
        	{
        		auto ass = beacon.GetRPS().GetRawAssigmentObj(raw_index);

        		if (ass.GetRawTypeIndex() == 4) // only support Generic Raw (paged STA RAW or not)
        		{
        			m_pagedStaRaw = true;
        		}
        		else
        		{
        			m_pagedStaRaw = false;
        		}
        		m_currentRAW_start=m_currentRAW_start+(500 + m_slotDurationCount * 120)*m_slotNum;//0
        		m_slotDurationCount = ass.GetSlotDurationCount();//200
        		m_slotNum = ass.GetSlotNum();//3
        		m_slotDuration = MicroSeconds(500 + m_slotDurationCount * 120);//24500
        		m_lastRawDurationus = m_lastRawDurationus + m_slotDuration * m_slotNum;//3*24500
        		m_crossSlotBoundaryAllowed = ass.GetSlotCrossBoundary() == 0x0001;

        		if (ass.GetRawGroupPage() == ((GetAID(0) >> 11 ) & 0x0003)) //in the page indexed
        		{
        			uint16_t statsPerSlot = 0;
        			uint16_t statRawSlot = 0;

        			Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
        			uint16_t offset = m_rv->GetValue (0, 1023);
        			offset =0; // for test
        			statsPerSlot = (ass.GetRawGroupAIDEnd() - ass.GetRawGroupAIDStart() + 1)/m_slotNum;
        			//statRawSlot = ((GetAID(0) & 0x03ff)-raw_start)/statsPerSlot;
        			statRawSlot = ((GetAID(0) & 0x03ff)+offset)%m_slotNum;

        			if ((ass.GetRawGroupAIDStart() <= (GetAID(0) & 0x03ff)) && ((GetAID(0) & 0x03ff) <= ass.GetRawGroupAIDEnd()))
        			{
        				m_statSlotStart = MicroSeconds((500 + m_slotDurationCount * 120)*statRawSlot+m_currentRAW_start);//
        				//NS_LOG_DEBUG ("aid=" << GetAID(0) << ", ass.GetRawStart=" << (int)ass.GetRawStart() << ", m_statSlotStart=" << m_statSlotStart);

        				SetInRAWgroup ();
        				m_currentslotDuration = m_slotDuration; //To support variable time duration among multiple RAWs

        				// NS_LOG_DEBUG (Simulator::Now () << ", StaWifiMac:: GetAID(0) = " << GetAID(0) <<  ", m_statSlotStart=" << m_statSlotStart << ", m_lastRawDurationus = " << m_lastRawDurationus << ", m_currentslotDuration = " << m_currentslotDuration);
        				//break; //break should not used if multiple RAW is supported
        			}
        			//NS_LOG_DEBUG (Simulator::Now () << ", StaWifiMac:: GetAID(0) = " << GetAID(0) << ", raw_start =" << raw_start << ", raw_end=" << raw_end << ", m_statSlotStart=" << m_statSlotStart << ", m_lastRawDurationus = " << m_lastRawDurationus << ", m_currentslotDuration = " << m_currentslotDuration);
        		}
        	}
        	m_rawStart = true;
        	S1gTIMReceived(beacon);
        }
        AuthenticationCtrl AuthenCtrl;
         AuthenCtrl = beacon.GetAuthCtrl ();
         fasTAssocType = AuthenCtrl.GetControlType ();
         if (!fasTAssocType)  //only support centralized cnotrol
           {
             fastAssocThreshold = AuthenCtrl.GetThreshold();
           }
     }
    S1gBeaconReceived (beacon);
    waitingack = false;
    outsideraw = false;
    //set when the station has to wake up again (next beacon, own slot, shared slot)
    Simulator::Schedule(m_lastRawDurationus, &StaWifiMac::WakeUp, this); //shared slot
    if (m_pagedInDtim)
    {
        //std::cout << "sharedSlot at=" << Simulator::Now() + m_lastRawDurationus << " us, mySlotStart = " << Simulator::Now() + m_statSlotStart << std::endl;
        /*std::cout << "NOW=" << Simulator::Now() << ", timeBeacon=" << timeBeacon << std::endl;
        MacLowTransmissionParameters params;
        params.DisableRts();
        params.DisableAck();
        params.DisableNextData();
        Time bufferTimeToAllowBeaconToBeReceived = m_low->CalculateOverallTxTime(packet, hdr, params);
        std::cout << "dif at=" << Simulator::Now() + m_statSlotStart - bufferTimeToAllowBeaconToBeReceived << std::endl;
    	*/
        Time sleept = Time();
        if (m_statSlotStart > MicroSeconds (1))
        	sleept = m_statSlotStart - MicroSeconds (1);
        else
        	sleept = m_statSlotStart;
        Simulator::Schedule(sleept, &StaWifiMac::WakeUp, this); //own slot
    	m_pagedInDtim = false;
    }

    //std::cout << "sharedSlot at=" << Simulator::Now() + m_lastRawDurationus << " us, mySlotStart = " << Simulator::Now() + m_statSlotStart << std::endl;
    GoToSleepBinary(1);
    return;
   }
  else if (hdr->IsProbeResp ())
    {
      if (m_state == WAIT_PROBE_RESP)
        {
          MgtProbeResponseHeader probeResp;
          packet->RemoveHeader (probeResp);
          if (!probeResp.GetSsid ().IsEqual (GetSsid ()))
            {
              //not a probe resp for our ssid.
              return;
            }
          SupportedRates rates = probeResp.GetSupportedRates ();
          for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
            {
              uint32_t selector = m_phy->GetBssMembershipSelector (i);
              if (!rates.IsSupportedRate (selector))
                {
                  return;
                }
            }
          SetBssid (hdr->GetAddr3 ());
          Time delay = MicroSeconds (probeResp.GetBeaconIntervalUs () * m_maxMissedBeacons);
          RestartBeaconWatchdog (delay);
          if (m_probeRequestEvent.IsRunning ())
            {
              m_probeRequestEvent.Cancel ();
            }
          SetState (WAIT_ASSOC_RESP);
          SendAssociationRequest ();
        }
      return;
    }
  else if (hdr->IsAssocResp () || m_state == WAIT_ANOTHER_ASSOC_RESP)
    {
      if (m_state == WAIT_ASSOC_RESP)
        {
          MgtAssocResponseHeader assocResp;
          packet->RemoveHeader (assocResp);
          if (m_assocRequestEvent.IsRunning ())
            {
              m_assocRequestEvent.Cancel ();
            }
          if (assocResp.GetStatusCode ().IsSuccess ())
            {
        	  AddAID(assocResp.GetAID());
        	  SetState (ASSOCIATED);
        	  if (testtrackit)
        		  NS_LOG_DEBUG("[" << this->GetAddress() <<"] is associated and has AID = " << this->GetAID(0));

        	  /*if (GetAID (0) == 1 && Simulator::Now() < Seconds (10))
        		  Simulator::Schedule(Seconds(10), &StaWifiMac::SendAnotherAssociationRequest, this);
        	  else if (GetAids ().size() > 1)
        	  {
        		  SetState(ASSOCIATED);
        		  std::cout << "++++++++++++++++++++++++++all AIDs = ";
        		  for (auto aid : GetAids())
        			  std::cout << "   " << aid;
        		  std::cout << std::endl;
        		  return;
        	  }*/
              SupportedRates rates = assocResp.GetSupportedRates ();
              if (m_htSupported)
                {
                  HtCapabilities htcapabilities = assocResp.GetHtCapabilities ();
                  m_stationManager->AddStationHtCapabilities (hdr->GetAddr2 (),htcapabilities);
                }
              
              if (m_s1gSupported)
                {
                  S1gCapabilities s1gcapabilities = assocResp.GetS1gCapabilities ();
                  NS_LOG_DEBUG (GetAddress () << ", receive " << uint16_t( s1gcapabilities.GetChannelWidth ()));
                  m_stationManager->AddStationS1gCapabilities (hdr->GetAddr2 (),s1gcapabilities);
                }

              for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
                {
                  WifiMode mode = m_phy->GetMode (i);
                  for (uint32_t j = 0; j < m_phy->m_deviceRateSet.size (); j++)
                   {
                    if (m_phy->m_deviceRateSet[j] == mode )
                    {
                    	NS_LOG_DEBUG (GetAddress () << ", AddSupportedMode " << hdr->GetAddr2 () << ", " << mode);
                        m_stationManager->AddSupportedMode (hdr->GetAddr2 (), mode);
                        if (rates.IsBasicRate (mode.GetDataRate ()))
                        {
                            m_stationManager->AddBasicMode (mode);
                        }
                    }
                  }
                  if (rates.IsSupportedRate (mode.GetDataRate ()))
                    {
                      m_stationManager->AddSupportedMode (hdr->GetAddr2 (), mode);
                      if (rates.IsBasicRate (mode.GetDataRate ()))
                        {
                          m_stationManager->AddBasicMode (mode);
                        }
                    }
                }
              if (m_htSupported)
                {
                  HtCapabilities htcapabilities = assocResp.GetHtCapabilities ();
                  for (uint32_t i = 0; i < m_phy->GetNMcs (); i++)
                    {
                      uint8_t mcs = m_phy->GetMcs (i);
                      if (htcapabilities.IsSupportedMcs (mcs))
                        {
                          m_stationManager->AddSupportedMcs (hdr->GetAddr2 (), mcs);
                          //here should add a control to add basic MCS when it is implemented
                        }
                    }
                }
              if (!m_linkUp.IsNull ())
                {
                  m_linkUp ();
                }
            }
          else
            {
              NS_LOG_DEBUG ("assoc refused");
              SetState (REFUSED);
            }
        }
      return;
    }

  //Invoke the receive handler of our parent class to deal with any
  //other frames. Specifically, this will handle Block Ack-related
  //Management Action frames.
  RegularWifiMac::Receive (packet, hdr);
}

SupportedRates
StaWifiMac::GetSupportedRates (void) const
{
  SupportedRates rates;
  if (m_htSupported)
    {
      for (uint32_t i = 0; i < m_phy->GetNBssMembershipSelectors (); i++)
        {
          rates.SetBasicRate (m_phy->GetBssMembershipSelector (i));
        }
    }
  for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
    {
      WifiMode mode = m_phy->GetMode (i);
      rates.AddSupportedRate (mode.GetDataRate ());
    }
  return rates;
}

HtCapabilities
StaWifiMac::GetHtCapabilities (void) const
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
StaWifiMac::GetS1gCapabilities (void) const
{
  S1gCapabilities capabilities;
  capabilities.SetS1gSupported (1);
  capabilities.SetStaType (GetStaType ()); //need to define/change
  capabilities.SetChannelWidth (GetChannelWidth ());
  capabilities.SetPageSlicingSupport(1);
  return capabilities;
}

void
StaWifiMac::SetState (MacState value)
{
		if (value == ASSOCIATED && m_state != ASSOCIATED)
		{
			OnAssociated();

		}
		else if (value != ASSOCIATED && m_state == ASSOCIATED)
		{
			OnDeassociated();

		}
  m_state = value;
}

} //namespace ns3
