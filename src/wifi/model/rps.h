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

#ifndef RPS_H
#define RPS_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute.h"
#include "ns3/attribute-helper.h"
#include "ns3/wifi-information-element.h"
#include "ns3/vector.h"


namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 TIM Element
 *
 * \see attribute_Tim
 */
class RPS : public WifiInformationElement
{
public:
    
      class RawAssignment
       {
       public:
          RawAssignment ();
          ~RawAssignment ();
          
          void SetRawControl (uint8_t control);
          void SetSlotFormat (uint8_t format);
          void SetSlotCrossBoundary (uint8_t cross);
          void SetSlotDurationCount (uint16_t count);
          void SetSlotNum (uint16_t count);

          void SetSlot(uint16_t value);

          void SetRawStart (uint8_t start);
          void SetRawGroup (uint32_t group);
          void SetChannelInd (uint16_t channel);
          void SetPRAW (uint32_t praw);
          void SetRawTypeIndex(uint8_t val);
          
          uint8_t GetRawControl (void) const;
          uint16_t GetRawSlot (void) ;
          uint8_t GetSlotFormat (void) const;
          uint8_t GetSlotCrossBoundary (void) const;
          uint16_t GetSlotDurationCount (void) const;
          uint16_t GetSlotNum (void) const;
           
          uint8_t GetRawStart (void) const;
          uint32_t GetRawGroup (void) const;
          uint16_t GetChannelInd (void) const;
          uint32_t GetPRAW (void) const;
          
          uint8_t GetSize (void) const;

          uint8_t GetRawGroupPage() const;
          uint16_t GetRawGroupAIDStart() const;
          uint16_t GetRawGroupAIDEnd() const;
          uint8_t GetRawTypeIndex() const;
          //void Serialize (Buffer::Iterator start) const;
          //uint8_t Deserialize (Buffer::Iterator start);
          
      private:
          uint8_t m_rawcontrol;
          uint16_t m_rawslot;
          uint8_t m_SlotFormat;
          uint8_t m_slotCrossBoundary;
          uint16_t m_slotDurationCount;
          uint16_t m_slotNum;
          uint8_t m_rawTypeIndex;

          uint8_t m_rawstart;
          uint32_t m_rawgroup;
          uint16_t m_channelind;
          uint32_t m_prawparam;
          
          uint8_t raw_length; //!< length of (single) Raw Assignment
       };
    
  RPS ();
  ~RPS ();
 
   /**
   * Set the Partial Virtual Bitmap.
   *
   * \Set the Partial Virtual Bitmap
   */
  void SetRawAssignment (RPS::RawAssignment raw);
  /**
   * Return the Partial Virtual Bitmap.
   *
   * \Return the Partial Virtual Bitmap
   */
  uint8_t * GetRawAssignment (void) const;  //to do, use std::vector
  RPS::RawAssignment GetRawAssigmentObj(uint32_t index = 0) const;

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
    
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
  uint8_t GetNumberOfRawGroups (void) const;

  uint8_t m_length;
private:
  RPS::RawAssignment assignment; //!< RawAssignment subfield
  uint8_t * m_rps; //! Support up to 10 RAW Assignment subfield
  //uint8_t m_rpsarry[12];
    uint8_t m_rpsarry_temp[1200];
  std::vector<uint8_t> m_rpsarry;
  
  //uint8_t m_length; //!< Total length of all RAW Assignments
};

std::ostream &operator << (std::ostream &os, const RPS &rps);
std::istream &operator >> (std::istream &is, RPS &rps);

ATTRIBUTE_HELPER_HEADER (RPS);

//} //namespace ns3


    
class RPSVector
{
public:
    typedef std::vector<ns3::RPS *> RPSlist;
    RPSlist rpsset;
    
    uint32_t getlen();
    void setlen(uint8_t len);
    
    friend std::ostream &operator << (std::ostream &os, const RPSVector &rpsv);
    friend std::istream &operator >> (std::istream &is, RPSVector &rpsv);
    
private:
    uint32_t length;
};
    
std::ostream &operator << (std::ostream &os, const RPSVector &rpsv);
std::istream &operator >> (std::istream &is, RPSVector &rpsv);
    

ATTRIBUTE_HELPER_HEADER (RPSVector);

} //namespace ns3

#endif /* RPS_H */
