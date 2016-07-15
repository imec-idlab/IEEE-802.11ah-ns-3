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

#include "ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/log.h"
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
#include "ns3/uinteger.h"
#include "wifi-mac-queue.h"

#include "s1g-raw-control.h"

#include <iostream>     // std::cout
#include <algorithm>    // std::find
#include <vector>       // std::vector
#include <fstream>
#include <string>

#include <stdio.h>
#include <valarray>
#include <stdlib.h>
#include <math.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("S1gRawCtr");

//NS_OBJECT_ENSURE_REGISTERED (S1gRawCtr);
    
//** AP update info after RAW ends(right before next beacon is sent)
//list of sensor allowed to transmit in last beacon ************
Sensor::Sensor ()
{
  last_transmissionInterval = 0;
    m_transmissionIntervalMax =1;
    m_transmissionIntervalMin = 1;
}
    
Sensor::~Sensor ()
{
}


void
Sensor::EstimateNextTransmissionId (uint64_t m_nextId)
{
    m_nextTransmissionId = m_nextId;
}

    
void
Sensor::SetAid (uint16_t aid)
{
    m_aid = aid;
}
    
void
Sensor::SetTransmissionSuccess (bool success)
{
    m_transmissionSuccess = success;
}
    
uint16_t
Sensor::GetAid (void) const
{
    return m_aid;
}
    
bool
Sensor::GetTransmissionSuccess (void) const
{
    return m_transmissionSuccess;
}
    
uint64_t
Sensor::GetEstimateNextTransmissionId (void) const
{
    return m_nextTransmissionId;
}
    
void
Sensor::SetEverSuccess(bool success)
{
    m_everSuccess = success;
}
bool
Sensor::GetEverSuccess (void) const
{
    return m_everSuccess;
}
    
//OffloadStation
OffloadStation::OffloadStation ():
                m_offloadFailedCount (0)
{
}
    
OffloadStation::~OffloadStation ()
{
}

void
OffloadStation::SetAid (uint16_t aid)
{
    m_aid = aid;
}

void
OffloadStation::SetTransmissionSuccess (bool success)
{
    m_transmissionSuccess = success;
}

void
OffloadStation::SetOffloadStaActive (bool active)
{
    m_offloadStaActive = active;
}

void
OffloadStation::IncreaseFailedTransmissionCount (bool add)
{
    if (add)
     {
        m_offloadFailedCount++;
     }
    else
     {
        m_offloadFailedCount = 0;
     }
}


uint16_t
OffloadStation::GetAid (void) const
{
    return m_aid;
}

bool
OffloadStation::GetTransmissionSuccess (void) const
{
    return m_transmissionSuccess;
}

bool
OffloadStation::GetOffloadStaActive (void) const
{
    return m_offloadStaActive;
}

uint16_t
OffloadStation::GetFailedTransmissionCount (void) const
{
    return m_offloadFailedCount;
}

//
S1gRawCtr::S1gRawCtr ()
{
   RpsIndex = 0;
   m_offloadFailedMax = 5;
   sensorpacketsize = 256;
   offloadpacketsize = 256;
    m_rawslotDuration = (100*120) +500; //for test.
    m_offloadRawslotDuration = (100*120) +500;
    currentId = 1;

}

S1gRawCtr::~S1gRawCtr ()
{
}

