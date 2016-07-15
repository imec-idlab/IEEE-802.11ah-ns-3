/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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
 */

#include "s1g-beacon-compatibility.h"
#include "ns3/assert.h"
#include "ns3/simulator.h"

namespace ns3 {

S1gBeaconCompatibility::S1gBeaconCompatibility ()
{
  m_length = 8;
}

S1gBeaconCompatibility::~S1gBeaconCompatibility ()
{
}

void
S1gBeaconCompatibility::SetBeaconInterval (uint64_t us)
{
  m_beaconinterval = us;
}

uint64_t
S1gBeaconCompatibility::GetBeaconInterval (void) const
{
  return m_beaconinterval;
}

uint32_t
S1gBeaconCompatibility::Gettsfcompletetion (void) const
{
  return m_tsfcompletetion;
}
    
WifiInformationElementId
S1gBeaconCompatibility::ElementId () const
{
  return IE_S1G_BEACON_COMPATIBILITY;
}

uint8_t
S1gBeaconCompatibility::GetInformationFieldSize () const
{
  return m_length;
}

void
S1gBeaconCompatibility::SerializeInformationField (Buffer::Iterator start) const
{
 start = m_compatibility.Serialize (start);
 start.WriteHtolsbU16 (m_beaconinterval / 1024);
 uint64_t tsf_completion = Simulator::Now ().GetMicroSeconds ();
 start.WriteHtolsbU32 ((tsf_completion >> 32) & 0xffffffff);
}

uint8_t
S1gBeaconCompatibility::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  start = m_compatibility.Deserialize (start);
  m_beaconinterval = start.ReadLsbtohU16 ();
  m_beaconinterval *= 1024;
  m_tsfcompletetion = start.ReadLsbtohU32 ();
  return 8;
}

ATTRIBUTE_HELPER_CPP (S1gBeaconCompatibility);
    
std::ostream &
operator << (std::ostream &os, const S1gBeaconCompatibility &compatibility)
{
  os << compatibility.GetInformationFieldSize ();
  return os;
}
    
std::istream &operator >> (std::istream &is, S1gBeaconCompatibility &compatibility)
{
  uint64_t interval;
  is >> interval;
  compatibility.SetBeaconInterval(interval);
  return is;
}

} //namespace ns3
