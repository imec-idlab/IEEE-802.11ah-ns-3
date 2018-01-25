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
  m_aid = 8192;
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

  //Let the lower layers know that we are acting as a non-AP STA in
  //an infrastructure BSS.
  SetTypeOfStation (STA);
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

uint32_t
StaWifiMac::GetAID (void) const
{
  NS_ASSERT ((1 <= m_aid) && (m_aid<= 8191) || (m_aid == 8192));
  return m_aid;
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
StaWifiMac::SetAID (uint32_t aid)
{
  NS_ASSERT ((1 <= aid) && (aid <= 8191));
  m_aid = aid;
}

void
StaWifiMac::SetRawDuration (Time interval)
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
  hdr.SetId (GetAID());
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

void 
StaWifiMac::S1gTIMReceived (S1gBeaconHeader beacon)
{
    uint8_t BeaconNumForTIM;
    uint8_t m_selfBlockPos;
    uint8_t blockind;
    uint8_t subblockind;
    
    uint8_t m_blockbitmap;
          
    uint8_t * m_PartialVBitmap;
    uint8_t length;
       
    m_selfAid = GetAID() & 0x07;
    m_selfSubBlock = (GetAID() >> 3)& 0x07;
    m_selfBlock = (GetAID() >> 6) & 0x1f;
    m_selfPage = GetAID() >> 11;
        //Encoded block subfield
    m_TIM = beacon.GetTIM ();
    m_DTIMCount = m_TIM.GetDTIMCount ();  
    m_DTIMPeriod = m_TIM.GetDTIMPeriod ();
    m_PageSliceNum = m_TIM.GetPageSliceNum ();
    m_PageIndex = m_TIM.GetPageIndex ();
      
    if (m_DTIMCount == 0)
       {
          m_pageSlice = beacon.GetpageSlice ();
          m_PagePeriod = m_pageSlice.GetPagePeriod ();
          m_Pageindex_slice = m_pageSlice.GetPageindex ();
          NS_ASSERT (m_Pageindex_slice == m_PageIndex);
          m_PageSliceLen = m_pageSlice.GetPageSliceLen ()  ;
          m_PageSliceCount = m_pageSlice.GetPageSliceCount () ;
          m_BlockOffset = m_pageSlice.GetBlockOffset () ;
          m_TIMOffset = m_pageSlice.GetTIMOffset ();
          m_PageBitmapLen = m_pageSlice.GetInformationFieldSize () - 4;
          uint8_t * map = m_pageSlice.GetPageBitmap ();
          for (uint8_t i=0; i < m_PageBitmapLen ; i++)
            {
               m_PageBitmap[i] = *map;
               map++;
           }
       }
          
    if ((m_DTIMCount == 0 && m_TIMOffset ==0) || (m_DTIMCount !=0 && (m_DTIMPeriod - m_DTIMCount) == m_TIMOffset) )
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
        }
       
    //non first page slice, use info from TIM 
    // m_PageSliceNum from TIM is not used, seems not necessary
    if (m_selfPage != m_Pageindex ) 
        {
          return;
          //not sure, how to handle when TIM and station are not in the same page
    }
          
    m_PartialVBitmap = m_TIM.GetPartialVBitmap ();
    length =  m_TIM.GetInformationFieldSize ();
    NS_ASSERT (length >=5);
    for (uint8_t i = 0; i <  length -3;) //exclude DTIM COUNT,DTIM period. bitmap control
        {
            blockind = ((*m_PartialVBitmap) >> 3) & 0x1f;
            m_PartialVBitmap++;
            m_blockbitmap = *m_PartialVBitmap;
                
            if (blockind == m_selfBlock) 
             {
                if ( (m_blockbitmap & (1 << m_selfSubBlock)) == 0) //no packets in subblock
                  {
                     //if not included in the page slice element, wake up for DTIM
                    GoToSleepCurrentTIM (beacon);
                    return;
                  }
                else
                 {
                    for (uint8_t j=0; j <= m_selfSubBlock; j++)
                      {
                        if ((m_blockbitmap & (1 << j)) == 1)
                          {
                            m_PartialVBitmap++;
                          }   
                      }
                    subblockind = *m_PartialVBitmap; 
                    if ((subblockind & (1 < m_selfAid) ) == 0) //no packet for me
                     {
                        GoToSleepCurrentTIM (beacon);
                        return;
                     }
                    else // if has packet, set station to sleep after this beacon
                     {
                        GoToSleepNextTIM (beacon);
                        return;
                     }
                 }
             }
            else 
             {
                for (uint8_t k=0; k <= 7; k++)
                 {
                    if ((m_blockbitmap & (1 << k)) == 1)
                    {
                        m_PartialVBitmap++;
                    }
                 }
             } 
        }  
    GoToSleepCurrentTIM (beacon);
}

