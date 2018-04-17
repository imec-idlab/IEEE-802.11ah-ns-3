/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */


#include "ns3/simulator.h"
#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "extension-headers.h"
#include "ns3/log.h"

namespace ns3 {

/***********************************************************
 *          S1G Beacon Frame
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED (S1gBeaconHeader);

S1gBeaconHeader::S1gBeaconHeader ()
{
}
    
S1gBeaconHeader::~S1gBeaconHeader ()
{
}

/*
void
S1gBeaconHeader::SetSA (Mac48Address sa)
{
  m_address = sa;
}*/

void
S1gBeaconHeader::SetChangeSequence (uint8_t sequence)
{
  m_sequence = sequence;
}
    
void
S1gBeaconHeader::SetNextTBTT (uint32_t tbtt)
{
  m_tbtt = tbtt;
}

void
S1gBeaconHeader::SetCompressedSSID (uint32_t compressedssid)
{
  m_compressedssid = compressedssid;
}

void
S1gBeaconHeader::SetAccessNetwork (uint8_t accessnetwork)
{
  m_accessnetwork = accessnetwork;
}

void
S1gBeaconHeader::SetBeaconCompatibility (S1gBeaconCompatibility compatibility)
{
  m_beaconcompatibility = compatibility;
}

void
S1gBeaconHeader::SetTIM (TIM tim)
{
  m_tim = tim;
}

void
S1gBeaconHeader::SetRPS (RPS rps)
{
  m_rps = rps;
}

void
S1gBeaconHeader::SetpageSlice (pageSlice page)
{
  m_pageSlice = page;
}
    
void
S1gBeaconHeader::SetAuthCtrl (AuthenticationCtrl auth)
{
   m_auth = auth;
}

/*
Mac48Address
S1gBeaconHeader::GetSA (void) const
{
  return m_address;
}*/
    
uint32_t
S1gBeaconHeader::GetTimeStamp (void) const
{
  return m_timestamp;
}
    
uint8_t
S1gBeaconHeader::GetChangeSequence (void) const
{
  return m_sequence;
}
    
uint32_t
S1gBeaconHeader::GetNextTBTT (void) const
{
  return m_tbtt;
}

uint32_t
S1gBeaconHeader::GetCompressedSSID (void) const
{
  return m_compressedssid;
}

uint8_t
S1gBeaconHeader::GetAccessNetwork (void) const
{
  return m_accessnetwork;
}
    
S1gBeaconCompatibility
S1gBeaconHeader::GetBeaconCompatibility (void) const
{
  return m_beaconcompatibility;
}

TIM
S1gBeaconHeader::GetTIM (void) const
{
  return m_tim;
}

pageSlice
S1gBeaconHeader::GetpageSlice (void) const
{
  return m_pageSlice;
}
    
RPS
S1gBeaconHeader::GetRPS (void) const
{
  return m_rps;
}
    
AuthenticationCtrl
S1gBeaconHeader::GetAuthCtrl (void) const
{
  return m_auth;
}


TypeId
S1gBeaconHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::S1gBeaconHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<S1gBeaconHeader> ()
  ;
  return tid;
}

TypeId
S1gBeaconHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
S1gBeaconHeader::Print (std::ostream &os) const
{
  // to do
}

//suppose all subfield are present, change in future
uint32_t
S1gBeaconHeader::GetSerializedSize (void) const
{
  uint32_t size = 0;
  size += 4; // Timestamp
  size += 1; // Change Sequence
  size += 3; // Next TBTT
  size += 4; // Compressed SSID
  size += 1; // Access Network
  size += m_beaconcompatibility.GetSerializedSize ();
  size += m_tim.GetSerializedSize ();
  size += m_rps.GetSerializedSize ();
  if (!m_tim.GetDTIMCount())
	  size += m_pageSlice.GetSerializedSize ();
  size += m_auth.GetSerializedSize ();
  
  return size;
}

void
S1gBeaconHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    uint32_t stamp_lst32 = (uint32_t)Simulator::Now ().GetMicroSeconds ();
    i.WriteHtolsbU32 (stamp_lst32);
    i.WriteU8 (m_sequence);
    
    uint16_t tbtt_h = (uint16_t)(m_tbtt >> 8); //write m_tbtt (23-8) bits
    i.WriteHtolsbU16 (tbtt_h);
    uint16_t tbtt_l = (uint8_t)m_tbtt; //write m_tbtt (7-0) bits
    i.WriteU8 (tbtt_l);
    
    i.WriteHtolsbU32 (m_compressedssid);
    i.WriteU8 (m_accessnetwork);
    i = m_beaconcompatibility.Serialize (i);
    i = m_tim.Serialize (i);
    i = m_rps.Serialize (i);
    if (!m_tim.GetDTIMCount())
    	i = m_pageSlice.Serialize (i);
    i = m_auth.Serialize (i);
}

uint32_t
S1gBeaconHeader::Deserialize (Buffer::Iterator start)
{
    
    Buffer::Iterator i = start;
    m_timestamp = i.ReadLsbtohU32 ();
    m_sequence = i.ReadU8 ();
    
    uint32_t tbtth16 = i.ReadLsbtohU16 (); //read m_tbtt (23-8) bits
    uint32_t tbttl8 = i.ReadU8 (); //read m_tbtt (7-0) bits
    m_tbtt = (tbtth16 << 8) | tbttl8;
    
    m_compressedssid = i.ReadLsbtohU32 ();
    m_accessnetwork = i.ReadU8 ();
    i = m_beaconcompatibility.Deserialize (i);
    i = m_tim.Deserialize (i);
    i = m_rps.Deserialize (i);
    if (!m_tim.GetDTIMCount())
    	i = m_pageSlice.Deserialize (i);
    i = m_auth.DeserializeIfPresent (i);

    return i.GetDistanceFrom (start);
}


} //namespace ns3
