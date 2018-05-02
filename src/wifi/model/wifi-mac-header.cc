/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */
#include "ns3/assert.h"
#include "ns3/address-utils.h"
#include "wifi-mac-header.h"
#include "ns3/log.h"


#include "ap-wifi-mac.h"
//#include "ns3/assert.h"
//#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/boolean.h"
#include "qos-tag.h"
#include "wifi-phy.h"
#include "dcf-manager.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mgt-headers.h"
#include "extension-headers.h"
#include "mac-low.h"
#include "amsdu-subframe-header.h"
#include "msdu-aggregator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacHeader");
NS_OBJECT_ENSURE_REGISTERED (WifiMacHeader);

enum
{
  TYPE_MGT = 0,
  TYPE_CTL  = 1,
  TYPE_DATA = 2,
  TYPE_EXTENSION = 3
};

enum
{
  SUBTYPE_CTL_BACKREQ = 8,
  SUBTYPE_CTL_BACKRESP = 9,
  SUBTYPE_CTL_PSPOLL = 10,
  SUBTYPE_CTL_RTS = 11,
  SUBTYPE_CTL_CTS = 12,
  SUBTYPE_CTL_ACK = 13,
 
  SUBTYPE_CTL_CTLWRAPPER = 7

};

WifiMacHeader::WifiMacHeader ()
  : m_ctrlMoreData (0),
    m_ctrlWep (0),
    m_ctrlOrder (1),
    m_ctrlPowerManagement (0),
    m_amsduPresent (0)
{
}
WifiMacHeader::~WifiMacHeader ()
{
}

void
WifiMacHeader::SetDsFrom (void)
{
  m_ctrlFromDs = 1;
}

void
WifiMacHeader::SetDsNotFrom (void)
{
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetDsTo (void)
{
  m_ctrlToDs = 1;
}

void
WifiMacHeader::SetDsNotTo (void)
{
  m_ctrlToDs = 0;
}

void
WifiMacHeader::SetAddr1 (Mac48Address address)
{
  m_addr1 = address;
}

void
WifiMacHeader::SetAddr2 (Mac48Address address)
{
  m_addr2 = address;
}

void
WifiMacHeader::SetAddr3 (Mac48Address address)
{
  m_addr3 = address;
}

void
WifiMacHeader::SetAddr4 (Mac48Address address)
{
  m_addr4 = address;
}

void
WifiMacHeader::SetAssocReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0;
}
    
void
WifiMacHeader::SetDisAssocReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 10;
}

void
WifiMacHeader::SetAssocResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 1;
}

void
WifiMacHeader::SetProbeReq (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 4;
}

void
WifiMacHeader::SetProbeResp (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 5;
}

void
WifiMacHeader::SetBeacon (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 8;
}
    
void
WifiMacHeader::SetS1gBeacon (void)
{
  m_ctrlType = TYPE_EXTENSION;
  m_ctrlSubtype = 0x01;
}

void
WifiMacHeader::SetBlockAckReq (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 8;
}

void
WifiMacHeader::SetBlockAck (void)
{
  m_ctrlType = TYPE_CTL;
  m_ctrlSubtype = 9;
}

void
WifiMacHeader::SetTypeData (void)
{
  m_ctrlType = TYPE_DATA;
  m_ctrlSubtype = 0;
}

void
WifiMacHeader::SetAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0D;
}

void
WifiMacHeader::SetMultihopAction (void)
{
  m_ctrlType = TYPE_MGT;
  m_ctrlSubtype = 0x0F;
}

