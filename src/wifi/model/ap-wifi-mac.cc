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
#include "extension-headers.h"
#include "mac-low.h"
#include "amsdu-subframe-header.h"
#include "msdu-aggregator.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ApWifiMac");

NS_OBJECT_ENSURE_REGISTERED (ApWifiMac);

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
    .AddAttribute ("EnableBeaconJitter", "If beacons are enabled, whether to jitter the initial send event.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ApWifiMac::m_enableBeaconJitter),
                   MakeBooleanChecker ())
    .AddAttribute ("BeaconGeneration", "Whether or not beacons are generated.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&ApWifiMac::SetBeaconGeneration,
                                        &ApWifiMac::GetBeaconGeneration),
                   MakeBooleanChecker ())
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
  ;
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
          rates.SetBasicRate (m_phy->GetBssMembershipSelector (i));
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
ApWifiMac::SendAssocResp (Mac48Address to, bool success)
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
  //
  uint8_t mac[6];
  to.CopyTo (mac);
  uint8_t aid_l = mac[5];
  uint8_t aid_h = mac[4] & 0x1f;
  uint16_t aid = (aid_h << 8) | (aid_l << 0); //assign mac address as AID
    //NS_LOG_UNCOND ("ap-wifi-mac, set aid = " << aid << ", sta address =" <<  to);
  //
    //NS_LOG_UNCOND ("time = " << Simulator::Now ().GetMicroSeconds () << "ap send, aid= " << aid );
  assoc.SetAID(aid); //
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
  packet->AddHeader (assoc);

  //The standard is not clear on the correct queue for management
  //frames if we are a QoS AP. The approach taken here is to always
  //use the DCF for these regardless of whether we have a QoS
  //association or not.
  m_dca->Queue (packet, hdr);
}

void
ApWifiMac::SendOneBeacon (void)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
    if (m_s1gSupported)
     {
      hdr.SetS1gBeacon ();
      hdr.SetAddr1 (Mac48Address::GetBroadcast ());
      hdr.SetAddr2 (GetAddress ()); // for debug, not accordance with draft, need change
      hdr.SetAddr3 (GetAddress ()); // for debug
      Ptr<Packet> packet = Create<Packet> ();
      S1gBeaconHeader beacon;
      //beacon.SetSA (GetAddress ()); according to draft, shold be the address of AP, to make it easily, use broadcast temporarily
      S1gBeaconCompatibility compatibility;
      compatibility.SetBeaconInterval (m_beaconInterval.GetMicroSeconds ());
      beacon.SetBeaconCompatibility (compatibility);
      /* According to draft, one TIM can carry information of one page stations
         to make it simple, we only support at most 64 stations(one block)
      */
      /*TIM m_tim;
        //m_tim.SetDTIMCount (uint8_t count); //no configure, do not ues current
        //m_tim.SetDTIMPeriod (uint8_t count); //no configure, do not ues current
      m_tim.SetBitmapControl (192); // (b7-b6, page index), (b5-b1, page slice, no support) (b0 traffice indicator, no support)
      TIM::EncodedBlock block;
        //block.SetBlockControl (enum BlockCoding coding); //no configure, support Block Bitmap coding in dafault
      block.SetBlockOffset (1);  // (0-31)
      uint8_t codeinfo[4]={7,2,3,4};
      uint8_t * encodedInfo = codeinfo;
      block.SetEncodedInfo (encodedInfo, 3); //(encodedInfo, include bitmap and subblock)(subblocklength, length of subblock)
      m_tim.SetPartialVBitmap (block);
      beacon.SetTIM (m_tim);*/
      RPS m_rps;
      RPS::RawAssignment raw;
      uint8_t control = 0;
      raw.SetRawControl (control);//support paged STA or not
        //raw.SetRawSlot (uint16_t slot); //not used currently
        //raw.SetRawStart (uint8_t start); //not used currently
      //
         uint32_t page = 0;
         static uint32_t aid_start = 1;
         static uint32_t aid_end = m_rawGroupInterval; //m_rawGroupInterval;
         uint32_t rawinfo = (aid_end << 13) | (aid_start << 2) | page;
         //NS_LOG_UNCOND ("time = " << Simulator::Now ().GetMicroSeconds () << ",  ap-wifi-mac-476, aid_start =" << aid_start << ", aid_end = " << aid_end);
         //NS_LOG_UNCOND ("rawinfo = " << rawinfo);
      //
      raw.SetRawGroup (rawinfo); // (b0-b1, page index) (b2-b12, raw start AID) (b13-b23, raw end AID)
        //
         aid_start = aid_start + m_rawGroupInterval;
         aid_end = aid_end + m_rawGroupInterval;
         if (aid_end > m_totalStaNum)
           {
             aid_start = 1;
             aid_end = m_rawGroupInterval;
           }
         //
        //raw.SetChannelInd (uint16_t channel); //not used currently
        //raw.SetPRAW (uint32_t praw);//not used currently
      m_rps.SetRawAssignment(raw);
      beacon.SetRPS (m_rps);
      /*if (m_htSupported)
        {
           beacon.SetHtCapabilities (GetHtCapabilities ());
           hdr.SetNoOrder ();
         }*/
         //uint16_t adf = hdr.GetFrameControl (); //for test
         //NS_LOG_UNCOND ("DcaTxop::ApWifiMac::SendOneBeacon = wlh," << adf); //for test
      packet->AddHeader (beacon);
      //The beacon has it's own special queue, so we load it in there
      m_beaconDca->Queue (packet, hdr);
     }
    else
     {
         //NS_LOG_LOGIC ("send  beacon " ); //for test
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
     // NS_LOG_LOGIC ("send  beacon 1" ); //for test
      if (m_htSupported)
        {
          beacon.SetHtCapabilities (GetHtCapabilities ());
          hdr.SetNoOrder ();
        }
      //NS_LOG_LOGIC ("send  beacon 2" ); //for test
      packet->AddHeader (beacon);
      //The beacon has it's own special queue, so we load it in there
      m_beaconDca->Queue (packet, hdr);
      }
 // NS_LOG_LOGIC ("send  beacon 3" );  //for test
  m_beaconEvent = Simulator::Schedule (m_beaconInterval, &ApWifiMac::SendOneBeacon, this);
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
  //NS_LOG_LOGIC ("AP waiting   " << segg); //for test
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
          NotifyRxDrop (packet);
        }
      else
        {
          //we can ignore these frames since
          //they are not targeted at the AP
          NotifyRxDrop (packet);
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
                  SendAssocResp (hdr->GetAddr2 (), false);
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
                  //send assoc response with success status.
                  SendAssocResp (hdr->GetAddr2 (), true);
                }
              return;
            }
          else if (hdr->IsDisassociation ())
            {
              m_stationManager->RecordDisassociated (from);
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
  RegularWifiMac::DoInitialize ();
}

} //namespace ns3