void
StaWifiMac::GoToSleepNextTIM (S1gBeaconHeader beacon) //to do, merge with GoToSleepCurrentTIM
{
   uint8_t BeaconNumForTIM;
   if (m_selfBlock < m_BlockOffset) //not included in the page slice element
     {
        BeaconNumForTIM = m_DTIMPeriod - m_DTIMCount - 1;
     }
   else
    //if included in the page slice element, wake up for after last page slice, not sure that's the correct way.    
    {
       BeaconNumForTIM = m_PagePeriod - m_PageSliceNum -1 ;
    }
  GoToSleep (MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval () * BeaconNumForTIM)); 
}

void
StaWifiMac::GoToSleepCurrentTIM (S1gBeaconHeader beacon)
{
    uint8_t BeaconNumForTIM;
   if (m_selfBlock < m_BlockOffset) //not included in the page slice element
     {
        BeaconNumForTIM = m_DTIMPeriod - m_DTIMCount;
     }
   else
    //if included in the page slice element, wake up for after last page slice, not sure that's the correct way.    
    {
       BeaconNumForTIM = m_PagePeriod - m_PageSliceNum;
    }
  GoToSleep (MicroSeconds (beacon.GetBeaconCompatibility().GetBeaconInterval () * BeaconNumForTIM)); 
}

    

void
StaWifiMac::GoToSleep(Time  sleeptime) 
{
    if (!m_low->GetPhy()->IsStateSleep() && (sleeptime.GetMicroSeconds() != 0))
      {
        m_low->GetPhy()->SetSleepMode();
        Simulator::Schedule(sleeptime, &StaWifiMac::WakeUp, this);
      }
}

void
StaWifiMac::WakeUp (void)
{ 
    if (m_low->GetPhy()->IsStateSleep())
      {
        m_low->GetPhy()->ResumeFromSleep();
      }
}

bool
StaWifiMac::IsInPagebitmap (uint8_t block)
{
    uint8_t Ind, offset;
    bool inPage;
    Ind = block/8;
    offset = block % 8;
    inPage = m_PageBitmap[Ind] & (1 < offset);
    return inPage;
}