void
WifiMacHeader::SetType (enum WifiMacType type)
{
  switch (type)
    {
    case WIFI_MAC_CTL_BACKREQ:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKREQ;
      break;
    case WIFI_MAC_CTL_BACKRESP:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_BACKRESP;
      break;
    case WIFI_MAC_CTL_PSPOLL:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_PSPOLL;
      break;
    case WIFI_MAC_CTL_RTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_RTS;
      break;
    case WIFI_MAC_CTL_CTS:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTS;
      break;
    case WIFI_MAC_CTL_ACK:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_ACK;
      break;
    case WIFI_MAC_CTL_CTLWRAPPER:
      m_ctrlType = TYPE_CTL;
      m_ctrlSubtype = SUBTYPE_CTL_CTLWRAPPER;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_MGT_PROBE_REQUEST:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_MGT_PROBE_RESPONSE:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_MGT_BEACON:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_MGT_DISASSOCIATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_MGT_AUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_MGT_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 13;
      break;
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      m_ctrlType = TYPE_MGT;
      m_ctrlSubtype = 15;
      break;
            
    case WIFI_MAC_EXTENSION_S1G_BEACON:
      m_ctrlType = TYPE_EXTENSION;
      m_ctrlSubtype = 1;
        break;

    case WIFI_MAC_DATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 0;
      break;
    case WIFI_MAC_DATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 1;
      break;
    case WIFI_MAC_DATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 2;
      break;
    case WIFI_MAC_DATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 3;
      break;
    case WIFI_MAC_DATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 4;
      break;
    case WIFI_MAC_DATA_NULL_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 5;
      break;
    case WIFI_MAC_DATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 6;
      break;
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 7;
      break;
    case WIFI_MAC_QOSDATA:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 8;
      break;
    case WIFI_MAC_QOSDATA_CFACK:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 9;
      break;
    case WIFI_MAC_QOSDATA_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 10;
      break;
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 11;
      break;
    case WIFI_MAC_QOSDATA_NULL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 12;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 14;
      break;
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      m_ctrlType = TYPE_DATA;
      m_ctrlSubtype = 15;
      break;
    }
  m_ctrlToDs = 0;
  m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetRawDuration (uint16_t duration)
{
  m_duration = duration;
}

void
WifiMacHeader::SetDuration (Time duration)
{
  int64_t duration_us = ceil ((double)duration.GetNanoSeconds () / 1000);
  NS_ASSERT (duration_us >= 0 && duration_us <= 0x7fff);
  m_duration = static_cast<uint16_t> (duration_us);
}

void WifiMacHeader::SetId (uint16_t id)
{
  NS_ASSERT ((id << 14) == 3);
  m_duration = id;
}

void WifiMacHeader::SetSequenceNumber (uint16_t seq)
{
  m_seqSeq = seq;
}

void WifiMacHeader::SetFragmentNumber (uint8_t frag)
{
  m_seqFrag = frag;
}

void WifiMacHeader::SetNoMoreFragments (void)
{
  m_ctrlMoreFrag = 0;
}

void WifiMacHeader::SetMoreFragments (void)
{
  m_ctrlMoreFrag = 1;
}

void WifiMacHeader::SetOrder (void)
{
  m_ctrlOrder = 1;
}

void WifiMacHeader::SetNoOrder (void)
{
  m_ctrlOrder = 0;
}

void WifiMacHeader::SetRetry (void)
{
  m_ctrlRetry = 1;
}

void WifiMacHeader::SetNoRetry (void)
{
  m_ctrlRetry = 0;
}

//
void WifiMacHeader::SetNextTBTT (void)
{
  m_nextTBTT = 1;
}
    
void WifiMacHeader::SetNoNextTBTT (void)
{
  m_nextTBTT = 0;
}
    
void WifiMacHeader::SetCompressedSSID (void)
{
  m_compressed_SSID = 1;
}

void WifiMacHeader::SetNoCompressedSSID (void)
{
  m_compressed_SSID = 0;
}

void WifiMacHeader::SetANO (void)
{
  m_ANO = 1;
}

void WifiMacHeader::SetNoANO (void)
{
  m_ANO = 0;
}

void WifiMacHeader::SetSecurity (void)
{
  m_security = 1;
}

void WifiMacHeader::SetNoSecurity (void)
{
  m_security = 0;
}

void WifiMacHeader::SetAPPM (void)
{
  m_AP_PM = 1;
}

void WifiMacHeader::SetNoAPPM (void)
{
  m_AP_PM = 0;
}
    
void WifiMacHeader::SetBSSBW (enum S1G_BSS_BW S1g_BW)
{
  switch (S1g_BW)
    {
    case S1G_BSS_BW_F1_T2:
      m_BSS_BW = 0;
      break;
    case S1G_BSS_BW_F_T:
      m_BSS_BW = 1;
      break;
    case S1G_BSS_BW_F1_T4:
      m_BSS_BW = 2;
      break;
    case S1G_BSS_BW_F2_T4:
      m_BSS_BW = 3;
      break;
    case S1G_BSS_BW_F1_T8:
      m_BSS_BW = 4;
      break;
    case S1G_BSS_BW_F2_T8:
      m_BSS_BW = 5;
      break;
    case S1G_BSS_BW_F1_T16:
      m_BSS_BW = 6;
      break;
    case S1G_BSS_BW_F2_T16:
      m_BSS_BW = 7;
      break;
    }
}
    
//

void WifiMacHeader::SetQosTid (uint8_t tid)
{
  m_qosTid = tid;
}

void WifiMacHeader::SetQosEosp ()
{
  m_qosEosp = 1;
}

void WifiMacHeader::SetQosNoEosp ()
{
  m_qosEosp = 0;
}

void WifiMacHeader::SetQosAckPolicy (enum QosAckPolicy policy)
{
  switch (policy)
    {
    case NORMAL_ACK:
      m_qosAckPolicy = 0;
      break;
    case NO_ACK:
      m_qosAckPolicy = 1;
      break;
    case NO_EXPLICIT_ACK:
      m_qosAckPolicy = 2;
      break;
    case BLOCK_ACK:
      m_qosAckPolicy = 3;
      break;
    }
}

void
WifiMacHeader::SetQosNormalAck ()
{
  m_qosAckPolicy = 0;
}

void
WifiMacHeader::SetQosBlockAck ()
{
  m_qosAckPolicy = 3;
}

void
WifiMacHeader::SetQosNoAck ()
{
  m_qosAckPolicy = 1;
}

void WifiMacHeader::SetQosAmsdu (void)
{
  m_amsduPresent = 1;
}

void WifiMacHeader::SetQosNoAmsdu (void)
{
  m_amsduPresent = 0;
}

void WifiMacHeader::SetQosTxopLimit (uint8_t txop)
{
  m_qosStuff = txop;
}

Mac48Address
WifiMacHeader::GetAddr1 (void) const
{
  return m_addr1;
}

Mac48Address
WifiMacHeader::GetAddr2 (void) const
{
  return m_addr2;
}

Mac48Address
WifiMacHeader::GetAddr3 (void) const
{
  return m_addr3;
}

Mac48Address
WifiMacHeader::GetAddr4 (void) const
{
  return m_addr4;
}

enum WifiMacType
WifiMacHeader::GetType (void) const
{
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_MGT_ASSOCIATION_REQUEST;
          break;
        case 1:
          return WIFI_MAC_MGT_ASSOCIATION_RESPONSE;
          break;
        case 2:
          return WIFI_MAC_MGT_REASSOCIATION_REQUEST;
          break;
        case 3:
          return WIFI_MAC_MGT_REASSOCIATION_RESPONSE;
          break;
        case 4:
          return WIFI_MAC_MGT_PROBE_REQUEST;
          break;
        case 5:
          return WIFI_MAC_MGT_PROBE_RESPONSE;
          break;
        case 8:
          return WIFI_MAC_MGT_BEACON;
          break;
        case 10:
          return WIFI_MAC_MGT_DISASSOCIATION;
          break;
        case 11:
          return WIFI_MAC_MGT_AUTHENTICATION;
          break;
        case 12:
          return WIFI_MAC_MGT_DEAUTHENTICATION;
          break;
        case 13:
          return WIFI_MAC_MGT_ACTION;
          break;
        case 14:
          return WIFI_MAC_MGT_ACTION_NO_ACK;
          break;
        case 15:
          return WIFI_MAC_MGT_MULTIHOP_ACTION;
          break;
        }
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_BACKREQ:
          return WIFI_MAC_CTL_BACKREQ;
          break;
        case SUBTYPE_CTL_BACKRESP:
          return WIFI_MAC_CTL_BACKRESP;
          break;
        case SUBTYPE_CTL_RTS:
          return WIFI_MAC_CTL_RTS;
          break;
        case SUBTYPE_CTL_CTS:
          return WIFI_MAC_CTL_CTS;
          break;
        case SUBTYPE_CTL_ACK:
          return WIFI_MAC_CTL_ACK;
          break;
        case SUBTYPE_CTL_PSPOLL:
          return WIFI_MAC_CTL_PSPOLL;
          break;
        }
      break;
    case TYPE_DATA:
      switch (m_ctrlSubtype)
        {
        case 0:
          return WIFI_MAC_DATA;
          break;
        case 1:
          return WIFI_MAC_DATA_CFACK;
          break;
        case 2:
          return WIFI_MAC_DATA_CFPOLL;
          break;
        case 3:
          return WIFI_MAC_DATA_CFACK_CFPOLL;
          break;
        case 4:
          return WIFI_MAC_DATA_NULL;
          break;
        case 5:
          return WIFI_MAC_DATA_NULL_CFACK;
          break;
        case 6:
          return WIFI_MAC_DATA_NULL_CFPOLL;
          break;
        case 7:
          return WIFI_MAC_DATA_NULL_CFACK_CFPOLL;
          break;
        case 8:
          return WIFI_MAC_QOSDATA;
          break;
        case 9:
          return WIFI_MAC_QOSDATA_CFACK;
          break;
        case 10:
          return WIFI_MAC_QOSDATA_CFPOLL;
          break;
        case 11:
          return WIFI_MAC_QOSDATA_CFACK_CFPOLL;
          break;
        case 12:
          return WIFI_MAC_QOSDATA_NULL;
          break;
        case 14:
          return WIFI_MAC_QOSDATA_NULL_CFPOLL;
          break;
        case 15:
          return WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL;
          break;
        }
      break;
   case TYPE_EXTENSION:
      switch (m_ctrlSubtype)
        {
        case 1:
        default:
          return WIFI_MAC_EXTENSION_S1G_BEACON;
          break;
        }
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum WifiMacType) -1;
}

