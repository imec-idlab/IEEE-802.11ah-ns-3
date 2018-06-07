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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */
#ifndef STA_WIFI_MAC_H
#define STA_WIFI_MAC_H

#include "regular-wifi-mac.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/traced-callback.h"
#include "supported-rates.h"
#include "amsdu-subframe-header.h"
#include "s1g-capabilities.h"
#include "ns3/traced-value.h"
#include "extension-headers.h"

namespace ns3  {

class MgtAddBaRequestHeader;

/**
 * \ingroup wifi
 *
 * The Wifi MAC high model for a non-AP STA in a BSS.
 */
class StaWifiMac : public RegularWifiMac
{
public:
  static TypeId GetTypeId (void);

  typedef void (* S1gBeaconMissedCallback)
		  (bool nextBeaconIsDTIM);

  StaWifiMac ();
  virtual ~StaWifiMac ();

  /**
   * \param stationManager the station manager attached to this MAC.
   */
  virtual void SetWifiRemoteStationManager (Ptr<WifiRemoteStationManager> stationManager);

  /**
   * \param packet the packet to send.
   * \param to the address to which the packet should be sent.
   *
   * The packet should be enqueued in a tx queue, and should be
   * dequeued as soon as the channel access function determines that
   * access is granted to this MAC.
   */
  virtual void Enqueue (Ptr<const Packet> packet, Mac48Address to);

  /**
   * \param missed the number of beacons which must be missed
   * before a new association sequence is started.
   */
  void SetMaxMissedBeacons (uint32_t missed);
  /**
   * \param timeout
   *
   * If no probe response is received within the specified
   * timeout, the station sends a new probe request.
   */
  void SetProbeRequestTimeout (Time timeout);
  /**
   * \param timeout
   *
   * If no association response is received within the specified
   * timeout, the station sends a new association request.
   */
  void SetAssocRequestTimeout (Time timeout);

  /**
   * Start an active association sequence immediately.
   */
  void StartActiveAssociation (void);
  void TxOk (const WifiMacHeader &hdr);

    uint32_t GetStaType (void) const;
    uint32_t GetChannelWidth (void) const;
    void SetStaType (uint32_t statype);
    void SetChannelWidth (uint32_t width);
    void SendDisAssociationRequest (void);
    void SendAssociationRequest (void);

  /**
   * Get Station AID.
   */
  uint32_t GetAID (void) const;

    /*void SetPageSlicingSupported (uint8_t support);
    uint8_t GetPageSlicingSupported (void) const;*/
private:
    uint32_t m_staType;
    uint32_t m_channelWidth;
    Time m_currentslotDuration;
  /**
   * The current MAC state of the STA.
   */
  enum MacState
  {
    ASSOCIATED,
    WAIT_PROBE_RESP,
    WAIT_ASSOC_RESP,
    WAIT_DISASSOC_ACK,
    BEACON_MISSED,
    REFUSED
  };

  void OnAssociated();
  void OnDeassociated();
/*
  void GrantDCAAccess();
  void DenyDCAAccess();
  */
  /**
   * Enable or disable active probing.
   *
   * \param enable enable or disable active probing
   */
  void SetActiveProbing (bool enable);
  /**
   * Return whether active probing is enabled.
   *
   * \return true if active probing is enabled, false otherwise
   */
  bool GetActiveProbing (void) const;

  virtual void Receive (Ptr<Packet> packet, const WifiMacHeader *hdr);

  /**
   * Forward a probe request packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   */
  void SendProbeRequest (void);
  /**
   * Forward an association request packet to the DCF. The standard is not clear on the correct
   * queue for management frames if QoS is supported. We always use the DCF.
   */
  //void SendAssociationRequest (void);
  /**
   * Try to ensure that we are associated with an AP by taking an appropriate action
   * depending on the current association status.
   */
  void TryToEnsureAssociated (void);
  /**
   * This method is called after the association timeout occurred. We switch the state to
   * WAIT_ASSOC_RESP and re-send an association request.
   */
  void AssocRequestTimeout (void);
  /**
   * This method is called after the probe request timeout occurred. We switch the state to
   * WAIT_PROBE_RESP and re-send a probe request.
   */
  void ProbeRequestTimeout (void);
  /**
   * Return whether we are associated with an AP.
   *
   * \return true if we are associated with an AP, false otherwise
   */
  bool IsAssociated (void) const;
  /**
   * Return whether we are waiting for an association response from an AP.
   *
   * \return true if we are waiting for an association response from an AP, false otherwise
   */
  bool IsWaitDisAssocTxOk (void) const;
  bool IsWaitAssocResp (void) const;
  /**
   * This method is called after we have not received a beacon from the AP
   */
  void MissedBeacons (void);
  /**
   * Restarts the beacon timer.
   *
   * \param delay the delay before the watchdog fires
   */
  void RestartBeaconWatchdog (Time delay);
  /**
   * Return an instance of SupportedRates that contains all rates that we support
   * including HT rates.
   *
   * \return SupportedRates all rates that we support
   */
  SupportedRates GetSupportedRates (void) const;
  /**
   * Set the current MAC state.
   *
   * \param value the new state
   */
  void SetState (enum MacState value);
  /**
   * Return the HT capability of the current AP.
   *
   * \return the HT capability that we support
   */
  HtCapabilities GetHtCapabilities (void) const;
  S1gCapabilities GetS1gCapabilities (void) const;
  /**
   * Set the AID.
   *
   * \param aid the AID received from assoc response frame
   */
  void SetAID (uint32_t aid);

