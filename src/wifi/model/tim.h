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

#ifndef TIM_H
#define TIM_H

#include <stdint.h>
#include "ns3/buffer.h"
#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/wifi-information-element.h"


namespace ns3 {

/**
 * \ingroup wifi
 *
 * The IEEE 802.11 TIM Element
 *
 * \see attribute_Tim
 */
class TIM : public WifiInformationElement
{
public:
  TIM ();
  ~TIM ();


  enum BlockCoding
  {
    BLOCK_BITMAP = 0,
    SINGAL_AID = 1,
    // to do
  };
  class EncodedBlock
   {
      public:
        EncodedBlock ();
        ~EncodedBlock ();

        void SetBlockControl (enum BlockCoding coding);
        void SetBlockOffset (uint8_t offset);
        void SetEncodedInfo (uint8_t * encodedInfo, uint8_t length);
        void SetBlockBitmap (uint8_t offset);
      
        enum  TIM::BlockCoding GetBlockControl (void) const;
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
    

 
  /**
   * Set the DTIM Count.
   *
   * \Set the DTIM Count
   */
  void SetDTIMCount (uint8_t count);
  /**
   * Set the DTIM Period.
   *
   * \Set the DTIM Period
   */
  void SetDTIMPeriod (uint8_t count);
  /**
   * Set the Bitmap Control.
   *
   * \Set the Bitmap Control
   */
  void SetBitmapControl (uint8_t control);
  /**
   * Set the Partial Virtual Bitmap.
   *
   * \Set the Partial Virtual Bitmap
   */
  void SetPartialVBitmap (TIM::EncodedBlock block);
    
  /**
   * Return the TIM Count.
   *
   * \Return the TIM Count
   */
  uint8_t GetDTIMCount (void) const;
  /**
   * Return the DTIM Period.
   *
   * \Return the DTIM Period
   */
  uint8_t GetDTIMPeriod (void) const;
  /**
   * Return the Bitmap Control.
   *
   * \Return the Bitmap Control
   */
  uint8_t GetBitmapControl (void) const;
  /**
   * Return the Partial Virtual Bitmap.
   *
   * \Return the Partial Virtual Bitmap
   */
  uint8_t * GetPartialVBitmap (void) const;
  
   void SetTafficIndicator (uint8_t control);
   void SetPageSliceNum (uint8_t control);
   void SetPageIndex (uint8_t control);
   
   uint8_t GetTrafficIndicator () const;
   uint8_t GetPageSliceNum () const;
   uint8_t GetPageIndex () const;

    

  WifiInformationElementId ElementId () const;
  uint8_t GetInformationFieldSize () const;
  void SerializeInformationField (Buffer::Iterator start) const;
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);
  
   uint8_t m_length; //!< length of partial Virtual Bitmap field

private:
  uint8_t m_DTIMCount; //!< DTIM Count
  uint8_t m_DTIMPeriod; //!< DTIM Period is uint32 (0-255)
  uint8_t m_BitmapControl; //!< Bitmap Control
  uint8_t m_TrafficIndicator;
  uint8_t m_PageSliceNum;
  uint8_t m_PageIndex;
  TIM::EncodedBlock m_encodeblock; //!< encoded block subfield of partial Virtual Bitmap field
  uint8_t m_partialVBitmap_arrary[251]; // see 9.4.2.6.1
  uint8_t * m_partialVBitmap;

  uint8_t * subblock; 
};

std::ostream &operator << (std::ostream &os, const TIM &pageS);
std::istream &operator >> (std::istream &is, TIM &pageS);
ATTRIBUTE_HELPER_HEADER (TIM);
} //namespace ns3

#endif /* TIM_H */