bool
WifiMacHeader::IsFromDs (void) const
{
  return m_ctrlFromDs == 1;
}

bool
WifiMacHeader::IsToDs (void) const
{
  return m_ctrlToDs == 1;
}

bool
WifiMacHeader::IsData (void) const
{
  return (m_ctrlType == TYPE_DATA);

}

bool
WifiMacHeader::IsQosData (void) const
{
  return (m_ctrlType == TYPE_DATA && (m_ctrlSubtype & 0x08));
}

bool
WifiMacHeader::IsCtl (void) const
{
  return (m_ctrlType == TYPE_CTL);
}

bool
WifiMacHeader::IsMgt (void) const
{
  return (m_ctrlType == TYPE_MGT);
}

bool
WifiMacHeader::IsCfpoll (void) const
{
  switch (GetType ())
    {
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
      return true;
      break;
    default:
      return false;
      break;
    }
}

bool
WifiMacHeader::IsRts (void) const
{
  return (GetType () == WIFI_MAC_CTL_RTS);
}

bool
WifiMacHeader::IsCts (void) const
{
  return (GetType () == WIFI_MAC_CTL_CTS);
}

bool
WifiMacHeader::IsAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_ACK);
}
    
bool
WifiMacHeader::IsPsPoll (void) const
{
  return (GetType () == WIFI_MAC_CTL_PSPOLL);
}