void
S1gRawCtr::UdpateSensorStaInfo (std::vector<uint16_t> m_sensorlist, std::vector<uint16_t> m_receivedAid, std::string outputpath)
{
   //initialization
  //if sorted queue is empty, put all sensor into queue
  std::string ApNode;
  std::ofstream outputfile;
  std::ostringstream APId;
    
  uint16_t numsensor = m_sensorlist.size (); //need to be improved
  /*if (m_stations.size() < numsensor)
     {
      for (uint16_t i = m_stations.size(); i < numsensor; i++)
        {
            Sensor * m_sta = new Sensor;
            m_sta->SetAid (i+1);
            m_sta->EstimateNextTransmissionId (currentId+1);
            //NS_LOG_UNCOND ("initial, set next id = " << currentId);
            //initialize UpdateInfo struct
            m_sta->SetEverSuccess (false);
            m_sta->m_snesorUpdatInfo = (UpdateInfo){currentId,currentId,currentId,false,currentId,currentId,currentId,false};
            //
            m_stations.push_back (m_sta); // should create a Dispose function?
            //
            APId.clear ();
            APId.str ("");
            APId << (i+1);
            ApNode = APId.str();
            sensorfile = outputpath + ApNode + ".txt";
            outputfile.open (sensorfile, std::ios::out | std::ios::trunc);
            outputfile.close();
            //
        }
     } */
    
    for (std::vector<uint16_t>::iterator ci = m_sensorlist.begin(); ci != m_sensorlist.end(); ci++)
    {
        if (LookupSensorSta (*ci) == nullptr)
        {
            Sensor * m_sta = new Sensor;
            m_sta->SetAid (*ci);
            m_sta->EstimateNextTransmissionId (currentId+1);
            //NS_LOG_UNCOND ("initial, set next id = " << currentId);
            //initialize UpdateInfo struct
            m_sta->SetEverSuccess (false);
            m_sta->m_snesorUpdatInfo = (UpdateInfo){currentId,currentId,currentId,false,currentId,currentId,currentId,false};
            //
            m_stations.push_back (m_sta); // should create a Dispose function?
            //
            APId.clear ();
            APId.str ("");
            APId << (*ci);
            ApNode = APId.str();
            sensorfile = outputpath + ApNode + ".txt";
            outputfile.open (sensorfile, std::ios::out | std::ios::trunc);
            outputfile.close();
            //
            m_lastTransmissionList.push_back (*ci);
        }
    }
    
    NS_LOG_UNCOND ("m_aidList.size() = " << m_aidList.size() << ", m_receivedAid = " << m_receivedAid.size () << ", m_stations.size() = " << m_stations.size() << ", currentId = " << currentId);

    //update transmission interval info, stations allowed to transmit in last beacon
    for (std::vector<uint16_t>::iterator it = m_aidList.begin(); it != m_aidList.end(); it++)
     {
        Sensor * stationTransmit = LookupSensorSta (*it);
         if (stationTransmit == nullptr)
         {
             return;
         }
         
         m_receivedsuccess = false;
         for (std::vector<uint16_t>::iterator ci = m_receivedAid.begin(); ci != m_receivedAid.end(); ci++)
         {

             if (*ci == *it)
              {
                NS_LOG_UNCOND ("stations of aid " << *it << " received, " << *ci);
                stationTransmit->SetTransmissionSuccess (true);
                  //output to files.
                  //write into currentID  coulumn
                  //write  into allowed to send coulumn
                  //write  into reaeived to receive coulumn
                  APId.clear ();
                  APId.str ("");
                  APId << (*it);
                  ApNode = APId.str();
                  
                  sensorfile = outputpath + ApNode + ".txt";
                  outputfile.open (sensorfile, std::ios::out | std::ios::app);
                  outputfile << currentId << "\t" << "1" << "\t" << "1" << "\n";
                  outputfile.close();
                  //
                  
                if (stationTransmit->GetEverSuccess () == false)
                  {
                      stationTransmit->m_snesorUpdatInfo = (UpdateInfo){currentId-1,currentId-1,currentId-1,false,currentId-1,currentId-1,currentId-1,false};
                      stationTransmit->SetEverSuccess (true);
                  }
                 
                stationTransmit->m_snesorUpdatInfo.lastTryBFpreSuccessId = stationTransmit->m_snesorUpdatInfo.lastTryBFCurrentSuccessId; //update, swith current to pre
                 
                stationTransmit->m_snesorUpdatInfo.preSuccessId = stationTransmit->m_snesorUpdatInfo.CurrentSuccessId; //update, swith current to pre
                stationTransmit->m_snesorUpdatInfo.CurrentSuccessId = currentId;
                  
                  stationTransmit->m_snesorUpdatInfo.lastTryBFCurrentSuccessId = std::max(stationTransmit->m_snesorUpdatInfo.preSuccessId, stationTransmit->m_snesorUpdatInfo.CurrentUnSuccessId);
                 
                stationTransmit->m_snesorUpdatInfo.preTrySuccess = stationTransmit->m_snesorUpdatInfo.CurrentTrySuccess;
                stationTransmit->m_snesorUpdatInfo.CurrentTrySuccess = true;
                goto EstimateInterval;
              }
         }
         
       
            NS_LOG_UNCOND ("stations of aid " << *it << " not received");
            stationTransmit->SetTransmissionSuccess (false);
            //output to files.
            //write into currentID  coulumn
            //write  into allowed to send coulumn
            //write  into received to receive coulumn
            APId.clear ();
            APId.str ("");
            APId << (*it);
            ApNode = APId.str();
         
            sensorfile = outputpath + ApNode + ".txt";
            outputfile.open (sensorfile, std::ios::out | std::ios::app);
            outputfile << currentId << "\t" << "1" << "\t" << "0" << "\n";
            outputfile.close();
            //
         
            stationTransmit->m_snesorUpdatInfo.preUnsuccessId = stationTransmit->m_snesorUpdatInfo.CurrentUnSuccessId; //update, swith current to pre
            stationTransmit->m_snesorUpdatInfo.CurrentUnSuccessId = currentId;
              
            stationTransmit->m_snesorUpdatInfo.preTrySuccess = stationTransmit->m_snesorUpdatInfo.CurrentTrySuccess;
            stationTransmit->m_snesorUpdatInfo.CurrentTrySuccess = false;
         
    EstimateInterval :
           stationTransmit->EstimateTransmissionInterval (currentId);
         
     }
}
    