  void SetRawDuration (Time interval);
  Time GetRawDuration (void) const;
  Time GetEarlyWakeTime (void) const;
  void SendPspoll (void);
  void SendPspollIfnecessary (void);
  void S1gBeaconReceived (S1gBeaconHeader beacon);
  void S1gTIMReceived (S1gBeaconHeader beacon);

  void StartRawbackoff (void);
  void OutsideRawStartBackoff (void);
  bool Is(uint8_t blockbitmap, uint8_t j);
  void InsideBackoff (void);
  void RawSlotStartBackoff (void);  
  void RawSlotStartBackoffPostpone (void);
  uint32_t GetSelfPageSliceNum (void);

  void SetDataBuffered (void);
  void ClearDataBuffered (void);
  void SetInRAWgroup(void);
  void UnsetInRAWgroup(void);
  
   /**
   * wake up operation
   */
  void WakeUp (void);
  void SleepIfQueueIsEmpty(bool);
  bool HasPacketsInQueue();
  void BeaconWakeUp (void);
  void GoToSleepBinary (int value);

  TracedValue<uint16_t> nrOfTransmissionsDuringRAWSlot = 0;
  bool IsInPagebitmap (uint8_t block);
  
  void GoToSleepNextTIM (S1gBeaconHeader beacon);
  void GoToSleepCurrentTIM (S1gBeaconHeader beacon);
  void GoToSleep(Time  sleeptime); 

  Time m_lastRawDurationus;
  Time m_lastRawStart;
  Time m_rawDuration;
  Time m_slotDuration;
  Time m_statSlotStart;
  bool m_rawStart;
  bool m_inRawGroup;
  bool m_pagedStaRaw;
  bool m_dataBuffered;
  EventId m_outsideRawEvent;
  EventId m_insideBackoffEvent;
  enum MacState m_state;
  Time m_probeRequestTimeout;
  Time m_assocRequestTimeout;
  EventId m_probeRequestEvent;
  EventId m_assocRequestEvent;
  EventId m_disassocRequestEvent;
  EventId m_beaconWatchdog;
  Time m_beaconWatchdogEnd;
  uint32_t m_maxMissedBeacons;
  uint32_t m_aid;
  bool fasTAssocType;
  uint16_t fastAssocThreshold;
    uint16_t assocVaule;
  uint8_t m_slotCrossBoundary;
    
  bool firstBeacon;
  bool receivingBeacon;  
  EventId m_beaconWakeUpEvent; 
  Time beaconInterval;
  uint64_t timeDifferenceBeacon;  
  uint64_t timeBeacon;
  bool outsideraw;
  bool stationrawslot;
  bool waitingack;

  bool m_activeProbing;
  Ptr<DcaTxop> m_pspollDca;  //!< Dedicated DcaTxop for beacons
  virtual void DoDispose (void);
  
  TIM m_TIM;
  uint8_t m_DTIMCount; //!< DTIM Count
  uint8_t m_DTIMPeriod; //!< DTIM Period
  uint8_t m_TrafficIndicator;
  uint8_t m_PageSliceNum;
  uint8_t m_PageIndex;
          
  pageSlice m_pageSlice;   
  uint8_t m_PagePeriod;
  uint8_t m_Pageindex_slice; // page index from page slice element, different from page index in TIM element
  uint8_t m_Pageindex;
  uint8_t m_PageSliceLen;
  uint8_t m_PageSliceCount;
  uint8_t m_BlockOffset;
  uint8_t m_TIMOffset;  
  uint8_t m_PageBitmap[4];
  uint8_t m_PageBitmapLen;
  uint8_t m_TIMSeq;  // 0 for DTIM ???

  bool m_pagedInDtim;


  uint8_t m_selfBlock;
  uint8_t m_selfPage;
  uint8_t m_selfSubBlock;
  uint8_t m_selfAid;
  
  uint8_t m_supportsPageSlicing;

  Time m_maxTimeInQueue;
  bool m_crossSlotBoundaryAllowed;
  TracedCallback<bool> m_beaconMissed;

  TracedCallback<Mac48Address> m_assocLogger;
  TracedCallback<Mac48Address> m_deAssocLogger;
};

} //namespace ns3

#endif /* STA_WIFI_MAC_H */