bool
WifiMacHeader::IsAssocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsAssocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsReassocReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsReassocResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsProbeReq (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_REQUEST);
}

bool
WifiMacHeader::IsProbeResp (void) const
{
  return (GetType () == WIFI_MAC_MGT_PROBE_RESPONSE);
}

bool
WifiMacHeader::IsBeacon (void) const
{
  return (GetType () == WIFI_MAC_MGT_BEACON);
}
    
bool
WifiMacHeader::IsS1gBeacon (void) const
{
  return (GetType () == WIFI_MAC_EXTENSION_S1G_BEACON);
}
    
bool
WifiMacHeader::IsNextTBTT (void) const
{
  return m_nextTBTT == 1;
}
    
bool
WifiMacHeader::IsCompressedSSID (void) const
{
  return m_compressed_SSID == 1;
}

bool
WifiMacHeader::IsANO (void) const
{
  return m_ANO == 1;
}
    
enum S1G_BSS_BW
WifiMacHeader::GetBSSBW (void) const
{
  switch (m_BSS_BW)
    {
    case 0:
      return S1G_BSS_BW_F1_T2;
      break;
    case 1:
      return S1G_BSS_BW_F_T;
      break;
    case 2:
      return S1G_BSS_BW_F1_T4;
      break;
    case 3:
      return S1G_BSS_BW_F2_T4;
      break;
    case 4:
      return S1G_BSS_BW_F1_T8;
      break;
    case 5:
      return S1G_BSS_BW_F2_T8;
      break;
    case 6:
      return S1G_BSS_BW_F1_T16;
      break;
    case 7:
      return S1G_BSS_BW_F2_T16;
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum S1G_BSS_BW) -1;
}

