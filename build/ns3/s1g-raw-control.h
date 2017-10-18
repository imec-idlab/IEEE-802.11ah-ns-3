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

#ifndef S1G_RAW_CTR_H
#define S1G_RAW_CTR_H

#include "regular-wifi-mac.h"
#include "ht-capabilities.h"
#include "amsdu-subframe-header.h"
#include "supported-rates.h"
#include "ns3/random-variable-stream.h"
#include "rps.h"

namespace ns3 {
    
struct UpdateInfo
    //(uint64_t x, uint64_t y, uint64_t z, bool success, uint64_t q, uint64_t p,uint64_t n, bool good)
    {
        uint64_t lastTryBFpreSuccessId;
        uint64_t preSuccessId;
        uint64_t preUnsuccessId;
        bool preTrySuccess;
        
        uint64_t CurrentSuccessId;
        uint64_t CurrentUnSuccessId;
        uint64_t lastTryBFCurrentSuccessId;
        bool CurrentTrySuccess;

        
        /*UpdateInfo (): lastTryBFpreSuccessId(x),
                       preSuccessId(y),
                       preUnsuccessId(z),
                       preTrySuccess(success),
                       CurrentSuccessId(q),
                       CurrentUnSuccessId(p),
               lastTryBFCurrentSuccessId(n),
                      CurrentTrySuccess(good){}*/
    };
    
class Sensor
{
public:
    Sensor ();
    virtual ~Sensor ();
    
    void SetAid (uint16_t aid); //to indify station
    void SetTransmissionSuccess (bool success);
    void EstimateTransmissionInterval (uint64_t currentId, uint64_t beaconinterval);
    void EstimateNextTransmissionId (uint64_t intervalId);
    void SetStaType (uint8_t statype);
    void SetDataRate (uint16_t m_dataRate); //question, how to get datarate info? Impossibile
    void SetPacketSize (uint16_t m_packetSize); //necessary?
    
    void SetEverSuccess(bool success);
    bool GetEverSuccess (void) const;
    
    
    
    uint16_t GetAid (void) const;
    bool GetTransmissionSuccess (void) const;
    uint64_t GetEstimateNextTransmissionId (void) const;
    uint8_t GetStaType (uint8_t statype);
    uint16_t GetDataRate (uint16_t m_dataRate);
    uint16_t GetPacketSize (uint16_t m_packetSize);
    
    void SetNumPacketsReceived (uint16_t numReceived);
    uint16_t GetNumPacketsReceived (void) const;
    
    uint16_t GetTransInOneBeacon (void) const;
    void SetTransInOneBeacon (uint16_t num);


    std::vector<uint16_t> m_transIntervalList;
    uint16_t m_transIntervalListSize;
    
    UpdateInfo m_snesorUpdatInfo;
    
private:
    uint64_t m_transmissionInterval; // beacon as unit
    uint64_t m_transInOneBeacon; // beacon as unit

    uint64_t last_transmissionInterval;
    uint64_t m_nextTransmissionId; // beacon as unit
    uint16_t m_packetSize;
    uint16_t m_dataRate;
    uint8_t  m_staType;
    uint16_t m_aid;
    bool m_transmissionSuccess;
    uint64_t m_transmissionIntervalMin;
    uint64_t m_transmissionIntervalMax;
    
    uint16_t m_receivedNum;
    uint64_t last2_transmissionInterval;
    
    uint16_t m_index;
    
    bool m_everSuccess;
};
    
class OffloadStation
{
public:
    OffloadStation ();
    virtual ~OffloadStation ();
    
