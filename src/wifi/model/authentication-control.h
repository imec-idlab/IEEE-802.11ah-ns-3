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

#ifndef AuthenticationCtrl_H
#define AuthenticationCtrl_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute-helper.h"
#include "ns3/wifi-information-element.h"


namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 TIM Element
 *
 * \see attribute_Tim
 */
class AuthenticationCtrl : public WifiInformationElement
{
public:
   AuthenticationCtrl ();
   ~AuthenticationCtrl ();
 

  void SetCentralizedParameter ();
  void SetDistributedParameter ();

  void  SetControlType (bool IsDistributed);
  void  SetThreshold (uint16_t threshold);
  void  SetSlotDuration (uint8_t duration);
  void  SetMaxInterval (uint8_t max);
  void  SetMinInterval (uint8_t min);
  
  bool  GetControlType (void) const;
  uint16_t  GetThreshold (void) const;
  uint8_t  GetSlotDuration (void) const;
  uint8_t  GetMaxInterval (void) const;
  uint8_t  GetMinInterval (void) const;
    
  void SetAuthenSupported (bool authen);


  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  uint16_t GetSerializedSize () const;

    
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
    

private:
  bool ctrltype;
  uint16_t ctrlThreshold;
  uint8_t slotDuration;
  uint8_t maxInterval;
  uint8_t minInterval;
  bool m_AuthenSupported;

  
  uint8_t m_length;
};


//ATTRIBUTE_HELPER_HEADER (RPS);

} //namespace ns3

#endif /* RPS_H */