bool
WifiMacHeader::IsDisassociation (void) const
{
  return (GetType () == WIFI_MAC_MGT_DISASSOCIATION);
}

bool
WifiMacHeader::IsAuthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_AUTHENTICATION);
}

bool
WifiMacHeader::IsDeauthentication (void) const
{
  return (GetType () == WIFI_MAC_MGT_DEAUTHENTICATION);
}

bool
WifiMacHeader::IsAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_ACTION);
}

bool
WifiMacHeader::IsMultihopAction (void) const
{
  return (GetType () == WIFI_MAC_MGT_MULTIHOP_ACTION);
}

bool
WifiMacHeader::IsBlockAckReq (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKREQ) ? true : false;
}

bool
WifiMacHeader::IsBlockAck (void) const
{
  return (GetType () == WIFI_MAC_CTL_BACKRESP) ? true : false;
}

uint16_t
WifiMacHeader::GetRawDuration (void) const
{
  return m_duration;
}

Time
WifiMacHeader::GetDuration (void) const
{
  return MicroSeconds (m_duration);
}
    
uint16_t
WifiMacHeader::GetAID (void) const
{
  NS_ASSERT ((m_duration << 14) == 3);
  return m_duration;
}

uint16_t
WifiMacHeader::GetSequenceControl (void) const
{
  return (m_seqSeq << 4) | m_seqFrag;
}

uint16_t
WifiMacHeader::GetSequenceNumber (void) const
{
  return m_seqSeq;
}

uint16_t
WifiMacHeader::GetFragmentNumber (void) const
{
  return m_seqFrag;
}

bool
WifiMacHeader::IsRetry (void) const
{
  return (m_ctrlRetry == 1);
}

bool
WifiMacHeader::IsMoreFragments (void) const
{
  return (m_ctrlMoreFrag == 1);
}

bool
WifiMacHeader::IsQosBlockAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 3);
}

bool
WifiMacHeader::IsQosNoAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 1);
}

bool
WifiMacHeader::IsQosAck (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosAckPolicy == 0);
}

bool
WifiMacHeader::IsQosEosp (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_qosEosp == 1);
}

bool
WifiMacHeader::IsQosAmsdu (void) const
{
  NS_ASSERT (IsQosData ());
  return (m_amsduPresent == 1);
}

uint8_t
WifiMacHeader::GetQosTid (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosTid;
}

enum WifiMacHeader::QosAckPolicy
WifiMacHeader::GetQosAckPolicy (void) const
{
  switch (m_qosAckPolicy)
    {
    case 0:
      return NORMAL_ACK;
      break;
    case 1:
      return NO_ACK;
      break;
    case 2:
      return NO_EXPLICIT_ACK;
      break;
    case 3:
      return BLOCK_ACK;
      break;
    }
  // NOTREACHED
  NS_ASSERT (false);
  return (enum QosAckPolicy) -1;
}

uint8_t
WifiMacHeader::GetQosTxopLimit (void) const
{
  NS_ASSERT (IsQosData ());
  return m_qosStuff;
}

uint16_t
WifiMacHeader::GetFrameControl (void) const
{
  uint16_t val = 0;
  if (IsS1gBeacon ())
    {
      val |= (m_ctrlType << 2) & (0x3 << 2);
      val |= (m_ctrlSubtype << 4) & (0xf << 4);
      val |= (m_nextTBTT << 8) & (0x1 << 8);
      val |= (m_compressed_SSID << 9) & (0x1 << 9);
      val |= (m_ANO << 10) & (0x1 << 10);
      val |= (m_BSS_BW << 11) & (0x7 << 11);
      val |= (m_security << 14) & (0x01 << 14);
      val |= (m_AP_PM << 15) & (0x01 << 15);
      return val;
    }
  else
    {
      val |= (m_ctrlType << 2) & (0x3 << 2);
      val |= (m_ctrlSubtype << 4) & (0xf << 4);
      val |= (m_ctrlToDs << 8) & (0x1 << 8);
      val |= (m_ctrlFromDs << 9) & (0x1 << 9);
      val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
      val |= (m_ctrlRetry << 11) & (0x1 << 11);
      val |= (m_ctrlPowerManagement << 12) & (0x1 << 12);
      val |= (m_ctrlMoreData << 13) & (0x1 << 13);
      val |= (m_ctrlWep << 14) & (0x1 << 14);
      val |= (m_ctrlOrder << 15) & (0x1 << 15);
      return val;
    }
}
    
