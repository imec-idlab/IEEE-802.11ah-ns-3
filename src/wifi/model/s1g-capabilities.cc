/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013
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
 * Author: Ghada Badawy <gbadawy@rim.com>
 */

#include "s1g-capabilities.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("S1gCapabilities");

S1gCapabilities::S1gCapabilities ()
  :  m_staType (0),
    m_s1gSupported (0),
    m_ChannelWidth (0),
	m_pageSlicingImplemented (0),
	m_ndpPsPollSupported (0)
{
}

WifiInformationElementId
S1gCapabilities::ElementId () const
{
  return IE_S1G_CAPABILITIES;
}

void
S1gCapabilities::SetS1gSupported (uint8_t s1gsupported)
{
  m_s1gSupported = s1gsupported;
}
  
void
S1gCapabilities::SetStaType (uint8_t type)
{
  m_staType = type;
}

void
S1gCapabilities::SetChannelWidth (uint8_t width)
{
  m_ChannelWidth = width;
}

void
S1gCapabilities::SetPageSlicingSupport (uint8_t pageSlicingImplemented)
{
	NS_ASSERT (pageSlicingImplemented <= 1);
	m_pageSlicingImplemented = pageSlicingImplemented;
}

uint8_t
S1gCapabilities::GetPageSlicingSupport (void) const
{
	return m_pageSlicingImplemented;
}

void
S1gCapabilities::SetNdpPsPollSupport (uint8_t psPollingSupported)
{
	NS_ASSERT (psPollingSupported <= 1);
	m_ndpPsPollSupported = psPollingSupported;
}
uint8_t
S1gCapabilities::GetNdpPsPollSupport (void) const
{
	return m_ndpPsPollSupported;
}

uint8_t
S1gCapabilities::GetStaType (void) const
{
  return m_staType;
}

uint8_t
S1gCapabilities::GetChannelWidth (void) const
{
  return m_ChannelWidth;
}

uint8_t
S1gCapabilities::GetInformationFieldSize () const
{
  //we should not be here if s1g is not supported
  NS_ASSERT (m_s1gSupported > 0);
  return 15;
}

Buffer::Iterator
S1gCapabilities::Serialize (Buffer::Iterator i) const
{
  if (m_s1gSupported < 1)
    {
      return i;
    }
  return WifiInformationElement::Serialize (i);
}

uint16_t
S1gCapabilities::GetSerializedSize () const
{
  if (m_s1gSupported < 1)
    {
      return 0;
    }
  return WifiInformationElement::GetSerializedSize ();
}

uint64_t
S1gCapabilities::GetS1gCapabilitiesInfoL64 (void) const
{
  uint64_t val = 0;
  val |= ((uint64_t(m_staType) << 38) & (uint64_t(0x03) << 38)) | ((uint64_t(m_pageSlicingImplemented) << 52) & (uint64_t(0x01) << 52));
  val |= (uint64_t(m_ChannelWidth) << 6) & (uint64_t(0x03) << 6);
  val |= ((uint64_t(m_ndpPsPollSupported) << 50) & (uint64_t(0x01) << 50));
  return val;
}
    
uint16_t
S1gCapabilities::GetS1gCapabilitiesInfoH16 (void) const
{
  uint16_t val = 0;
  return val;
}
    
uint8_t
S1gCapabilities::GetSupportedMCS_NSSH8 (void) const
{
  uint8_t val = 0;
  return val;
}

uint32_t
S1gCapabilities::GetSupportedMCS_NSSL32 (void) const
{
  uint32_t val = 0;
  return val;
}

void
S1gCapabilities::SetS1gCapabilitiesInfoL64 (uint64_t info)
{
  m_staType = (info >> 38) & 0x03;
  m_ChannelWidth = (info >> 6) & 0x03;
  m_ndpPsPollSupported = (info >> 50) & 0x01;
  m_pageSlicingImplemented = (info >> 52) & 0x01;
}
    
void
S1gCapabilities::SetS1gCapabilitiesInfoH16 (uint16_t info)
{

}
    
void
S1gCapabilities::SetSupportedMCS_NSSH8 (uint8_t MCS_NSS_L)
{
}

void
S1gCapabilities::SetSupportedMCS_NSSL32 (uint32_t MCS_NSS_H)
{
}


void
S1gCapabilities::SerializeInformationField (Buffer::Iterator start) const
{
  if (m_s1gSupported == 1)
    {
      //write the corresponding value for each bit
      start.WriteHtolsbU16 (GetS1gCapabilitiesInfoH16 ());
      start.WriteHtolsbU64 (GetS1gCapabilitiesInfoL64 ());

      start.WriteU8 (GetSupportedMCS_NSSH8 ());
      start.WriteHtolsbU32 (GetSupportedMCS_NSSL32 ());
    }
}

uint8_t
S1gCapabilities::DeserializeInformationField (Buffer::Iterator start,
                                             uint8_t length)
{
  Buffer::Iterator i = start;
    //NS_LOG_UNCOND ("WifiInformationElement::DeserializeIfPresent 79" << length); //for test
  
  uint16_t s1ginfoH16 = i.ReadLsbtohU16 ();
  uint64_t s1ginfoL64 = i.ReadLsbtohU64 ();
  uint8_t MCSNSSH8 = i.ReadU8 ();
  uint32_t MCSNSSL32 = i.ReadLsbtohU32 ();
    
  SetS1gCapabilitiesInfoH16 (s1ginfoH16);
  SetS1gCapabilitiesInfoL64 (s1ginfoL64);
    
  SetSupportedMCS_NSSH8 (MCSNSSH8);
  SetSupportedMCS_NSSL32 (MCSNSSL32);
    
  return length;
}

ATTRIBUTE_HELPER_CPP (S1gCapabilities);

std::ostream &
operator << (std::ostream &os, const S1gCapabilities &s1gcapabilities)
{
  os <<  bool (s1gcapabilities.GetStaType ());

  return os;
}

std::istream &operator >> (std::istream &is,S1gCapabilities &s1gcapabilities)
{
  bool c1;
  is >> c1;
  s1gcapabilities.SetS1gSupported (c1);

  return is;
}

} //namespace ns3
