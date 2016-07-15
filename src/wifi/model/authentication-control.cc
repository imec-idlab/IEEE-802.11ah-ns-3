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

#include "authentication-control.h"
#include "ns3/assert.h"
#include "ns3/log.h" //for test

namespace ns3 {
    
NS_LOG_COMPONENT_DEFINE ("AuthenticationCtrl");
    

    
AuthenticationCtrl::AuthenticationCtrl ()
{
  m_length = 0;
  m_AuthenSupported = 1;
    ctrlThreshold = 250;
}

AuthenticationCtrl::~AuthenticationCtrl ()
{
}


void
AuthenticationCtrl::SetControlType (bool IsDistributed)
{
    ctrltype = IsDistributed;
    //"0"--centralized, "1"--distributed
}
    
void
AuthenticationCtrl::SetThreshold (uint16_t threshold)
{
    NS_ASSERT (!ctrltype);
    ctrlThreshold = threshold;
}
    
void
AuthenticationCtrl::SetSlotDuration (uint8_t duration)
{
    NS_ASSERT (ctrltype);
    slotDuration = duration;
}
    
void
AuthenticationCtrl::SetMaxInterval (uint8_t max)
{
    NS_ASSERT (ctrltype);
    maxInterval = max;
}
    
void
AuthenticationCtrl::SetMinInterval (uint8_t min)
{
    NS_ASSERT (ctrltype);
    minInterval = min;
}
    
    
bool
AuthenticationCtrl::GetControlType (void) const
{
    return ctrltype;
}
    
uint16_t
AuthenticationCtrl::GetThreshold (void) const
{
    NS_ASSERT (!ctrltype);
    return ctrlThreshold;
}
    
uint8_t
AuthenticationCtrl::GetSlotDuration (void) const
{
    NS_ASSERT (ctrltype);
    return slotDuration;
}
    
uint8_t
AuthenticationCtrl::GetMaxInterval (void) const
{
    NS_ASSERT (ctrltype);
    return maxInterval;
}
    
uint8_t
AuthenticationCtrl::GetMinInterval (void) const
{
    NS_ASSERT (ctrltype);
    return minInterval;
}


WifiInformationElementId
AuthenticationCtrl::ElementId () const
{
    return IE_AUTHENTICATION_CONTROL;
}
    
void
AuthenticationCtrl::SetAuthenSupported (bool authen)
{
   m_AuthenSupported = authen;

}

    
uint16_t
AuthenticationCtrl::GetSerializedSize () const
{
    if (!m_AuthenSupported)
        {
            return 0;
        }
    return WifiInformationElement::GetSerializedSize ();
}
    
uint8_t
AuthenticationCtrl::GetInformationFieldSize () const
{
    //we should not be here if  is not supported
    NS_ASSERT (m_AuthenSupported > 0);
    if (ctrltype)
     {
        return 2;
     }
    else
    {
       return 2;
    }
}

    
void
AuthenticationCtrl::SerializeInformationField (Buffer::Iterator start) const
{
  if (m_AuthenSupported)
    {
     if (ctrltype)
       {
        start.WriteU8 (slotDuration | 0x80);
        start.WriteU8 (maxInterval);
        start.WriteU8 (minInterval);
       }
     else
       {
        start.WriteU16 (ctrlThreshold);
       }
    }
}
    

    
uint8_t
AuthenticationCtrl::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{

    ctrlThreshold = start.ReadU16 ();

    if ((ctrlThreshold & 0x8000) == 0x0000)
      {
        ctrltype = false;
        ctrlThreshold = ctrlThreshold & 0x03ff;
      }
    else
     {
       ctrltype = true;
       slotDuration = (ctrlThreshold >> 8) & 0x007f;
       maxInterval = ctrlThreshold & 0x00ff;
       minInterval = start.ReadU8 ();
     }

    m_length = length;
    return length;
}

} //namespace ns3