uint16_t
WifiMacHeader::GetQosControl (void) const
{
  uint16_t val = 0;
  val |= m_qosTid;
  val |= m_qosEosp << 4;
  val |= m_qosAckPolicy << 5;
  val |= m_amsduPresent << 7;
  val |= m_qosStuff << 8;
  return val;
}

void
WifiMacHeader::SetFrameControl (bool S1gBeacon, uint16_t ctrl)
{
  if (S1gBeacon)
    {
      //m_protVersion = ctrl & 0x03;
      m_ctrlType = (ctrl >> 2) & 0x03;
      m_ctrlSubtype = (ctrl >> 4) & 0x0f;
      m_nextTBTT = (ctrl >> 8) & 0x01;
      m_compressed_SSID = (ctrl >> 9) & 0x01;
      m_ANO = (ctrl >> 10) & 0x01;
      m_BSS_BW = (ctrl >> 11) & 0x07;
      m_security = (ctrl >> 14) & 0x01;
      m_AP_PM = (ctrl >> 15) & 0x01;
      NS_ASSERT (m_ctrlType == TYPE_EXTENSION && m_ctrlSubtype == 1);
      m_ctrlMoreFrag = 0;
    }
  else
    {
      m_ctrlType = (ctrl >> 2) & 0x03;
      m_ctrlSubtype = (ctrl >> 4) & 0x0f;
      m_ctrlToDs = (ctrl >> 8) & 0x01;
      m_ctrlFromDs = (ctrl >> 9) & 0x01;
      m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
      m_ctrlRetry = (ctrl >> 11) & 0x01;
      m_ctrlPowerManagement = (ctrl >> 12) & 0x01;
      m_ctrlMoreData = (ctrl >> 13) & 0x01;
      m_ctrlWep = (ctrl >> 14) & 0x01;
      m_ctrlOrder = (ctrl >> 15) & 0x01;
     }
}

/*
void
WifiMacHeader::SetFrameControl_S1gBeacon (uint16_t ctrl)
{
   m_protVersion = ctrl & 0x03;
   m_ctrlType = (ctrl >> 2) & 0x03;
   m_ctrlSubtype = (ctrl >> 4) & 0x0f;
   m_nextTBTT = (ctrl >> 8) & 0x01;
   m_compressed_SSID = (ctrl >> 9) & 0x01;
   m_ANO = (ctrl >> 10) & 0x01;
   m_BSS_BW = (ctrl >> 11) & 0x07;
   m_security = (ctrl >> 14) & 0x01;
   m_AP_PM = (ctrl >> 15) & 0x01;
}
*/

    
void
WifiMacHeader::SetSequenceControl (uint16_t seq)
{
  m_seqFrag = seq & 0x0f;
  m_seqSeq = (seq >> 4) & 0x0fff;
}
void
WifiMacHeader::SetQosControl (uint16_t qos)
{
  m_qosTid = qos & 0x000f;
  m_qosEosp = (qos >> 4) & 0x0001;
  m_qosAckPolicy = (qos >> 5) & 0x0003;
  m_amsduPresent = (qos >> 7) & 0x0001;
  m_qosStuff = (qos >> 8) & 0x00ff;
}

uint32_t
WifiMacHeader::GetSize (void) const
{
  uint32_t size = 0;
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      break;
    case TYPE_CTL:
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_PSPOLL:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          size = 2 + 2 + 6;
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          size = 2 + 2 + 6 + 6;
          break;
        case SUBTYPE_CTL_CTLWRAPPER:
          size = 2 + 2 + 6 + 2 + 4;
          break;
        }
      break;
    case TYPE_DATA:
      size = 2 + 2 + 6 + 6 + 6 + 2;
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          size += 6;
        }
      if (m_ctrlSubtype & 0x08)
        {
          size += 2;
        }
      break;
    case TYPE_EXTENSION:
      size = 2 + 2 + 6 + 6 +6; //for debug
      break;
    }
  return size;
}

