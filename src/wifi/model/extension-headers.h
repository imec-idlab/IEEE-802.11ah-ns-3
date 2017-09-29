/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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

#ifndef EXTENSION_HEADERS_H
#define EXTENSION_HEADERS_H

#include <stdint.h>

#include "ns3/header.h"
#include "ns3/mac48-address.h"
#include "s1g-beacon-compatibility.h"
#include "tim.h"
#include "rps.h"
#include "pageSlice.h"
#include "authentication-control.h"

namespace ns3 {

/**
 * \ingroup wifi
 * Implement the header for extension frames of type S1G beacon .
 */
class S1gBeaconHeader : public Header
{
public:
  S1gBeaconHeader ();
  ~S1gBeaconHeader ();
    
  void SetSA (Mac48Address sa);
  void SetChangeSequence (uint8_t sequence);
  void SetNextTBTT (uint32_t tbtt); //only (23-0) bits are used
  void SetCompressedSSID (uint32_t compressedssid);
  void SetAccessNetwork (uint8_t accessnetwork);
  void SetBeaconCompatibility (S1gBeaconCompatibility compatibility);
  void SetTIM (TIM tim);
  void SetpageSlice (pageSlice page);
  void SetRPS (RPS rps);
  void SetAuthCtrl (AuthenticationCtrl auth);

  //Mac48Address GetSA (void) const;
  uint32_t GetTimeStamp (void) const;
  uint8_t GetChangeSequence (void) const;
  uint32_t GetNextTBTT (void) const;
  uint32_t GetCompressedSSID (void) const;
  uint8_t GetAccessNetwork (void) const;
  S1gBeaconCompatibility GetBeaconCompatibility (void) const;
  TIM GetTIM (void) const;
  pageSlice GetpageSlice (void) const;
  RPS GetRPS (void) const;
  AuthenticationCtrl GetAuthCtrl (void) const;
    
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
private:
  Mac48Address m_address;
  uint32_t m_timestamp;
  uint8_t m_sequence;
  uint32_t m_tbtt;
  uint32_t m_compressedssid;
  uint8_t m_accessnetwork;

  S1gBeaconCompatibility m_beaconcompatibility;
  TIM m_tim;
  RPS m_rps;
  pageSlice m_pageSlice;
  AuthenticationCtrl  m_auth;
};




} //namespace ns3

#endif /* EXTENSION_HEADERS_H */