void
StaWifiMac::S1gBeaconReceived (S1gBeaconHeader beacon)
{
    S1gTIMReceived (beacon);
    if (m_outsideRawEvent.IsRunning ())
     {
        m_outsideRawEvent.Cancel ();          //avoid error when actual beacon interval become shorter, otherwise, AccessAllowedIfRaw will set again after raw starting
        //Simulator::ScheduleNow(&StaWifiMac::OutsideRawStartBackoff, this);

     }

  if (m_aid == 8192) // send assoication request when Staion is not assoicated
    {
      m_dca->AccessAllowedIfRaw (true);
    }
  else if (m_rawStart && m_inRawGroup && m_pagedStaRaw && m_dataBuffered ) // if m_pagedStaRaw is true, only m_dataBuffered can access channel
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
StaWifiMac::RawSlotStartBackoff (void)
{
    if (m_insideBackoffEvent.IsRunning ())
     {
        m_insideBackoffEvent.Cancel ();
     } //a bug is fixed, prevent previous RAW from disabling current RAW.
    m_insideBackoffEvent = Simulator::Schedule(m_currentslotDuration, &StaWifiMac::InsideBackoff, this);
    m_pspollDca->AccessAllowedIfRaw (true);
    m_dca->AccessAllowedIfRaw (true);
    m_edca.find (AC_VO)->second->AccessAllowedIfRaw (true);
    m_edca.find (AC_VI)->second->AccessAllowedIfRaw (true);
    m_edca.find (AC_BE)->second->AccessAllowedIfRaw (true);
    m_edca.find (AC_BK)->second->AccessAllowedIfRaw (true);
    StartRawbackoff();
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

}


void
StaWifiMac::StartRawbackoff (void)
{
  m_pspollDca->RawStart (); //not really start raw useless allowedAccessRaw is true;
  m_dca->RawStart ();
  m_edca.find (AC_VO)->second->RawStart ();
  m_edca.find (AC_VI)->second->RawStart ();
  m_edca.find (AC_BE)->second->RawStart ();
  m_edca.find (AC_BK)->second->RawStart ();

}


void
StaWifiMac::OutsideRawStartBackoff (void)
{
   if (m_insideBackoffEvent.IsRunning ())
     {
       m_insideBackoffEvent.Cancel ();
     }
  /*Simulator::ScheduleNow(&DcaTxop::OutsideRawStart, StaWifiMac::m_pspollDca);
  Simulator::ScheduleNow(&DcaTxop::OutsideRawStart, StaWifiMac::m_dca);
  Simulator::ScheduleNow(&EdcaTxopN::OutsideRawStart, StaWifiMac::m_edca.find (AC_VO)->second);
  Simulator::ScheduleNow(&EdcaTxopN::OutsideRawStart, StaWifiMac::m_edca.find (AC_VI)->second);
  Simulator::ScheduleNow(&EdcaTxopN::OutsideRawStart, StaWifiMac::m_edca.find (AC_BE)->second);
  Simulator::ScheduleNow(&EdcaTxopN::OutsideRawStart, StaWifiMac::m_edca.find (AC_BK)->second);*/
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
      NS_LOG_UNCOND ( GetAddress () << " disassociation request is received bt " << hdr.GetAddr1 () );
      SetState (REFUSED);  //use REFUSED
      if (m_disassocRequestEvent.IsRunning ())
        {
          m_disassocRequestEvent.Cancel ();
        }
    }
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
        NS_LOG_UNCOND (GetAddress () << " StaWifiMac::SendDisAssociationRequest ");

    }

    SetState (WAIT_DISASSOC_ACK);  // temporary used, should create another state
    m_aid = 8192; //ensure disassociated station is not affected by Raw
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
    //NS_LOG_UNCOND ("StaWifiMac::SendAssociationRequest (void)");

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
        NS_LOG_UNCOND ("StaWifiMac::SendAssociationRequest (void)");

    }

  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
  SetState (WAIT_ASSOC_RESP);
}

  if (m_assocRequestEvent.IsRunning ())
    {
      m_assocRequestEvent.Cancel ();
    }
  m_assocRequestEvent = Simulator::Schedule (m_assocRequestTimeout,
                                           &StaWifiMac::AssocRequestTimeout, this);
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
  if (!IsAssociated ())
    {
      NotifyTxDrop (packet);
      TryToEnsureAssociated ();
      NS_LOG_UNCOND (m_low->GetAddress () << "  cannot send since not associated");
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
        UnsetInRAWgroup ();
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
         m_lastRawDurationus = MicroSeconds(0);
    for (uint8_t raw_index=0; raw_index < RAW_number; raw_index++)
      {
        uint8_t rawtypeindex = rawassign[raw_index*rawAssignment_len+0] & 0x07;
        if (rawtypeindex == 4) // only support Generic Raw (paged STA RAW or not)
          {
            m_pagedStaRaw = true;
          }
        else
          {
            m_pagedStaRaw = false;
          }
        uint8_t pageindex = rawassign[raw_index*rawAssignment_len+3] & 0x03;

         uint16_t m_rawslot;
         m_rawslot = (uint16_t(rawassign[raw_index*rawAssignment_len+2]) << 8) | (uint16_t(rawassign[raw_index*rawAssignment_len+1]));
         uint8_t m_SlotFormat = uint8_t (m_rawslot >> 15) & 0x0001;
         uint8_t m_slotCrossBoundary = uint8_t (m_rawslot >> 14) & 0x0002;

          m_currentRAW_start=m_currentRAW_start+(500 + m_slotDurationCount * 120)*m_slotNum;

         NS_ASSERT (m_SlotFormat <= 1);

         if (m_SlotFormat == 0)
           {
             m_slotDurationCount = (m_rawslot >> 6) & 0x00ff;
             m_slotNum = m_rawslot & 0x003f;
           }
         else if (m_SlotFormat == 1)
           {
             m_slotDurationCount = (m_rawslot >> 3) & 0x07ff;
             m_slotNum = m_rawslot & 0x0007;
           }

        m_slotDuration = MicroSeconds(500 + m_slotDurationCount * 120);
        m_lastRawDurationus = m_lastRawDurationus + m_slotDuration * m_slotNum;

         if (pageindex == ((GetAID() >> 11 ) & 0x0003)) //in the page indexed
           {

             uint8_t rawgroup_l = rawassign[raw_index*rawAssignment_len+3];
             uint8_t rawgroup_m = rawassign[raw_index*rawAssignment_len+4];
             uint8_t rawgroup_h = rawassign[raw_index*rawAssignment_len+5];
             uint32_t rawgroup = (uint32_t(rawassign[raw_index*rawAssignment_len+5]) << 16) | (uint32_t(rawassign[raw_index*rawAssignment_len+4]) << 8) | uint32_t(rawassign[raw_index*rawAssignment_len+3]);
             uint16_t raw_start = (rawgroup >> 2) & 0x000003ff;
             uint16_t raw_end = (rawgroup >> 13) & 0x000003ff;

               uint16_t statsPerSlot = 0;
               uint16_t statRawSlot = 0;

               Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable> ();
               uint16_t offset = m_rv->GetValue (0, 1023);
               offset =0; // for test
               statsPerSlot = (raw_end - raw_start + 1)/m_slotNum;
               //statRawSlot = ((GetAID() & 0x03ff)-raw_start)/statsPerSlot;
               statRawSlot = ((GetAID() & 0x03ff)+offset)%m_slotNum;

             if ((raw_start <= (GetAID() & 0x03ff)) && ((GetAID() & 0x03ff) <= raw_end))
               {
                 m_statSlotStart = MicroSeconds((500 + m_slotDurationCount * 120)*statRawSlot+m_currentRAW_start);
                 SetInRAWgroup ();
                 m_currentslotDuration = m_slotDuration; //To support variable time duration among multiple RAWs
                  // NS_LOG_UNCOND (Simulator::Now () << ", StaWifiMac:: GetAID() = " << GetAID() << ", raw_start =" << raw_start << ", raw_end=" << raw_end << ", m_statSlotStart=" << m_statSlotStart << ", m_lastRawDurationus = " << m_lastRawDurationus << ", m_currentslotDuration = " << m_currentslotDuration);
                  //break; //break should not used if multiple RAW is supported
               }
               //NS_LOG_UNCOND (Simulator::Now () << ", StaWifiMac:: GetAID() = " << GetAID() << ", raw_start =" << raw_start << ", raw_end=" << raw_end << ", m_statSlotStart=" << m_statSlotStart << ", m_lastRawDurationus = " << m_lastRawDurationus << ", m_currentslotDuration = " << m_currentslotDuration);

           }
      }
         m_rawStart = true; //?
       
      static uint8_t PagePeriod = 0;

      static uint8_t Pageindex = 0;
      static uint8_t PageSliceLen = 0;
      static uint8_t PageSliceCount = 0;
      static uint8_t BlockOffset = 0;
      static uint8_t TIMOffset = 0;
      
      static uint32_t PageBitmap = 0;
     
      uint8_t PagePeriod_read = 0;
      uint8_t Pageindex_read = 0;
      uint8_t PageSliceLen_read = 0;
      uint8_t PageSliceCount_read = 0;
      uint8_t BlockOffset_read = 0;
      uint8_t TIMOffset_read = 0;
      uint32_t PageBitmap_read = 0;

     
      pageSlice pageSli; 
      pageSli = beacon.GetpageSlice ();
         
      
      PagePeriod_read = pageSli.GetPagePeriod ();
      
      Pageindex_read = pageSli.GetPageindex () ;
      PageSliceLen_read = pageSli.GetPageSliceLen ()  ;
      PageSliceCount_read = pageSli.GetPageSliceCount () ;
      BlockOffset_read = pageSli.GetBlockOffset () ;
      TIMOffset_read = pageSli.GetTIMOffset ();
      
      //PageBitmap_read = pageSli.GetPageBitmap ();
               //printf ("sta Pageindex_read =%d \n" ,Pageindex_read);
               //printf ("sta Pageindex =%d \n", Pageindex);

      NS_ASSERT (PagePeriod_read == PagePeriod );
      NS_ASSERT (Pageindex_read == (Pageindex & 0x03) );
      NS_ASSERT (PageSliceLen_read == (PageSliceLen & 0x1f));
      NS_ASSERT (PageSliceCount_read == (PageSliceCount & 0x1f));
      NS_ASSERT (BlockOffset_read == (BlockOffset & 0x1f));
      NS_ASSERT (TIMOffset_read == (TIMOffset & 0x0f));
      //NS_ASSERT (PageBitmap_read == PageBitmap);
      
      
    PagePeriod++;      
    Pageindex++;
    PageSliceLen++;
    PageSliceCount++;
    BlockOffset++;
    TIMOffset++;
    PageBitmap++;

    
    
         printf ("pageSli.TIMOffset_read =%d \n", TIMOffset_read);
         
         
         
         
      static  uint8_t m_DTIMCount = 0 ; //!< DTIM Count
      static  uint8_t m_DTIMPeriod = 0 ; //!< DTIM Period
      static   uint8_t m_TrafficIndicator = 0 ;
      static  uint8_t m_PageSliceNum = 0 ;
      static  uint8_t m_PageIndex = 0 ;
        
        //Encoded block subfield
      static  uint8_t m_blockcontrol = 0 ;
      static  uint8_t m_blockoffset = 0 ;
      static  uint8_t m_blockbitmap = 1 ;
      static  uint8_t m_subblock = 0 ;
      static  uint8_t subblocklength = 0 ;
      static  uint8_t m_blockbitmap_trail = 0 ;
      
      
      uint8_t m_DTIMCount_read = 0 ; //!< DTIM Count
      uint8_t m_DTIMPeriod_read = 0 ; //!< DTIM Period
      uint8_t m_TrafficIndicator_read = 0 ;
      uint8_t m_PageSliceNum_read = 0 ;
      uint8_t m_PageIndex_read = 0 ;
        
        //Encoded block subfield
      uint8_t m_blockcontrol_read = 0 ;
      uint8_t m_blockoffset_read = 0 ;
      uint8_t m_blockbitmap_read = 1 ;
      uint8_t m_subblock_read = 0 ;
      uint8_t subblocklength_read = 0 ;
      uint8_t m_blockbitmap_trail_read = 0 ;
      
      TIM m_TIM;
           // NS_LOG_UNCOND  ("===\n");

      m_TIM = beacon.GetTIM ();
      //NS_LOG_UNCOND ("---\n");

      m_DTIMCount_read = m_TIM.GetDTIMCount ();  
      m_DTIMPeriod_read = m_TIM.GetDTIMPeriod ();
      m_TrafficIndicator_read = m_TIM.GetTrafficIndicator ();
      m_PageSliceNum_read = m_TIM.GetPageSliceNum ();
      m_PageIndex_read = m_TIM.GetPageIndex ();
      
      
      uint8_t * m_PartialVBitmap = m_TIM.GetPartialVBitmap ();
      uint8_t length =  m_TIM.GetInformationFieldSize ();
      m_blockoffset_read = ((*m_PartialVBitmap) >> 3) & 0x1f;
      m_PartialVBitmap++;
      m_blockbitmap_read = *m_PartialVBitmap;
      m_PartialVBitmap++;
      m_subblock_read = *m_PartialVBitmap;
      
      
      printf ("---stawifimac, m_subblock_read %d \n", m_subblock_read);
      //printf ("---stawifimac, m_subblock %d \n", m_subblock);
      //printf ("---stawifimac, length %d \n", length);

      
    
    //TIM::EncodedBlock m_encodedBlock;
    //m_blockoffset_read  = m_encodedBlock.GetBlockOffset ();
    //m_blockbitmap_read = m_encodedBlock.GetBlockBitmap ();
    //m_subblock_read = *m_encodedBlock.GetSubblock ();
    
    
      NS_ASSERT (m_DTIMCount_read == m_DTIMCount);
      NS_ASSERT (m_DTIMPeriod_read == m_DTIMPeriod);
      NS_ASSERT (m_TrafficIndicator_read == (m_TrafficIndicator & 0x01));
      NS_ASSERT (m_PageSliceNum_read == (m_PageSliceNum & 0x1f));
      NS_ASSERT (m_PageIndex_read == (m_PageIndex & 0x03));
      
      NS_ASSERT (m_blockoffset_read == (m_blockoffset & 0x1f));
      NS_ASSERT (m_blockbitmap_read == m_blockbitmap);
      NS_ASSERT (m_subblock_read == m_subblock);

    
    
    m_DTIMCount++; //!< DTIM Count
    m_DTIMPeriod++; //!< DTIM Period
    m_TrafficIndicator++;
    m_PageSliceNum++;
    m_PageIndex++;   
        
        //Encoded block subfield
    m_blockcontrol++ ;
    m_blockoffset++;
    m_blockbitmap++;
    m_blockbitmap_trail++;
    m_subblock++ ;
    m_blockbitmap = pow (2, m_blockbitmap_trail & 0x07);
    
      
      

         AuthenticationCtrl AuthenCtrl;
         AuthenCtrl = beacon.GetAuthCtrl ();
         fasTAssocType = AuthenCtrl.GetControlType ();
         if (!fasTAssocType)  //only support centralized cnotrol
           {
             fastAssocThreshold = AuthenCtrl.GetThreshold();
           }
     }
    S1gBeaconReceived (beacon); //to do, include RPS element processing
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
  else if (hdr->IsAssocResp ())
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
              SetState (ASSOCIATED);
              NS_LOG_DEBUG ("assoc completed");
              SetAID (assocResp.GetAID ());
              SupportedRates rates = assocResp.GetSupportedRates ();
              if (m_htSupported)
                {
                  HtCapabilities htcapabilities = assocResp.GetHtCapabilities ();
                  m_stationManager->AddStationHtCapabilities (hdr->GetAddr2 (),htcapabilities);
                }

              for (uint32_t i = 0; i < m_phy->GetNModes (); i++)
                {
                  WifiMode mode = m_phy->GetMode (i);
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

  return capabilities;
}

void
StaWifiMac::SetState (MacState value)
{
  if (value == ASSOCIATED
      && m_state != ASSOCIATED)
    {
      m_assocLogger (GetBssid ());
    }
  else if (value != ASSOCIATED
           && m_state == ASSOCIATED)
    {
      m_deAssocLogger (GetBssid ());
    }
  m_state = value;
}

} //namespace ns3