void
Sensor::EstimateTransmissionInterval (uint64_t currentId)
{
    if (m_snesorUpdatInfo.preTrySuccess && m_transmissionSuccess)
    {
        m_transmissionInterval = last_transmissionInterval;
    }
    else if (m_transmissionSuccess == true)
    {
        m_transmissionIntervalMin = m_snesorUpdatInfo.CurrentUnSuccessId - m_snesorUpdatInfo.preSuccessId + 1;
        m_transmissionIntervalMax = currentId - m_snesorUpdatInfo.lastTryBFpreSuccessId - 1;
    }
    else
    {
        m_transmissionIntervalMin = currentId - m_snesorUpdatInfo.CurrentSuccessId + 1;
        //m_transmissionIntervalMax remains
    }
    if (m_transmissionIntervalMax < m_transmissionIntervalMin)
     {
         m_transmissionIntervalMax = 2 * m_transmissionIntervalMin;
     }
    m_transmissionInterval = (m_transmissionIntervalMin + m_transmissionIntervalMax)/2;
    uint64_t m_nextId = m_snesorUpdatInfo.CurrentSuccessId + m_transmissionInterval; ////** AP update info after RAW ends(right before next beacon is sent)
    EstimateNextTransmissionId (m_nextId);
    //NS_LOG_UNCOND (m_snesorUpdatInfo.lastTryBFpreSuccessId << "," << m_snesorUpdatInfo.lastTryBFCurrentSuccessId << ", " << m_snesorUpdatInfo.preSuccessId << ", " << currentId << ", " << m_snesorUpdatInfo.preTrySuccess << ", " << m_snesorUpdatInfo.preUnsuccessId);
    
    //NS_LOG_UNCOND ("m_transmissionIntervalMin = " << m_transmissionIntervalMin << ", m_transmissionIntervalMin = " << m_transmissionIntervalMax);
    
}


void
S1gRawCtr::calculateSensorNumWantToSend ()
{
    m_numSensorWantToSend = 0;

    for (StationsCI it = m_stations.begin(); it != m_stations.end(); it++)
      {
          //NS_LOG_UNCOND ("calculateSensorNumWantToSend " << (*it)->GetAid () <<  "," << (*it)->GetEstimateNextTransmissionId () << ", " <<currentId);
          NS_ASSERT ((*it)->GetEstimateNextTransmissionId () >= currentId);
        if ((*it)->GetEstimateNextTransmissionId () == currentId)
          m_numSensorWantToSend++;
      }
}
 
