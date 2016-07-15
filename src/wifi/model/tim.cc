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
TIM::EncodedBlock::SetEncodedInfo (uint8_t * encodedInfo, uint8_t subblocklength)
{
  //NS_ASSERT (1 <= subblocklength <= 8);
  uint8_t * blockbitmap = encodedInfo;
  uint8_t i = 0;
  uint8_t len = 0;
  uint8_t info = *blockbitmap;
  while (i <= 7)
  {
    if (Is (info, i))
      {
       len++;
      }
    i++;
  }
  NS_ASSERT (len == subblocklength);
  blockbitmap++;
  m_subblock = blockbitmap;
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

//to be implemented, should divided into traffic indicator, page slice and page index
void
TIM::SetBitmapControl (uint8_t control)
{
  //to be implemented
  m_BitmapControl = control;
}
    
void
TIM::SetPartialVBitmap (TIM::EncodedBlock block)
{
  m_encodeblock = block;
  m_length = 0;
  uint8_t offset = m_encodeblock.GetBlockOffset ();
  uint8_t control = m_encodeblock.GetBlockBitmap ();
  uint8_t offcont = ((offset << 3) & 0xf8) | ((control << 0) & 0x07);
  m_partialVBitmap[m_length] = offcont;
  m_length++;
  m_partialVBitmap[m_length] = m_encodeblock.GetBlockBitmap ();
  m_length++;

  uint8_t * subblock = m_encodeblock.GetSubblock ();
  uint8_t len = m_encodeblock.GetSize ();  //size of EncodedBlock
  uint8_t i=0;
  while (i < len-2) //blockcotrol, blockoffset has already been added into m_partialVBitmap
  {
    m_partialVBitmap[m_length] = *subblock;
    m_length++;
    subblock++;
    i++;
  }
  //NS_ASSERT ( 1 <= m_length <= 251);
}
    
uint8_t
TIM::GetTIMCount (void) const
{
  return m_DTIMCount;
}

uint8_t
TIM::GetDTIMPeriod (void) const
{
  return m_DTIMPeriod;
}

uint8_t
TIM::GetBitmapControl (void) const
{
  return m_BitmapControl;
}

unsigned char *
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
  return (m_length + 3);
}

void
TIM::SerializeInformationField (Buffer::Iterator start) const
{
 start.WriteU8 (m_DTIMCount);
 start.WriteU8 (m_DTIMPeriod);
 start.WriteU8 (m_BitmapControl);
 start.Write (m_partialVBitmap, m_length);
}

uint8_t
TIM::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  m_DTIMCount = start.ReadU8 ();
  m_DTIMPeriod = start.ReadU8 ();
  m_BitmapControl = start.ReadU8 ();
  start.Read (m_partialVBitmap, (length-3));
    m_length = length-3;
  return length;
}

//ATTRIBUTE_HELPER_CPP (TIM);

} //namespace ns3





