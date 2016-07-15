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

#include "rps.h"
#include "ns3/assert.h"
#include "ns3/log.h" //for test
#include <sstream>

namespace ns3 {
    
NS_LOG_COMPONENT_DEFINE ("RPS");
    
RPS::RawAssignment::RawAssignment ()
{
    m_SlotFormat = 1;
}
    
RPS::RawAssignment::~RawAssignment ()
{
}

void
RPS::RawAssignment::SetRawControl (uint8_t control)
{
  m_rawcontrol = control;
}

void
RPS::RawAssignment::SetSlotFormat (uint8_t format)
{
    NS_ASSERT (format <= 1);
    m_SlotFormat = format;
    //printf("SetSlotFormat is %u\n",  m_SlotFormat);

}
    
void
RPS::RawAssignment::SetSlotCrossBoundary (uint8_t cross)
{
    NS_ASSERT (cross <= 1);
    m_slotCrossBoundary = cross;
}
    
void
RPS::RawAssignment::SetSlotDurationCount (uint16_t count)
{
    NS_ASSERT((!m_SlotFormat & (count < 256)) || (m_SlotFormat & (count < 2048)));
    m_slotDurationCount = count;
}
    
void
RPS::RawAssignment::SetSlotNum (uint16_t count)
{
   NS_ASSERT((!m_SlotFormat & (count < 64)) || (m_SlotFormat & (count < 8)));
   m_slotNum = count;
    
}
  
void
RPS::RawAssignment::SetRawStart (uint8_t start)
{
  m_rawstart = start;
}
    
void
RPS::RawAssignment::SetRawGroup (uint32_t group)
{
  m_rawgroup = group;
//NS_LOG_UNCOND ("SetRawGroup =" << m_rawgroup);
    //NS_LOG_UNCOND (" set m_rawgroup address=" << &m_rawgroup);
}
   
void
RPS::RawAssignment::SetChannelInd (uint16_t channel)
{
  m_channelind = channel;
}
    
void
RPS::RawAssignment::SetPRAW (uint32_t praw)
{
  m_prawparam = praw;
}

uint8_t
RPS::RawAssignment::GetRawControl (void) const
{
  return m_rawcontrol;
}

uint8_t
RPS::RawAssignment::GetSlotFormat (void) const
{
   return m_SlotFormat;
}
    
uint8_t
RPS::RawAssignment::GetSlotCrossBoundary (void) const
{
   return  m_slotCrossBoundary;
}
    
uint16_t
RPS::RawAssignment::GetSlotDurationCount (void) const
{
   return m_slotDurationCount;
}
    
uint16_t
RPS::RawAssignment::GetSlotNum (void) const
{
   return m_slotNum;
}
    
uint16_t
RPS::RawAssignment::GetRawSlot (void)
{
    //printf("GetSlotFormat is %u\n",  m_SlotFormat);

    NS_ASSERT (m_SlotFormat <= 1);

  if (m_SlotFormat == 0)
    {
      m_rawslot =  (uint16_t(m_slotCrossBoundary) << 14) | (m_slotDurationCount << 6) | m_slotNum;
    }
  else
   {
      m_rawslot = 0x8000 | (uint16_t(m_slotCrossBoundary) << 14) | (m_slotDurationCount << 3) | m_slotNum;
   }
  return m_rawslot;
}

uint8_t
RPS::RawAssignment::GetRawStart (void) const
{
  return m_rawstart;
}

uint32_t
RPS::RawAssignment::GetRawGroup (void) const
{
    //NS_LOG_UNCOND (" return m_rawgroup =" << m_rawgroup);
    //NS_LOG_UNCOND (" return m_rawgroup address=" << &m_rawgroup);

  return m_rawgroup;
}

uint16_t
RPS::RawAssignment::GetChannelInd (void) const
{
  return m_channelind;
}

uint32_t
RPS::RawAssignment::GetPRAW (void) const
{
  return m_prawparam;
}
    
uint8_t
RPS::RawAssignment::GetSize (void) const
{
  //return raw_length;
  return 12;
}

    
RPS::RPS ()
{
  m_length = 0;
}

RPS::~RPS ()
{
}

//suppose all subfield of RAW Assignment are presented, 12 octets
// change in future
    
    
    
void
RPS::SetRawAssignment (RPS::RawAssignment raw)
{
    //m_length = 0;
     //uint8_t m_rpsarry[12]; //! Support up to 10 RAW Assignment subfield
    //uint8_t len = assignment.GetSize ();
    m_rpsarry.push_back(raw.GetRawControl ());
    m_length++;
    m_rpsarry.push_back((uint8_t)raw.GetRawSlot ());
    m_length++;
    m_rpsarry.push_back((uint8_t)(raw.GetRawSlot () >> 8));
    m_length++;
    m_rpsarry.push_back(raw.GetRawStart ());
    m_length++;
    m_rpsarry.push_back((uint8_t)(raw.GetRawGroup ()));//(7-0)
    m_length++;
    m_rpsarry.push_back((uint8_t)(raw.GetRawGroup () >> 8));//(15-8)
    m_length++;
    m_rpsarry.push_back( (uint8_t)(raw.GetRawGroup () >> 16));//(23-16)
    m_length++;
    m_rpsarry.push_back((uint8_t)raw.GetChannelInd ());
    m_length++;
    m_rpsarry.push_back((uint8_t)(raw.GetChannelInd () >> 16));
    m_length++;
    m_rpsarry.push_back((uint8_t)raw.GetPRAW ());
    m_length++;
    m_rpsarry.push_back((uint8_t)(raw.GetPRAW () >> 8));
    m_length++;
    m_rpsarry.push_back( (uint8_t)(raw.GetPRAW () >> 16));
    m_length++;
    
    m_rps = &m_rpsarry[0];
    //printf (" set m_rps %x\n" , m_rps[6]);

}
    
uint8_t *
RPS::GetRawAssignment (void) const
{
    //printf (" get m_rps addressx\n" , m_rps);
    //printf (" get m_rps %x\n" , m_rps[6]);
    return m_rps;
}
    
WifiInformationElementId
RPS::ElementId () const
{
  return IE_RPS;
}
    
uint8_t
RPS::GetInformationFieldSize () const
{
  return m_length;
}

void
RPS::SerializeInformationField (Buffer::Iterator start) const
{
  start.Write (m_rps, m_length);
}
    
uint8_t
RPS::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  //static uint8_t m_rpsarry[12];
  start.Read (m_rpsarry_temp, length);
  m_rps = m_rpsarry_temp;
  m_length = length;
  return length;
}

ATTRIBUTE_HELPER_CPP (RPS);
    
std::ostream &
operator << (std::ostream &os, const RPS &rps)
    {
        os <<  "|" ;
        return os;
    }
    /**
     * \brief Stream extraction operator.
     *
     * \param is the stream
     * \param rectangle the rectangle
     * \returns a reference to the stream
     */
    std::istream &
    operator >> (std::istream &is, RPS &rps)
    {
        is >> rps.m_length;
        return is;
    }
    
    ////////////
    
    
    uint32_t RPSVector::getlen ()
    {
        return length;
    }
    
    ATTRIBUTE_HELPER_CPP (RPSVector);
    
    std::ostream &
    operator << (std::ostream &os, const RPSVector &rpsv)
    {
        os <<  "|" ;
        return os;
    }
    
    std::istream &
    operator >> (std::istream &is, RPSVector &rpsv)
    {
        is >> rpsv.length;
        return is;
    }

    
} //namespace ns3