void
S1gRawCtr::calculateMaybeAirtime ()
{
    uint16_t sensortime = m_numSensorWantToSend * sensorpacketsize; //to do, define sensorpacketsize
    //uint16_t sensortime = std::max(sensortimeP, uint16_t (1));

    uint16_t offloadtime = m_numOffloadStaActive * offloadpacketsize; //to do, define offloadpacketsize
    //uint16_t offloadtime = std::max(offloadtimeP, uint16_t (1));
    if (sensortime + offloadtime == 0)
     {
        m_maybeAirtimeSensor = 0; //to do, define BeaconInterval;
         return;
     }
    m_maybeAirtimeSensor = m_beaconInterval * sensortime / (sensortime + offloadtime); //to do, define BeaconInterval;
}
/*
void
S1gRawCtr::SetSensorAllowedToSend ()
{
    m_aidList.clear (); //Re assign slot to stations
    uint16_t numAllowed = m_maybeAirtimeSensor/m_rawslotDuration;

    m_numSensorAllowedToSend =  std::min(m_numSensorWantToSend, numAllowed);
    //work here
    //m_numSensorAllowedToSend =  m_numSensorWantToSend; //to change
    NS_LOG_UNCOND ("-------------------------------------------start");
    NS_LOG_UNCOND ("SetSensorAllowedToSend () = " << m_numSensorAllowedToSend );

    for (StationsCI it = m_stations.begin(); it != m_stations.end(); it++)
     {
        if ((*it)->GetEstimateNextTransmissionId () == currentId)
         {
            if (m_aidList.size () == m_numSensorAllowedToSend)
              {
                  (*it)->EstimateNextTransmissionId (currentId+1);
              }
             else
              {
                 m_aidList.push_back((*it)->GetAid ());
                 std::vector<uint16_t>::iterator position = LookupLastTransmission ((*it)->GetAid ());
                  m_lastTransmissionList.erase (position);
                  m_lastTransmissionList.push_back ((*it)->GetAid ());
                  
              }
             ////put first m_numSensorAllowedToSend stations to m_aidList. To do, choose stations based on last transmission time.
         }
     }
}*/
    

 void
 S1gRawCtr::SetSensorAllowedToSend ()
 {
   m_aidList.clear (); //Re assign slot to stations
   uint16_t numAllowed = m_maybeAirtimeSensor/m_rawslotDuration;
 
   m_numSensorAllowedToSend =  std::min(m_numSensorWantToSend, numAllowed);
   //work here
   //m_numSensorAllowedToSend =  m_numSensorWantToSend; //to change
   NS_LOG_UNCOND ("-------------------------------------------start");
   NS_LOG_UNCOND ("SetSensorAllowedToSend () = " << m_numSensorAllowedToSend );
 
   std::vector<uint16_t>::iterator it = m_lastTransmissionList.begin();
   //for (std::vector<uint16_t>::iterator it = m_lastTransmissionList.begin(); it != m_lastTransmissionList.end(); it++)
    for (uint16_t i = 0; i < m_lastTransmissionList.size (); i++)
     {
         //NS_LOG_UNCOND ("aa m_lastTransmissionList () = " << *it );

         Sensor * stationTransmit = LookupSensorSta (*it);
         it++;
       if (stationTransmit->GetEstimateNextTransmissionId () == currentId)
        {
           if (m_aidList.size () == m_numSensorAllowedToSend)
             {
                stationTransmit->EstimateNextTransmissionId (currentId+1);
             }
           else
            {
               it--;
               m_aidList.push_back(stationTransmit->GetAid ());
               //NS_LOG_UNCOND ("allowed to send () = " << stationTransmit->GetAid () );

               std::vector<uint16_t>::iterator position = LookupLastTransmission (stationTransmit->GetAid ());
                //NS_LOG_UNCOND ("m_lastTransmissionList.size () = " << m_lastTransmissionList.size ());

               m_lastTransmissionList.erase (position);
                //NS_LOG_UNCOND ("m_lastTransmissionList.size ()-- = " << m_lastTransmissionList.size ());

               m_lastTransmissionList.push_back (stationTransmit->GetAid ());
            }
         ////put first m_numSensorAllowedToSend stations to m_aidList. To do, choose stations based on last transmission time.
        }
    }
     
 }

