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

#ifndef PAGESLICE_H
#define PAGESLICE_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/wifi-information-element.h"
#include "ns3/vector.h"


namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 PAGESLICE Element
 *
 * \see attribute_PAGESLICE
 */
class pageSlice : public WifiInformationElement
{
public:
  pageSlice ();
  ~pageSlice ();


  void SetPagePeriod (uint8_t Period);
  //void SetPageSliceControl (uint32_t control);
  void SetPageBitmap (uint32_t control);
  
  void SetPageindex (uint8_t count);
  void SetPageSliceLen (uint8_t count);
  void SetPageSliceCount (uint8_t count);
  void SetBlockOffset (uint8_t count);
  void SetTIMOffset (uint8_t count);

    
  uint8_t GetPagePeriod (void) const;
  //uint32_t GetPageSliceControl (void) const;
  uint8_t * GetPageBitmap (void) const;
  

  uint8_t GetPageindex (void) const;
  uint8_t GetPageSliceLen (void) const;
  uint8_t GetPageSliceCount (void) const;
  uint8_t GetBlockOffset (void) const;
  uint8_t GetTIMOffset (void) const;
  uint8_t GetPageBitmapLength (void) const;


  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
    
  //void Print (std::ostream &os) const;
  uint8_t m_length; //!< length of m_PageBitmap field*/     

private:
  uint8_t m_ElementId;
  uint8_t m_PagePeriod; //!< DPAGESLICE Count
  uint32_t m_PageSliceControl; //!< DPAGESLICE Period
  uint8_t m_Pageindex;
  uint8_t m_PageSliceLen;
  uint8_t m_PageSliceCount;
  uint8_t m_BlockOffset;
  uint8_t m_TIMOffset;
  
  uint8_t m_PageBitmaparry[4];
  uint8_t * m_PageBitmap;
  
};

std::ostream &operator << (std::ostream &os, const pageSlice &pageS);
std::istream &operator >> (std::istream &is, pageSlice &pageS);
ATTRIBUTE_HELPER_HEADER (pageSlice);
} //namespace ns3

#endif /* PAGESLICE_H */
