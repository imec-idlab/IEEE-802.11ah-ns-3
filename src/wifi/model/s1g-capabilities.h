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
 * Author: Ghada Badawy <gbadawy@gmail.com>
 */

#ifndef S1g_CAPABILITIES_H
#define S1g_CAPABILITIES_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute-helper.h"
#include "ns3/wifi-information-element.h"

/**
 * This defines the maximum number of supported MCSs that a STA is
 * allowed to have. Currently this number is set for IEEE 802.11n
 */
#define MAX_SUPPORTED_MCS  (77)  //need to be reset for IEEE 802.11ah

namespace ns3 {

/**
 * \brief The S1g Capabilities Information Element
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the S1g Capabilities Information Element
 *
 * \see attribute_S1gCapabilities
 */
class S1gCapabilities : public WifiInformationElement
{
public:
  S1gCapabilities ();
  void SetS1gSupported (uint8_t s1gsupported);
  void SetStaType (uint8_t type);
  void SetChannelWidth (uint8_t width);
  void SetPageSlicingSupport (uint8_t pageSlicingImplemented);
  void SetNdpPsPollSupport (uint8_t psPollingSupported);
  //Set the lowest 64bytes of the S1gCapabilitiesInfo in the S1G Capabilities information element
  void SetS1gCapabilitiesInfoL64 (uint64_t info);
  //Set the higest 16bytes of the S1gCapabilitiesInfo in the S1G Capabilities information element
  void SetS1gCapabilitiesInfoH16 (uint16_t info);
  //Set higest 32 bytes of Supported MCS and NSS set
  void SetSupportedMCS_NSSL32 (uint32_t MCS_NSS_L);
  //Set lowest 8 bytes of Supported MCS and NSS set
  void SetSupportedMCS_NSSH8 (uint8_t MCS_NSS_H);

  uint8_t GetPageSlicingSupport (void) const;
  uint8_t GetNdpPsPollSupport (void) const;
  uint8_t GetStaType (void) const;
  uint8_t GetChannelWidth (void) const;
  //Return the lowest 64bytes of the S1gCapabilitiesInfo in the S1G Capabilities information element
  uint64_t GetS1gCapabilitiesInfoL64 (void) const;
  //Return the highest 16bytes of the S1gCapabilitiesInfo in the S1G Capabilities information element
  uint16_t GetS1gCapabilitiesInfoH16 (void) const;
  //Return higest 32 bytes of Supported MCS and NSS set
  uint32_t GetSupportedMCS_NSSL32 (void) const;
  //Return lowest 8 bytes of Supported MCS and NSS set
  uint8_t GetSupportedMCS_NSSH8 (void) const;

    
  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start,
                                       uint8_t length);
  /**
   * This information element is a bit special in that it is only
   * included if the STA is an S1G STA. To support this we
   * override the Serialize and GetSerializedSize methods of
   * WifiInformationElement.
   *
   * \param start
   *
   * \return an iterator
   */
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  /**
   * Return the serialized size of this HT capability
   * information element.
   *
   * \return the serialized size of this HT capability information element
   */
  uint16_t GetSerializedSize () const;


private:
  uint8_t m_staType;
  //this is used to decide if this element should be added to the frame or not
  uint8_t m_s1gSupported;
  uint8_t m_ChannelWidth;
  uint8_t m_pageSlicingImplemented;
  uint8_t m_ndpPsPollSupported;
};

std::ostream &operator << (std::ostream &os, const S1gCapabilities &s1gcapabilities);
std::istream &operator >> (std::istream &is, S1gCapabilities &s1gcapabilities);

ATTRIBUTE_HELPER_HEADER (S1gCapabilities);

} //namespace ns3

#endif /* S1g_CAPABILITY_H */