std::vector<uint16_t>::iterator
S1gRawCtr::LookupLastTransmission (uint16_t aid)
{
    
    for (std::vector<uint16_t>::iterator it = m_lastTransmissionList.begin(); it != m_lastTransmissionList.end(); it++)
    {
       // NS_LOG_UNCOND ("m_lastTransmissionList = " << *it);
    }
    
  for (std::vector<uint16_t>::iterator it = m_lastTransmissionList.begin(); it != m_lastTransmissionList.end(); it++)
    {
        if (aid == *it)
        {
            return it;
        }
    }
    
  NS_ASSERT ("ERROR OCUURS");
}
    
    

  

//offload
void
S1gRawCtr::UdpateOffloadStaInfo (std::vector<uint16_t> m_OffloadList, std::vector<uint16_t> m_receivedAid, std::string outputpath)
//to do
//need to change Ap-wifi-mac.cc to get numsensor info
//need to get successful transmission info.
//this info can be got from static configuration or via association, should from association
{
    //initialization
    //if sorted queue is empty, put all offload stations into queue
    
    /*uint16_t offloadstaNum = m_OffloadList.size (); //need to be improved
    if (m_offloadStations.size() < offloadstaNum)
    {
        for (uint16_t i = m_offloadStations.size(); i < offloadstaNum; i++)
        {
            OffloadStation * m_offloadSta = new OffloadStation;
            m_offloadSta->SetAid (6); //for test
            m_offloadSta->SetOffloadStaActive (true);
            m_offloadSta->IncreaseFailedTransmissionCount (0);
            m_offloadStations.push_back (m_offloadSta); // should create a Dispose function?
            NS_LOG_UNCOND ("m_offloadStations.size () = " << m_offloadStations.size ());

        }
        //return;
    } should be removed*/
    
    
    std::string ApNode;
    std::ofstream outputfile;
    std::ostringstream APId;

    
    for (std::vector<uint16_t>::iterator ci = m_OffloadList.begin(); ci != m_OffloadList.end(); ci++)
     {
        if (LookupOffloadSta (*ci) == nullptr)
          {
              OffloadStation * m_offloadSta = new OffloadStation;
              m_offloadSta->SetAid (*ci);
              m_offloadSta->SetOffloadStaActive (true);
              m_offloadSta->IncreaseFailedTransmissionCount (0);
              m_offloadStations.push_back (m_offloadSta); // should create a Dispose function?
              NS_LOG_UNCOND ("m_offloadStations.size () = " << m_offloadStations.size ());
              
              APId.clear ();
              APId.str ("");
              APId << (*ci);
              ApNode = APId.str();
              sensorfile = outputpath + ApNode + ".txt";
              outputfile.open (sensorfile, std::ios::out | std::ios::trunc);
              outputfile.close();
          }
     }
    
    //update active offload stations' info.
    for (std::vector<uint16_t>::iterator it = m_aidOffloadList.begin(); it != m_aidOffloadList.end(); it++)
    {
        OffloadStation * OffloadStaTransmit = LookupOffloadSta (*it);
        
        for (std::vector<uint16_t>::iterator ci = m_receivedAid.begin(); ci != m_receivedAid.end(); ci++)
        {
            if (*ci == *it)
            {
                NS_LOG_UNCOND ("stations of aid " << *it << " received, " << *ci);
                //output to files.
                APId.clear ();
                APId.str ("");
                APId << (*it);
                ApNode = APId.str();
                sensorfile = outputpath + ApNode + ".txt";
                outputfile.open (sensorfile, std::ios::out | std::ios::app);
                outputfile << currentId << "\t" << "1" << "\t" << "1" << "\n";
                outputfile.close();
                //
                OffloadStaTransmit->SetTransmissionSuccess (true);
                OffloadStaTransmit->IncreaseFailedTransmissionCount (1);
                goto FailedMax;
            }
        }
        
        //output to files.
        NS_LOG_UNCOND ("stations of aid " << *it << " not received");
        APId.clear ();
        APId.str ("");
        APId << (*it);
        ApNode = APId.str();
        sensorfile = outputpath + ApNode + ".txt";
        outputfile.open (sensorfile, std::ios::out | std::ios::app);
        outputfile << currentId << "\t" << "1" << "\t" << "0" << "\n";
        outputfile.close();
        //
        
        OffloadStaTransmit->SetTransmissionSuccess (false);
        OffloadStaTransmit->IncreaseFailedTransmissionCount (0);
    FailedMax:
        if (OffloadStaTransmit->GetFailedTransmissionCount () == m_offloadFailedMax)
            OffloadStaTransmit->SetOffloadStaActive (false);
        
        OffloadStaTransmit->SetOffloadStaActive (true); //for test
    }
}
 