const char *
WifiMacHeader::GetTypeString (void) const
{
#define FOO(x) \
case WIFI_MAC_ ## x: \
  return # x; \
  break;

  switch (GetType ())
    {
      FOO (CTL_RTS);
      FOO (CTL_CTS);
      FOO (CTL_PSPOLL);
      FOO (CTL_ACK);
      FOO (CTL_BACKREQ);
      FOO (CTL_BACKRESP);

      FOO (MGT_BEACON);
      FOO (MGT_ASSOCIATION_REQUEST);
      FOO (MGT_ASSOCIATION_RESPONSE);
      FOO (MGT_DISASSOCIATION);
      FOO (MGT_REASSOCIATION_REQUEST);
      FOO (MGT_REASSOCIATION_RESPONSE);
      FOO (MGT_PROBE_REQUEST);
      FOO (MGT_PROBE_RESPONSE);
      FOO (MGT_AUTHENTICATION);
      FOO (MGT_DEAUTHENTICATION);
      FOO (MGT_ACTION);
      FOO (MGT_ACTION_NO_ACK);
      FOO (MGT_MULTIHOP_ACTION);

      FOO (DATA);
      FOO (DATA_CFACK);
      FOO (DATA_CFPOLL);
      FOO (DATA_CFACK_CFPOLL);
      FOO (DATA_NULL);
      FOO (DATA_NULL_CFACK);
      FOO (DATA_NULL_CFPOLL);
      FOO (DATA_NULL_CFACK_CFPOLL);
      FOO (QOSDATA);
      FOO (QOSDATA_CFACK);
      FOO (QOSDATA_CFPOLL);
      FOO (QOSDATA_CFACK_CFPOLL);
      FOO (QOSDATA_NULL);
      FOO (QOSDATA_NULL_CFPOLL);
      FOO (QOSDATA_NULL_CFACK_CFPOLL);
            
      FOO (EXTENSION_S1G_BEACON);
    default:
      return "ERROR";
    }
#undef FOO
  // needed to make gcc 4.0.1 ppc darwin happy.
  return "BIG_ERROR";
}

TypeId
WifiMacHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacHeader")
    .SetParent<Header> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacHeader> ()
  ;
  return tid;
}

TypeId
WifiMacHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
WifiMacHeader::PrintFrameControl (std::ostream &os) const  //to do, support S1G beacon frame
{
  os << "ToDS=" << std::hex << (int) m_ctrlToDs << ", FromDS=" << std::hex << (int) m_ctrlFromDs
     << ", MoreFrag=" <<  std::hex << (int) m_ctrlMoreFrag << ", Retry=" << std::hex << (int) m_ctrlRetry
     << ", PowerManagement=" << std::hex << (int)m_ctrlPowerManagement
     << ", MoreData=" <<  std::hex << (int) m_ctrlMoreData << std::dec
  ;
}

void
WifiMacHeader::Print (std::ostream &os) const  //to do, support S1G beacon frame
{
  os << GetTypeString () << " ";
  switch (GetType ())
    {
    case WIFI_MAC_CTL_RTS:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1 << ", TA=" << m_addr2;
      break;
    case WIFI_MAC_CTL_CTS:
    case WIFI_MAC_CTL_ACK:
      os << "Duration/ID=" << m_duration << "us"
         << ", RA=" << m_addr1;
      break;
    case WIFI_MAC_CTL_BACKREQ:
      break;
    case WIFI_MAC_CTL_BACKRESP:
      break;
    case WIFI_MAC_CTL_CTLWRAPPER:
      break;
    case WIFI_MAC_CTL_PSPOLL:
    	os << "Duration/ID=" << m_duration << "us"
    	<< ", RA=" << m_addr1 << ", TA=" << m_addr2;
      break;

    case WIFI_MAC_MGT_BEACON:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_DISASSOCIATION:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_PROBE_RESPONSE:
    case WIFI_MAC_MGT_AUTHENTICATION:
    case WIFI_MAC_MGT_DEAUTHENTICATION:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << ", DA=" << m_addr1 << ", SA=" << m_addr2
         << ", BSSID=" << m_addr3 << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_ACTION:
    case WIFI_MAC_MGT_ACTION_NO_ACK:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us"
         << "DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
      os << " Duration/ID=" << m_duration << "us"
         << "RA=" << m_addr1 << ", TA=" << m_addr2 << ", DA=" << m_addr3
         << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_DATA:
      PrintFrameControl (os);
      os << " Duration/ID=" << m_duration << "us";
      if (!m_ctrlToDs && !m_ctrlFromDs)
        {
          os << "DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3;
        }
      else if (!m_ctrlToDs && m_ctrlFromDs)
        {
          os << "DA=" << m_addr1 << ", SA=" << m_addr3 << ", BSSID=" << m_addr2;
        }
      else if (m_ctrlToDs && !m_ctrlFromDs)
        {
          os << "DA=" << m_addr3 << ", SA=" << m_addr2 << ", BSSID=" << m_addr1;
        }
      else if (m_ctrlToDs && m_ctrlFromDs)
        {
          os << "DA=" << m_addr3 << ", SA=" << m_addr4 << ", RA=" << m_addr1 << ", TA=" << m_addr2;
        }
      else
        {
          NS_FATAL_ERROR ("Impossible ToDs and FromDs flags combination");
        }
      os << ", FragNumber=" << std::hex << (int) m_seqFrag << std::dec
         << ", SeqNumber=" << m_seqSeq;
      break;
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL:
    case WIFI_MAC_DATA_NULL_CFACK:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA:
    case WIFI_MAC_QOSDATA_CFACK:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_EXTENSION_S1G_BEACON:  //to do
      break;
    }
}

