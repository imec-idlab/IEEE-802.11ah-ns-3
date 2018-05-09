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

#include "tim.h"
#include "ns3/assert.h"
#include "ns3/log.h" //for test

namespace ns3 {
    
	NS_LOG_COMPONENT_DEFINE ("TIM");

TIM::EncodedBlock::EncodedBlock ()
{
}

TIM::EncodedBlock::~EncodedBlock ()
{
}

void
TIM::EncodedBlock::SetBlockControl (enum BlockCoding coding)
{
  m_blockcontrol = 0; // only support Block Bitmap
}

void
TIM::EncodedBlock::SetBlockOffset (uint8_t offset)
{
  NS_ASSERT (offset <= 31);
  m_blockoffset = offset;
}

void
TIM::EncodedBlock::SetBlockBitmap (uint8_t encodedInfo)
{
  m_blockbitmap = encodedInfo;
}

void
TIM::EncodedBlock::SetEncodedInfo (uint8_t * encodedInfo, uint8_t subblocklength)
{
  //NS_ASSERT (1 <= subblocklength <= 8);
  uint8_t i = 0;
  uint8_t len = 0;
  while (i <= 7)
  {
    if (Is (m_blockbitmap, i))
      {
       len++;
      }
    i++;
  }
  NS_ASSERT (len == subblocklength);
  m_subblock = encodedInfo;
  subb_length = subblocklength;
}

enum TIM::BlockCoding
TIM::EncodedBlock::GetBlockControl (void) const
{
switch (m_blockcontrol)
  {
  case 0:
    return BLOCK_BITMAP;
    break;
  case 1:
  default:
    return BLOCK_BITMAP;
    break;
  }
}

uint8_t
TIM::EncodedBlock::GetBlockOffset (void) const
{
  NS_ASSERT (m_blockoffset <= 31);
  return m_blockoffset;
}

uint8_t
TIM::EncodedBlock::GetBlockBitmap (void) const
{
  return m_blockbitmap;
}

uint8_t *
TIM::EncodedBlock::GetSubblock (void) const
{
  return m_subblock;
}

uint8_t
TIM::EncodedBlock::GetSize (void) const
{
  return (subb_length + 2);
}

/*
void
TIM::EncodedBlock::Serialize (Buffer::Iterator start) const
{
  NS_ASSERT ( 1 <= subb_length <= 8);
  uint8_t m_block = ((m_blockoffset << 3) & 0xf8) | ((m_blockcontrol << 0) & 0x07);
  start.WriteU8 (m_block);
  start.WriteU8 (m_blockbitmap);
  start.Write (subblock, subb_length);
}

uint8_t
TIM::EncodedBlock::Deserialize (Buffer::Iterator start)
{
  NS_ASSERT ( 1 <= subb_length <= 8);
  uint8_t m_block = start.ReadU8 ();
  m_blockcontrol = (m_block >> 0) & 0x07
  m_blockoffset = (m_block >> 3) & 0x1f;
  m_blockbitmap = start.ReadU8 ();
  start.Read (m_subblock, subb_length);
  return (subb_length + 2);
}
*/

bool
TIM::EncodedBlock::Is (uint8_t info, uint8_t n)
{
  uint8_t mask = 1 << n;
  return (info & mask) == mask;
}

TIM::TIM ()
{
  m_length = 0;
}

TIM::~TIM ()
{
}

void
TIM::SetDTIMCount (uint8_t count)
{
  m_DTIMCount = count;
}
    
void
TIM::SetDTIMPeriod (uint8_t count)
{
  m_DTIMPeriod = count;
}

void
TIM::SetBitmapControl (uint8_t control)
{
  this->SetTafficIndicator(control & 0x01);
  this->SetPageSliceNum((control >> 1) & 0x1f);
  this->SetPageIndex((control >> 6) & 0x03);
  m_BitmapControl = control;
}


void 
TIM::SetTafficIndicator (uint8_t control)
{
   NS_ASSERT (control <= 1);
   m_TrafficIndicator = control;
}

void
TIM::SetPageSliceNum (uint8_t control)
{
   NS_ASSERT (control <= 31);
   m_PageSliceNum = control; 
}

void 
TIM::SetPageIndex (uint8_t control)
{
   NS_ASSERT (control <= 3);
   m_PageIndex = control;
}
  
void
TIM::SetPartialVBitmap (TIM::EncodedBlock block)
{
  m_encodeblock = block;
  uint8_t offset = m_encodeblock.GetBlockOffset ();
  uint8_t control = m_encodeblock.GetBlockControl ();

  uint8_t offcont = ((offset << 3) & 0xf8) | (control & 0x07);
  m_partialVBitmap_arrary[m_length] = offcont;
  m_length++;
  
  m_partialVBitmap_arrary[m_length] = m_encodeblock.GetBlockBitmap ();
  m_length++;
  NS_LOG_DEBUG ("Block Bitmap = " << (int)m_encodeblock.GetBlockBitmap ());

  subblock = m_encodeblock.GetSubblock ();
  uint8_t len = m_encodeblock.GetSize ();  //size of EncodedBlock
  uint8_t i=0;
  while (i < len-2) //blockcotrol, blockoffset has already been added into m_partialVBitmap
  {
	NS_LOG_DEBUG ("Subblock " << (int)i << " = " << (int)(*subblock));

    m_partialVBitmap_arrary[m_length] = *subblock;
    m_length++;
    subblock++;
    i++;
  }
  m_partialVBitmap = m_partialVBitmap_arrary;
  NS_ASSERT ( m_length < 252);
}


uint8_t
TIM::GetDTIMCount (void) const
{
  return m_DTIMCount;
}

uint8_t
TIM::GetDTIMPeriod (void) const
{
  return m_DTIMPeriod;
}
/*
uint8_t
TIM::GetBitmapControl (void) const
{
  return m_BitmapControl;
} */

uint8_t
TIM::GetTrafficIndicator  (void) const
{
    return m_TrafficIndicator; 
}
uint8_t
TIM::GetPageSliceNum  (void) const
{
    return m_PageSliceNum; 
}

uint8_t
TIM::GetPageIndex(void) const
{
    return m_PageIndex;
}

uint8_t *
TIM::GetPartialVBitmap (void) const
{
  return m_partialVBitmap;
}

WifiInformationElementId
TIM::ElementId () const
{
  return IE_TIM;
}

uint8_t
TIM::GetInformationFieldSize () const
{
  if (!m_BitmapControl && !m_length)
	return 2;
  else
    return (m_length + 3);
}

void
TIM::SerializeInformationField (Buffer::Iterator start) const
{
 NS_LOG_FUNCTION (this);
 start.WriteU8 (m_DTIMCount);
 start.WriteU8 (m_DTIMPeriod);

 if (m_BitmapControl || m_length != 0)
   {
     start.WriteU8 (m_BitmapControl);
     start.Write (m_partialVBitmap, m_length);
     NS_LOG_DEBUG ("Bitmap Control field is " << (int)m_BitmapControl);
     NS_LOG_DEBUG ("Length of Partial Virtual Bitmap is " << (int)m_length);
   }
}

uint8_t
TIM::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  NS_LOG_FUNCTION (this << length);
  m_DTIMCount = start.ReadU8 ();
  m_DTIMPeriod = start.ReadU8 ();
  if (length > 2)
    {
	  SetBitmapControl (start.ReadU8 ());
	  start.Read (m_partialVBitmap_arrary, (length-3));
	  m_length = length-3;
	  m_partialVBitmap = m_partialVBitmap_arrary;

    }
  else
    {
	  SetBitmapControl (0);
	  m_length = 0;
    }
  return length;
}

ATTRIBUTE_HELPER_CPP (TIM);

   std::ostream &
    operator << (std::ostream &os, const TIM &rpsv)
    {
        os <<  "|" ;
        return os;
    }


    std::istream &
    operator >> (std::istream &is, TIM &rpsv)
    {
        is >> rpsv.m_length;
        return is;
    }

} //namespace ns3