void
S1gRawCtr::calculateActiveOffloadSta ()
{
    m_numOffloadStaActive = 0;
    for (OffloadStationsCI it = m_offloadStations.begin(); it != m_offloadStations.end(); it++)
    {
        if ((*it)->GetOffloadStaActive () == true)
            m_numOffloadStaActive++;
    }
}
 
void
S1gRawCtr::SetOffloadAllowedToSend ()
{
    m_aidOffloadList.clear (); //Re assign slot to stations
    m_offloadRawslotDuration = (100*120)+500;
    uint16_t numAllowed = ((m_beaconInterval-100) - m_numSensorAllowedToSend * m_rawslotDuration)/m_offloadRawslotDuration;
    //NS_LOG_UNCOND ("SetOffloadAllowedToSend,  numAllowed= " << numAllowed << ", m_numOffloadStaActive = " << m_numOffloadStaActive);

    m_numOffloadAllowedToSend =  std::min(m_numOffloadStaActive, numAllowed);
    if (m_numOffloadAllowedToSend == 0)
      {
          return;
      }
    if (m_numOffloadAllowedToSend < numAllowed)
      {
        m_offloadRawslotDuration = ((m_beaconInterval-100) - m_numSensorAllowedToSend * m_rawslotDuration)/m_numOffloadAllowedToSend;
      }
    //m_numOffloadAllowedToSend = 1; //for test
    NS_LOG_UNCOND ("SetOffloadAllowedToSend ()= " << m_numOffloadAllowedToSend);

    for (OffloadStationsCI it = m_offloadStations.begin(); it != m_offloadStations.end(); it++)
    {
        if ((*it)->GetOffloadStaActive ())
         {
            m_aidOffloadList.push_back((*it)->GetAid ()); //put first m_numSensorAllowedToSend stations to m_aidList. To do, choose stations based on last transmission time.
            if (m_aidOffloadList.size () == m_numOffloadAllowedToSend)
                return;
         }
    }
}
    
RPS
S1gRawCtr::GetRPS ()
{
 if (RpsIndex < rpslist.rpsset.size())
    {
        m_rps = rpslist.rpsset.at(RpsIndex);
        NS_LOG_DEBUG ("< RpsIndex =" << RpsIndex);
        RpsIndex++;
    }
  else
    {
        m_rps = rpslist.rpsset.at(0);
        NS_LOG_DEBUG ("RpsIndex =" << RpsIndex);
        RpsIndex = 1;
    }
  return *m_rps;
}

