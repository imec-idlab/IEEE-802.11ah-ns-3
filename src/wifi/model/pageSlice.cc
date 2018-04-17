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
#include "ns3/attribute.h"
#include "pageSlice.h" 


namespace ns3 {
    
NS_LOG_COMPONENT_DEFINE ("pageSlice");
    
pageSlice::pageSlice () :  m_ElementId (209)
{
	for (auto i=0; i < 4; i++)
		m_PageBitmaparry[i] = 0;
}

pageSlice::~pageSlice ()
{
}

void
pageSlice::SetPagePeriod (uint8_t count)
{
  m_PagePeriod = count;
}

/*
void
pageSlice::SetPageSliceControl (uint32_t count)
{
  m_PageSliceControl = count;
} */

void
pageSlice::SetPageindex (uint8_t count)
{
  NS_ASSERT ((count & 0x03) < 4);
  m_Pageindex = count;
}

void
pageSlice::SetPageSliceLen (uint8_t count)
{
  m_PageSliceLen = count;
}

void
pageSlice::SetPageSliceCount (uint8_t count)
{
  m_PageSliceCount = count;
}

void
pageSlice::SetBlockOffset (uint8_t count)
{
  m_BlockOffset = count;
}

void
pageSlice::SetTIMOffset (uint8_t count)
{
  m_TIMOffset = count;
}

//to be implemented, should divided into traffic indicator, page slice and page index
void
pageSlice::SetPageBitmap (uint32_t control)
{  
 if ( (control & 0xff000000) > 0)
  {
     m_length = 4;
  }
 else if ( (control & 0x00ff0000) > 0)
  {
     m_length = 3;
  }
 else if ( (control & 0x0000ff00) > 0)
  {
     m_length = 2;
  }
 else if ((control & 0x000000ff) > 0)
  {
     m_length = 1;
  }
 else
  {
     m_length = 0; 
  }    
 
  for (uint8_t i=0; i < m_length; i++ )
   {
      m_PageBitmaparry[i] = (control >> i*8) & 0x000000ff ;
   }
 
  m_PageSliceControl = this->GetPageindex()  | ((m_PageSliceLen & 0x1f ) << 2 )| ( (m_PageSliceCount & 0x1f) << 7 )
         | ((m_BlockOffset & 0x1f )<< 12 ) | ( (m_TIMOffset & 0x0f) << 17 );
  
  m_PageBitmap = m_PageBitmaparry;
  //printf("     ---pageSlice::SetPageBitmap -- m_PageBitmap = %x\n", *m_PageBitmap);
  //printf("     ---pageSlice::SetPageBitmap -- m_length = %d\n", m_length);

}

uint8_t
pageSlice::GetPagePeriod (void) const
{
  return m_PagePeriod;
}

/*
uint32_t
pageSlice::GetPageSliceControl (void) const
{
  return m_PageSliceControl;
} */

uint8_t
pageSlice::GetPageindex (void) const
{
  return m_Pageindex;
}

uint8_t
pageSlice::GetPageSliceLen (void) const
{
  return m_PageSliceLen;
}

uint8_t
pageSlice::GetPageSliceCount (void) const
{
  return m_PageSliceCount;
}

uint8_t
pageSlice::GetBlockOffset (void) const
{
  return m_BlockOffset;
}

uint8_t
pageSlice::GetTIMOffset (void) const
{
  return m_TIMOffset;
}

uint8_t *
pageSlice::GetPageBitmap (void) const
{
  return m_PageBitmap;
}

uint8_t
pageSlice::GetPageBitmapLength (void) const
{
	//NS_ASSERT (m_length < 5);
	return m_length;
}


WifiInformationElementId
pageSlice::ElementId () const
{
  return IE_PAGE_SLICE;
}

uint8_t
pageSlice::GetInformationFieldSize () const
{
	  NS_ASSERT (m_length < 5);
  return (m_length + 4);
}

void
pageSlice::SerializeInformationField (Buffer::Iterator start) const
{
 start.WriteU8 (m_PagePeriod);
 start.WriteU8 ((uint8_t) m_PageSliceControl); //0-7
 start.WriteU8 ((uint8_t) (m_PageSliceControl >> 8)); //8-15
 start.WriteU8 ((uint8_t) (m_PageSliceControl >> 16)); // 16-23
 //start.WriteU32 (m_PageBitmap);
 //start.Write (m_PageBitmapP, m_length);
 NS_ASSERT (m_length < 5);
 for (uint8_t i=0; i < m_length; i++ )
   {
      start.Write (&m_PageBitmaparry[i], m_length);
   }
 }

uint8_t
pageSlice::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  uint8_t  m_PageSliceControl_l;
  uint8_t  m_PageSliceControl_m;
  uint8_t  m_PageSliceControl_h;
  
 // NS_LOG_UNCOND ("DeserializeInformationField length = " << length);
  
  m_PagePeriod = start.ReadU8 ();
  m_PageSliceControl_l = start.ReadU8 ();
  m_PageSliceControl_m = start.ReadU8 ();
  m_PageSliceControl_h = start.ReadU8 ();
  start.Read (m_PageBitmaparry, length - 4);
  m_PageBitmap = m_PageBitmaparry;
  
  m_PageSliceControl = (uint32_t)m_PageSliceControl_l | (uint32_t)m_PageSliceControl_m << 8 | (uint32_t) m_PageSliceControl_h << 16;

   this->SetPageindex (m_PageSliceControl & 0x00000003);
   m_PageSliceLen = (m_PageSliceControl >> 2) & 0x0000001f;
   m_PageSliceCount = (m_PageSliceControl >> 7) & 0x0000001f;
   m_BlockOffset = (m_PageSliceControl >> 12) & 0x0000001f;
   m_TIMOffset = (m_PageSliceControl >> 17) & 0x0000000f;
   m_length = length - 4;
   NS_ASSERT (m_length < 5);
  return length;
}

/*
void 
pageSlice::Print(std::ostream& os) const {
    os << "DTIM Count: " << std::to_string(m_PagePeriod) << std::endl;
    os << "DTIM Period: " << std::to_string(m_PageSliceControl) << std::endl;
    os << "Bitmap Control: " << std::to_string(m_PageBitmap) << std::endl;
    
    
    os << "Partial VBitmap" << "):";

    os << std::endl;
} */

   ATTRIBUTE_HELPER_CPP (pageSlice);

std::ostream &
operator << (std::ostream &os, const pageSlice &rpsv)
  {
        os <<  "|" ;
        return os;
  }

std::istream &
operator >> (std::istream &is, pageSlice &rpsv)
  {
        is >> rpsv.m_length;
        return is;
  }
} //namespace ns3





