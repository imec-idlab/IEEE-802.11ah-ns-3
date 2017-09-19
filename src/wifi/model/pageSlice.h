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
#include "ns3/wifi-information-element.h"


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

  /*
  enum BlockCoding
  {
    BLOCK_BITMAP = 0,
    SINGAL_AID = 1,
    // to do
  }; */
    /*
  class EncodedBlock
   {
      public:
        EncodedBlock ();
        ~EncodedBlock ();

        void SetBlockControl (enum BlockCoding coding);
        void SetBlockOffset (uint8_t offset);
        void SetEncodedInfo (uint8_t * encodedInfo, uint8_t length);
      
        enum  PAGESLICE::BlockCoding GetBlockControl (void) const;
        uint8_t GetBlockOffset (void) const;
        uint8_t GetBlockBitmap (void) const;
        uint8_t * GetSubblock (void) const;
      
        uint8_t GetSize (void) const;
        //void Serialize (Buffer::Iterator start) const;
        //uint8_t Deserialize (Buffer::Iterator start);
      
      private:
        bool Is (uint8_t info, uint8_t n);
      
        uint8_t m_blockcontrol;
        uint8_t m_blockoffset;
        uint8_t m_blockbitmap;
        uint8_t * m_subblock;
        uint8_t subb_length; //!< length of Subblock field
     };
    */

 

  void SetPagePeriod (uint8_t Period);
  void SetPageSliceControl (uint32_t control);
  void SetPageBitmap (uint32_t control);
    
  uint8_t GetPagePeriod (void) const;
  uint32_t GetPageSliceControl (void) const;
  uint32_t GetPageBitmap (void) const;


  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
    
  virtual void Print (std::ostream &os) const;
        

private:
  uint8_t m_PagePeriod; //!< DPAGESLICE Count
  uint8_t m_PageSliceControl; //!< DPAGESLICE Period
  uint8_t m_PageBitmap //!< Bitmap Control

  // slightly simplified representation of the vbitmap
  //uint32_t m_partialVBitmap;

  /*PAGESLICE::EncodedBlock m_encodeblock; //!< encoded block subfield of partial Virtual Bitmap field
  uint8_t * m_partialVBitmap;
  uint8_t m_length; //!< length of partial Virtual Bitmap field*/
};


//ATTRIBUTE_HELPER_HEADER (PAGESLICE);

} //namespace ns3

#endif /* PAGESLICE_H */