// Beacon duration), before that use NGroup=1 and initialize by ap-wifi-mac
RPS
S1gRawCtr::UpdateRAWGroupping (std::vector<uint16_t> m_sensorlist, std::vector<uint16_t> m_OffloadList, std::vector<uint16_t> m_receivedAid, uint64_t BeaconInterval, std::string outputpath)
 {
     m_beaconInterval = BeaconInterval;
     //currentId++; //beaconInterval counter
     //work here
     UdpateSensorStaInfo (m_sensorlist,  m_receivedAid, outputpath);
     UdpateOffloadStaInfo (m_OffloadList, m_receivedAid, outputpath);
     //NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingaa =");
     currentId++; //next id, actually
     calculateSensorNumWantToSend ();
     calculateActiveOffloadSta ();
     //NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingbb =");


     calculateMaybeAirtime ();
     //NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingdd =");

     SetSensorAllowedToSend ();
     SetOffloadAllowedToSend ();
     
     //NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingcc =");

     configureRAW ();
     return GetRPS ();
     
}
    
    /*
     void
     S1gRawCtr::configureRAW (RPS * m_rps, RPS::RawAssignment *  m_raw )
     {
     uint16_t RawControl = 0;
     uint16_t SlotCrossBoundary = 1;
     uint16_t SlotFormat = 1;
     uint16_t SlotDurationCount;
     uint16_t SlotNum = 1;
     uint16_t page = 0;
     uint32_t aid_start = 0;
     uint32_t aid_end = 0;
     uint16_t rawinfo;
     

     
     SlotDurationCount = (m_beaconInterval/(SlotNum) - 500)/120;
     NS_ASSERT (SlotDurationCount <= 2037);
     
     
     //RPS * m_rps = new RPS;
     //RPS * m_rps;
     
     //Release storage.
         //NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingee =");

         //RPS::RawAssignment *m_raw = new RPS::RawAssignment;
     
     m_raw->SetRawControl (RawControl);//support paged STA or not
     m_raw->SetSlotCrossBoundary (SlotCrossBoundary);
     m_raw->SetSlotFormat (1);
     m_raw->SetSlotDurationCount (300);//to change
     m_raw->SetSlotNum (SlotNum);
     
     aid_start = 1;
     aid_end = 1;
     rawinfo = (aid_end << 13) | (aid_start << 2) | page;
     m_raw->SetRawGroup (rawinfo);
     
        // NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppingff =");

     m_rps->SetRawAssignment(*m_raw);
     
        // NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppinggg =");

     rpslist.rpsset.push_back (m_rps); //only one RPS in rpslist actually, update info
        // NS_LOG_UNCOND ("S1gRawCtr::UpdateRAWGrouppinghh =");

 } */

//configure RAW based on grouping result