uint32_t
WifiMacHeader::GetSerializedSize (void) const
{
  return GetSize ();
}

void
WifiMacHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteHtolsbU16 (GetFrameControl ());
  i.WriteHtolsbU16 (m_duration);
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      WriteTo (i, m_addr1);
      WriteTo (i, m_addr2);
      WriteTo (i, m_addr3);
      i.WriteHtolsbU16 (GetSequenceControl ());
      break;
    case TYPE_CTL:
      WriteTo (i, m_addr1);
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_PSPOLL:
          WriteTo (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          WriteTo (i, m_addr2);
          break;
        default:
          //NOTREACHED
          NS_ASSERT (false);
          break;
        }
      break;
    case TYPE_DATA:
      {
        WriteTo (i, m_addr1);
        WriteTo (i, m_addr2);
        WriteTo (i, m_addr3);
        i.WriteHtolsbU16 (GetSequenceControl ());
        if (m_ctrlToDs && m_ctrlFromDs)
          {
            WriteTo (i, m_addr4);
          }
        if (m_ctrlSubtype & 0x08)
          {
            i.WriteHtolsbU16 (GetQosControl ());
          }
      } break;
    case TYPE_EXTENSION:
      WriteTo (i, m_addr1);
      WriteTo (i, m_addr2); // for debug
      WriteTo (i, m_addr3); // for debug
      break;
    default:
      //NOTREACHED
      NS_ASSERT (false);
      break;
    }
}

uint32_t
WifiMacHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t frame_control = i.ReadLsbtohU16 ();
  uint8_t frame_ctrlType = (frame_control >> 2) & 0x03;
  uint8_t frame_ctrlSubtype = (frame_control >> 4) & 0x0f;
  bool frame_S1gbeacon = ((frame_ctrlType == 3) && (frame_ctrlSubtype == 1));
  SetFrameControl (frame_S1gbeacon, frame_control);
  m_duration = i.ReadLsbtohU16 ();
  
  switch (m_ctrlType)
    {
    case TYPE_MGT:
      ReadFrom (i, m_addr1);
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      break;
    case TYPE_CTL:
      ReadFrom (i, m_addr1);
      switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_PSPOLL:
          ReadFrom (i, m_addr2);
          break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
          break;
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
          ReadFrom (i, m_addr2);
          break;
        }
      break;
    case TYPE_DATA:
      ReadFrom (i, m_addr1);
      ReadFrom (i, m_addr2);
      ReadFrom (i, m_addr3);
      SetSequenceControl (i.ReadLsbtohU16 ());
      if (m_ctrlToDs && m_ctrlFromDs)
        {
          ReadFrom (i, m_addr4);
        }
      if (m_ctrlSubtype & 0x08)
        {
          SetQosControl (i.ReadLsbtohU16 ());
        }
      break;
    case TYPE_EXTENSION:
      ReadFrom (i, m_addr1);
      ReadFrom (i, m_addr2); //for debug
      ReadFrom (i, m_addr3); //for debug
      break;
    }
  return i.GetDistanceFrom (start);
}

} //namespace ns3
