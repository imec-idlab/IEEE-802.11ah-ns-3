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

#ifndef S1G_BEACON_COMPATIBILITY_H
#define S1G_BEACON_COMPATIBILITY_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute-helper.h"
#include "ns3/wifi-information-element.h"
#include "ns3/capability-information.h"

namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 S1G Beacon Compatibility Information Element
 *
 * \see attribute_S1gBeaconCompatibility
 */
class S1gBeaconCompatibility : public WifiInformationElement
{
public:
  S1gBeaconCompatibility ();
  ~S1gBeaconCompatibility ();
 
  /**
   * Set the beacon interval.
   *
   * \Set the beacon interval
   */
  void SetBeaconInterval (uint64_t us);
  /**
   * Return the beacon interval.
   *
   * \return the beacon interval
   */
  uint64_t GetBeaconInterval (void) const;
  /**
   * Return the beacon interval.
   *
   * \return the beacon interval
   */
  uint32_t Gettsfcompletetion (void) const;

  WifiInformationElementId ElementId () const;  //<<<<<<< finished
  uint8_t GetInformationFieldSize () const;    // <<<<<<<  piece of cake
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

private:
  CapabilityInformation m_compatibility; //!< compatibility information, bit6 TSF RollOver flag is not supported
  uint64_t m_beaconinterval;  //!< beacon interval
  uint32_t m_tsfcompletetion; //!< beacon interval
  uint8_t m_length;   //!< Length of the S1gBeaconCompatibility
};

std::ostream &operator << (std::ostream &os, const S1gBeaconCompatibility &ssid);
std::istream &operator >> (std::istream &is, S1gBeaconCompatibility &ssid);

ATTRIBUTE_HELPER_HEADER (S1gBeaconCompatibility);

} //namespace ns3

#endif /* S1G_BEACON_COMPATIBILITY_H */