void
S1gRawCtr::configureRAW ( )
{
    uint16_t RawControl = 0;
    uint16_t SlotCrossBoundary = 1;
    uint16_t SlotFormat = 1;
    uint16_t SlotDurationCount;
    uint16_t SlotNum = 1;
    uint16_t page = 0;
    uint32_t aid_start = 0;
    uint32_t aid_end = 0;
    uint32_t rawinfo;
    
    m_rps = new RPS;
    
    uint16_t NGroups = m_aidList.size () + m_aidOffloadList.size();
    if (NGroups == 0)
      {
        SlotDurationCount = 100;
        RPS::RawAssignment *m_raw = new RPS::RawAssignment;
        
        m_raw->SetRawControl (RawControl);
        m_raw->SetSlotCrossBoundary (SlotCrossBoundary);
        m_raw->SetSlotFormat (SlotFormat);
        m_raw->SetSlotDurationCount (SlotDurationCount);
        m_raw->SetSlotNum (SlotNum);
        
        aid_start = 1;
        aid_end = 1;
        rawinfo = (aid_end << 13) | (aid_start << 2) | page;
        m_raw->SetRawGroup (rawinfo);
    
        m_rps->SetRawAssignment(*m_raw);
        delete m_raw;
        rpslist.rpsset.push_back (m_rps);
        return;
      }
    SlotDurationCount = ((m_beaconInterval-100)/(SlotNum*NGroups) - 500)/120;
    NS_ASSERT (SlotDurationCount <= 2037);
    SlotDurationCount = (m_rawslotDuration - 500)/120;
  
    
    if (m_rps != nullptr)
      {
        delete m_rps;
      }
    
    //Release storage.
    rpslist.rpsset.clear();

    
    //NS_LOG_UNCOND ("m_aidList =" << m_aidList.size () << ", m_aidOffloadList=" << m_aidOffloadList.size ());
    
    for (std::vector<uint16_t>::iterator it = m_aidList.begin(); it != m_aidList.end(); it++)
      {

          RPS::RawAssignment *m_raw = new RPS::RawAssignment;
          
          m_raw->SetRawControl (RawControl);//support paged STA or not
          m_raw->SetSlotCrossBoundary (SlotCrossBoundary);
          m_raw->SetSlotFormat (SlotFormat);
          m_raw->SetSlotDurationCount (SlotDurationCount);//to change
          m_raw->SetSlotNum (SlotNum);
          
          aid_start = *it;
          aid_end = *it;
          NS_LOG_UNCOND ("sensor, aid_start =" << aid_start << ", aid_end=" << aid_end << ", SlotDurationCount = " << SlotDurationCount);

          rawinfo = (aid_end << 13) | (aid_start << 2) | page;
          m_raw->SetRawGroup (rawinfo);
          
          m_rps->SetRawAssignment(*m_raw);
          delete m_raw;
      }
    
    
    uint64_t aidtime = (m_beaconInterval-100) - (SlotDurationCount * 120 + 500) * m_aidList.size();
    uint64_t offloadcount = (aidtime - 500)/120;
    offloadcount = (m_offloadRawslotDuration - 500)/120;
    //printf("offloadcount is %u\n",  offloadcount);


    for (std::vector<uint16_t>::iterator it = m_aidOffloadList.begin(); it != m_aidOffloadList.end(); it++)
     {
        RPS::RawAssignment *m_raw2 = new RPS::RawAssignment;
        
        m_raw2->SetRawControl (RawControl);//support paged STA or not
        m_raw2->SetSlotCrossBoundary (SlotCrossBoundary);
        m_raw2->SetSlotFormat (SlotFormat);
        m_raw2->SetSlotDurationCount (offloadcount); //to change
        m_raw2->SetSlotNum (SlotNum);
        
        aid_start = *it;
        aid_end = *it;
        rawinfo = (aid_end << 13) | (aid_start << 2) | page;
        NS_LOG_UNCOND ("offload, aid_start =" << aid_start << ", aid_end=" << aid_end << ", offloadcount =" << offloadcount);

        m_raw2->SetRawGroup (rawinfo);
        m_rps->SetRawAssignment(*m_raw2);
        delete m_raw2;
     }

    
    rpslist.rpsset.push_back (m_rps); //only one RPS in rpslist actually, update info every beacon in this algorithm.
    printf("rpslist.rpsset.size is %u\n",  rpslist.rpsset.size());

    //delete m_rps;
}
    
//what if lookup fails, is it possibile?
Sensor *
S1gRawCtr::LookupSensorSta (uint16_t aid)
{
   for (StationsCI it = m_stations.begin(); it != m_stations.end(); it++)
   {
       if (aid == (*it)->GetAid ())
        {
           return (*it);
        }
   }
    return nullptr;
}
    
//what if lookup fails, is it possibile?
OffloadStation *
S1gRawCtr::LookupOffloadSta (uint16_t aid)
{
    for (OffloadStationsCI it = m_offloadStations.begin(); it != m_offloadStations.end(); it++)
    {
        if (aid == (*it)->GetAid ())
        {
            return (*it);
        }
    }
    return nullptr;
}

void
S1gRawCtr::calculateRawSlotDuration (uint16_t numsta, uint16_t successprob)
{
    m_rawslotDuration = (100*120)+500; //for test.
    m_offloadRawslotDuration = (100*120)+500;
}

} //namespace ns3