    void SetAid (uint16_t aid); //to indentify station
    void SetTransmissionSuccess (bool success);
    void SetOffloadStaActive (bool active);
    void IncreaseFailedTransmissionCount (bool add); //"1" add, "0" reset.
    void SetStaType (uint8_t statype);
    void SetDataRate (uint16_t m_dataRate); //question, how to get datarate info? Impossibile
    void SetPacketSize (uint16_t m_packetSize); //necessary?
    
    
    uint16_t GetAid (void) const;
    bool GetTransmissionSuccess (void) const;
    bool GetOffloadStaActive (void) const;
    uint16_t GetFailedTransmissionCount (void) const;
    uint64_t GetEstimateTransmissionInterval (void) const;
    uint64_t GetEstimateNextTransmission (void) const;
    uint8_t GetStaType (uint8_t statype);
    uint16_t GetDataRate (uint16_t m_dataRate);
    uint16_t GetPacketSize (uint16_t m_packetSize);
    
private:
    uint64_t m_transmissionInterval; // beacon as unit
    uint64_t m_nextTransmission; // beacon as unit
    uint16_t m_packetSize;
    uint16_t m_dataRate;
    uint8_t  m_staType;
    uint16_t m_aid;
    bool m_transmissionSuccess;
    bool m_offloadStaActive;
    uint16_t m_offloadFailedCount; //reset once transmission succeed
    //failure here menas AP receives no packets.
};

class S1gRawCtr
{
public:
  static TypeId GetTypeId (void);

  S1gRawCtr ();
  virtual ~S1gRawCtr ();

    RPS  UpdateRAWGroupping (std::vector<uint16_t> m_aidlist, std::vector<uint16_t> m_aidOffloadList, std::vector<uint16_t> m_receivedAid, uint64_t BeaconInterval, std::string outputpath);

  void configureRAW ();
  RPS GetRPS ();
    
  void deleteRps ();
  void UdpateSensorStaInfo (std::vector<uint16_t> m_sensorlist, std::vector<uint16_t> m_receivedAid, std::string outputpath); //need to change, controlled by AP
  void UdpateOffloadStaInfo (std::vector<uint16_t> m_OffloadList, std::vector<uint16_t>  receivedStas, std::string outputpath);
  void calculateActiveOffloadSta ();
  void SetOffloadAllowedToSend ();
  
    
  void calculateRawSlotDuration (uint16_t numsta, uint16_t successprob); //nedd to be extended to support more felxibility.

  void calculateSensorNumWantToSend (void);
  void calculateMaybeAirtime (void);
  void SetSensorAllowedToSend (void);
  Sensor * LookupSensorSta (uint16_t aid);
  OffloadStation * LookupOffloadSta (uint16_t aid); //can be combined with function LookupSensorSta.
    
  std::vector<uint16_t>::iterator  LookupLastTransmission (uint16_t aid);
  
    
    typedef std::vector<Sensor *> Stations;
    typedef std::vector<Sensor *>::iterator StationsCI;
    Stations m_stations;
    
    typedef std::vector<OffloadStation *> OffloadStations;
    typedef std::vector<OffloadStation *>::iterator OffloadStationsCI;
    OffloadStations m_offloadStations;
    
    
    uint16_t MaxSlotForSensor;
    
    std::vector<uint16_t> m_aidList; //stations allowed to transmit in last beacon
    std::vector<uint16_t>::iterator m_aidListCI;
    std::vector<uint16_t> m_aidOffloadList;
    std::vector<uint16_t>::iterator m_aidOffloadListCI;
    std::vector<uint16_t> m_lastTransmissionList;
    
private:
    
  uint64_t m_rawslotDuration; //us
  uint64_t m_maybeAirtimeSensor;
  uint16_t m_numSensorAllowedToSend;
  uint16_t m_numSensorWantToSend;
    
  uint16_t  m_numSendSensorAllowed;
  uint16_t  m_numSendSensorWant;

   
  uint64_t m_offloadRawslotDuration;
  uint16_t m_numOffloadStaActive;
  uint16_t m_offloadFailedMax;
  uint16_t m_numOffloadAllowedToSend;
    
  uint64_t m_beaconInterval;
  uint16_t  sensorpacketsize;
  uint16_t  offloadpacketsize;
    
  uint16_t  m_slotDurationCount;
  uint64_t m_beaconOverhead;
    
  uint64_t currentId;
  std::vector<RPS::RawAssignment *> RawAssignmentList;
    
  uint16_t RpsIndex;
  RPS * m_rps;
  RPSVector rpslist;
    
    bool  m_receivedsuccess;
    
    std::string  sensorfile;
};

} //namespace ns3

#endif /* S1G_RAW_CTR_H */
