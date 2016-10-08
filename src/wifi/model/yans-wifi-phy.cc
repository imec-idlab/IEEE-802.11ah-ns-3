/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 *          Ghada Badawy <gbadawy@gmail.com>
 */

#include "yans-wifi-phy.h"
#include "yans-wifi-channel.h"
#include "wifi-mode.h"
#include "wifi-preamble.h"
#include "wifi-phy-state-helper.h"
#include "error-rate-model.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/uinteger.h"
#include "ns3/enum.h"
#include "ns3/pointer.h"
#include "ns3/net-device.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/boolean.h"
#include "ns3/node.h"
#include "ampdu-tag.h"
#include <cmath>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("YansWifiPhy");

NS_OBJECT_ENSURE_REGISTERED (YansWifiPhy);

TypeId
YansWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::YansWifiPhy")
    .SetParent<WifiPhy> ()
    .SetGroupName ("Wifi")
    .AddConstructor<YansWifiPhy> ()
    .AddAttribute ("EnergyDetectionThreshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to detect the signal.",
                   DoubleValue (-96.0),
                   MakeDoubleAccessor (&YansWifiPhy::SetEdThreshold,
                                       &YansWifiPhy::GetEdThreshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("CcaMode1Threshold",
                   "The energy of a received signal should be higher than "
                   "this threshold (dbm) to allow the PHY layer to declare CCA BUSY state.",
                   DoubleValue (-99.0),
                   MakeDoubleAccessor (&YansWifiPhy::SetCcaMode1Threshold,
                                       &YansWifiPhy::GetCcaMode1Threshold),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxGain",
                   "Transmission gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&YansWifiPhy::SetTxGain,
                                       &YansWifiPhy::GetTxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxGain",
                   "Reception gain (dB).",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&YansWifiPhy::SetRxGain,
                                       &YansWifiPhy::GetRxGain),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerLevels",
                   "Number of transmission power levels available between "
                   "TxPowerStart and TxPowerEnd included.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&YansWifiPhy::m_nTxPower),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("TxPowerEnd",
                   "Maximum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&YansWifiPhy::SetTxPowerEnd,
                                       &YansWifiPhy::GetTxPowerEnd),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TxPowerStart",
                   "Minimum available transmission level (dbm).",
                   DoubleValue (16.0206),
                   MakeDoubleAccessor (&YansWifiPhy::SetTxPowerStart,
                                       &YansWifiPhy::GetTxPowerStart),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("RxNoiseFigure",
                   "Loss (dB) in the Signal-to-Noise-Ratio due to non-idealities in the receiver."
                   " According to Wikipedia (http://en.wikipedia.org/wiki/Noise_figure), this is "
                   "\"the difference in decibels (dB) between"
                   " the noise output of the actual receiver to the noise output of an "
                   " ideal receiver with the same overall gain and bandwidth when the receivers "
                   " are connected to sources at the standard noise temperature T0 (usually 290 K)\".",
                   DoubleValue (7),
                   MakeDoubleAccessor (&YansWifiPhy::SetRxNoiseFigure,
                                       &YansWifiPhy::GetRxNoiseFigure),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("State",
                   "The state of the PHY layer.",
                   PointerValue (),
                   MakePointerAccessor (&YansWifiPhy::m_state),
                   MakePointerChecker<WifiPhyStateHelper> ())
    .AddAttribute ("ChannelSwitchDelay",
                   "Delay between two short frames transmitted on different frequencies.",
                   TimeValue (MicroSeconds (250)),
                   MakeTimeAccessor (&YansWifiPhy::m_channelSwitchDelay),
                   MakeTimeChecker ())
    .AddAttribute ("ChannelNumber",
                   "Channel center frequency = Channel starting frequency + 5 MHz * nch.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&YansWifiPhy::SetChannelNumber,
                                         &YansWifiPhy::GetChannelNumber),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("Frequency",
                   "The operating frequency.",
                   UintegerValue (2407),
                   MakeUintegerAccessor (&YansWifiPhy::GetFrequency,
                                         &YansWifiPhy::SetFrequency),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Transmitters",
                   "The number of transmitters.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&YansWifiPhy::GetNumberOfTransmitAntennas,
                                         &YansWifiPhy::SetNumberOfTransmitAntennas),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Receivers",
                   "The number of receivers.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&YansWifiPhy::GetNumberOfReceiveAntennas,
                                         &YansWifiPhy::SetNumberOfReceiveAntennas),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("ShortGuardEnabled",
                   "Whether or not short guard interval is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetGuardInterval,
                                        &YansWifiPhy::SetGuardInterval),
                   MakeBooleanChecker ())
    .AddAttribute ("LdpcEnabled",
                   "Whether or not LDPC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetLdpc,
                                        &YansWifiPhy::SetLdpc),
                   MakeBooleanChecker ())
    .AddAttribute ("STBCEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetStbc,
                                        &YansWifiPhy::SetStbc),
                   MakeBooleanChecker ())
    .AddAttribute ("GreenfieldEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetGreenfield,
                                        &YansWifiPhy::SetGreenfield),
                   MakeBooleanChecker ())
    .AddAttribute ("S1g1MfieldEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetS1g1Mfield,
                                        &YansWifiPhy::SetS1g1Mfield),
                   MakeBooleanChecker ())
    .AddAttribute ("S1gShortfieldEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),   // for test, temporarily
                   MakeBooleanAccessor (&YansWifiPhy::GetS1gShortfield,
                                        &YansWifiPhy::SetS1gShortfield),
                   MakeBooleanChecker ())
    .AddAttribute ("S1gLongfieldEnabled",
                   "Whether or not STBC is enabled.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&YansWifiPhy::GetS1gLongfield,
                                        &YansWifiPhy::SetS1gLongfield),
                   MakeBooleanChecker ())
    .AddAttribute ("ChannelWidth", "Whether 1MHz, 2MHz, 4MHz, 8MHz, 16MHz, 20MHz or 40MHz.",
                   UintegerValue (1),
                   MakeUintegerAccessor (&YansWifiPhy::GetChannelWidth,
                                         &YansWifiPhy::SetChannelWidth),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}

YansWifiPhy::YansWifiPhy ()
  : m_initialized (false),
    m_channelNumber (1),
    m_endRxEvent (),
    m_endPlcpRxEvent (),
    m_channelStartingFrequency (0),
    m_mpdusNum (0),
    m_plcpSuccess (false)
{
  NS_LOG_FUNCTION (this);
  m_random = CreateObject<UniformRandomVariable> ();
  m_state = CreateObject<WifiPhyStateHelper> ();
}

YansWifiPhy::~YansWifiPhy ()
{
  NS_LOG_FUNCTION (this);
}

void
YansWifiPhy::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_channel = 0;
  m_deviceRateSet.clear ();
  m_deviceMcsSet.clear ();
  m_device = 0;
  m_mobility = 0;
  m_state = 0;
}

void
YansWifiPhy::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_initialized = true;
}

void
YansWifiPhy::ConfigureStandard (enum WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211a:
      Configure80211a ();
      break;
    case WIFI_PHY_STANDARD_80211b:
      Configure80211b ();
      break;
    case WIFI_PHY_STANDARD_80211g:
      Configure80211g ();
      break;
    case WIFI_PHY_STANDARD_80211_10MHZ:
      Configure80211_10Mhz ();
      break;
    case WIFI_PHY_STANDARD_80211_5MHZ:
      Configure80211_5Mhz ();
      break;
    case WIFI_PHY_STANDARD_holland:
      ConfigureHolland ();
      break;
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      m_channelStartingFrequency = 2407;
      Configure80211n ();
      break;
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      m_channelStartingFrequency = 5e3;
      Configure80211n ();
      break;
     case WIFI_PHY_STANDARD_80211ah:
      Configure80211ah ();
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
YansWifiPhy::SetRxNoiseFigure (double noiseFigureDb)
{
  NS_LOG_FUNCTION (this << noiseFigureDb);
  m_interference.SetNoiseFigure (DbToRatio (noiseFigureDb));
}

void
YansWifiPhy::SetTxPowerStart (double start)
{
  NS_LOG_FUNCTION (this << start);
  m_txPowerBaseDbm = start;
}

void
YansWifiPhy::SetTxPowerEnd (double end)
{
  NS_LOG_FUNCTION (this << end);
  m_txPowerEndDbm = end;
}

void
YansWifiPhy::SetNTxPower (uint32_t n)
{
  NS_LOG_FUNCTION (this << n);
  m_nTxPower = n;
}

void
YansWifiPhy::SetTxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_txGainDb = gain;
}

void
YansWifiPhy::SetRxGain (double gain)
{
  NS_LOG_FUNCTION (this << gain);
  m_rxGainDb = gain;
}

void
YansWifiPhy::SetEdThreshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_edThresholdW = DbmToW (threshold);
}

void
YansWifiPhy::SetCcaMode1Threshold (double threshold)
{
  NS_LOG_FUNCTION (this << threshold);
  m_ccaMode1ThresholdW = DbmToW (threshold);
}

void
YansWifiPhy::SetErrorRateModel (Ptr<ErrorRateModel> rate)
{
  m_interference.SetErrorRateModel (rate);
}

void
YansWifiPhy::SetDevice (Ptr<NetDevice> device)
{
  m_device = device;
}

void
YansWifiPhy::SetMobility (Ptr<MobilityModel> mobility)
{
  m_mobility = mobility;
}

double
YansWifiPhy::GetRxNoiseFigure (void) const
{
  return RatioToDb (m_interference.GetNoiseFigure ());
}

double
YansWifiPhy::GetTxPowerStart (void) const
{
  return m_txPowerBaseDbm;
}

double
YansWifiPhy::GetTxPowerEnd (void) const
{
  return m_txPowerEndDbm;
}

double
YansWifiPhy::GetTxGain (void) const
{
  return m_txGainDb;
}

double
YansWifiPhy::GetRxGain (void) const
{
  return m_rxGainDb;
}

double
YansWifiPhy::GetEdThreshold (void) const
{
  return WToDbm (m_edThresholdW);
}

double
YansWifiPhy::GetCcaMode1Threshold (void) const
{
  return WToDbm (m_ccaMode1ThresholdW);
}

Ptr<ErrorRateModel>
YansWifiPhy::GetErrorRateModel (void) const
{
  return m_interference.GetErrorRateModel ();
}

Ptr<NetDevice>
YansWifiPhy::GetDevice (void) const
{
  return m_device;
}

Ptr<MobilityModel>
YansWifiPhy::GetMobility (void)
{
  if (m_mobility != 0)
    {
      return m_mobility;
    }
  else
    {
      return m_device->GetNode ()->GetObject<MobilityModel> ();
    }
}

double
YansWifiPhy::CalculateSnr (WifiMode txMode, double ber) const
{
  return m_interference.GetErrorRateModel ()->CalculateSnr (txMode, ber);
}

Ptr<WifiChannel>
YansWifiPhy::GetChannel (void) const
{
  return m_channel;
}

void
YansWifiPhy::SetChannel (Ptr<YansWifiChannel> channel)
{
  m_channel = channel;
  m_channel->Add (this);
}

void
YansWifiPhy::SetChannelNumber (uint16_t nch)
{
  if (!m_initialized)
    {
      //this is not channel switch, this is initialization
      NS_LOG_DEBUG ("start at channel " << nch);
      m_channelNumber = nch;
      return;
    }

  NS_ASSERT (!IsStateSwitching ());
  switch (m_state->GetState ())
    {
    case YansWifiPhy::RX:
      NS_LOG_DEBUG ("drop packet because of channel switching while reception");
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      goto switchChannel;
      break;
    case YansWifiPhy::TX:
      NS_LOG_DEBUG ("channel switching postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &YansWifiPhy::SetChannelNumber, this, nch);
      break;
    case YansWifiPhy::CCA_BUSY:
    case YansWifiPhy::IDLE:
      goto switchChannel;
      break;
    case YansWifiPhy::SLEEP:
      NS_LOG_DEBUG ("channel switching ignored in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }

  return;

switchChannel:

  NS_LOG_DEBUG ("switching channel " << m_channelNumber << " -> " << nch);
  m_state->SwitchToChannelSwitching (m_channelSwitchDelay);
  m_interference.EraseEvents ();
  /*
   * Needed here to be able to correctly sensed the medium for the first
   * time after the switching. The actual switching is not performed until
   * after m_channelSwitchDelay. Packets received during the switching
   * state are added to the event list and are employed later to figure
   * out the state of the medium after the switching.
   */
  m_channelNumber = nch;
}

uint16_t
YansWifiPhy::GetChannelNumber (void) const
{
  return m_channelNumber;
}

Time
YansWifiPhy::GetChannelSwitchDelay (void) const
{
  return m_channelSwitchDelay;
}

double
YansWifiPhy::GetChannelFrequencyMhz () const
{
  return m_channelStartingFrequency + 5 * GetChannelNumber ();
}

void
YansWifiPhy::SetSleepMode (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case YansWifiPhy::TX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current transmission");
      Simulator::Schedule (GetDelayUntilIdle (), &YansWifiPhy::SetSleepMode, this);
      break;
    case YansWifiPhy::RX:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of current reception");
      Simulator::Schedule (GetDelayUntilIdle (), &YansWifiPhy::SetSleepMode, this);
      break;
    case YansWifiPhy::SWITCHING:
      NS_LOG_DEBUG ("setting sleep mode postponed until end of channel switching");
      Simulator::Schedule (GetDelayUntilIdle (), &YansWifiPhy::SetSleepMode, this);
      break;
    case YansWifiPhy::CCA_BUSY:
    case YansWifiPhy::IDLE:
      NS_LOG_DEBUG ("setting sleep mode");
      m_state->SwitchToSleep ();
      break;
    case YansWifiPhy::SLEEP:
      NS_LOG_DEBUG ("already in sleep mode");
      break;
    default:
      NS_ASSERT (false);
      break;
    }
}

void
YansWifiPhy::ResumeFromSleep (void)
{
  NS_LOG_FUNCTION (this);
  switch (m_state->GetState ())
    {
    case YansWifiPhy::TX:
    case YansWifiPhy::RX:
    case YansWifiPhy::IDLE:
    case YansWifiPhy::CCA_BUSY:
    case YansWifiPhy::SWITCHING:
      {
        NS_LOG_DEBUG ("not in sleep mode, there is nothing to resume");
        break;
      }
    case YansWifiPhy::SLEEP:
      {
        NS_LOG_DEBUG ("resuming from sleep mode");
        Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
        m_state->SwitchFromSleep (delayUntilCcaEnd);
        break;
      }
    default:
      {
        NS_ASSERT (false);
        break;
      }
    }
}

void
YansWifiPhy::SetReceiveOkCallback (RxOkCallback callback)
{
  m_state->SetReceiveOkCallback (callback);
}

void
YansWifiPhy::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_state->SetReceiveErrorCallback (callback);
}

void
YansWifiPhy::StartReceivePreambleAndHeader (Ptr<Packet> packet,
                                            double rxPowerDbm,
                                            WifiTxVector txVector,
                                            enum WifiPreamble preamble,
                                            uint8_t packetType, Time rxDuration)
{
  //This function should be later split to check separately wether plcp preamble and plcp header can be successfully received.
  //Note: plcp preamble reception is not yet modeled.
  //NS_LOG_UNCOND (this << packet << rxPowerDbm << txVector.GetMode () << preamble << (uint32_t)packetType); //test
  NS_LOG_FUNCTION (this << packet << rxPowerDbm << txVector.GetMode () << preamble << (uint32_t)packetType);
  AmpduTag ampduTag;
  rxPowerDbm += m_rxGainDb;
  double rxPowerW = DbmToW (rxPowerDbm);
  Time endRx = Simulator::Now () + rxDuration;
  Time preambleAndHeaderDuration = CalculatePlcpPreambleAndHeaderDuration (txVector, preamble);

  Ptr<InterferenceHelper::Event> event;
  event = m_interference.Add (packet->GetSize (),
                              txVector,
                              preamble,
                              rxDuration,
                              rxPowerW);

  switch (m_state->GetState ())
    {
    case YansWifiPhy::SWITCHING:
      NS_LOG_DEBUG ("drop packet because of channel switching");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      /*
       * Packets received on the upcoming channel are added to the event list
       * during the switching state. This way the medium can be correctly sensed
       * when the device listens to the channel for the first time after the
       * switching e.g. after channel switching, the channel may be sensed as
       * busy due to other devices' tramissions started before the end of
       * the switching.
       */
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the completion of the
          //channel switching.
          goto maybeCcaBusy;
        }
      break;
    case YansWifiPhy::RX:
      NS_LOG_DEBUG ("drop packet because already in Rx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the reception of the
          //currently-received packet.
          goto maybeCcaBusy;
        }
      break;
    case YansWifiPhy::TX:
      NS_LOG_DEBUG ("drop packet because already in Tx (power=" <<
                    rxPowerW << "W)");
      NotifyRxDrop (packet);
      if (endRx > Simulator::Now () + m_state->GetDelayUntilIdle ())
        {
          //that packet will be noise _after_ the transmission of the
          //currently-transmitted packet.
          goto maybeCcaBusy;
        }
      break;
    case YansWifiPhy::CCA_BUSY:
    case YansWifiPhy::IDLE:
      
      //NS_LOG_UNCOND ("rxPowerW " << rxPowerW << "\t" << ",m_edThresholdW " << m_edThresholdW << "," << packet);
            
      if (rxPowerW > m_edThresholdW) //checked here, no need to check in the payload reception (current implementation assumes constant rx power over the packet duration)
        {
          if (preamble == WIFI_PREAMBLE_NONE && m_mpdusNum == 0)
            {
              NS_LOG_DEBUG ("drop packet because no preamble has been received");
              NotifyRxDrop (packet);
              goto maybeCcaBusy;
            }
          else if (preamble == WIFI_PREAMBLE_NONE && m_plcpSuccess == false) //A-MPDU reception fails
            {
              NS_LOG_DEBUG ("Drop MPDU because no plcp has been received");
              NotifyRxDrop (packet);
              goto maybeCcaBusy;
            }
          else if (preamble != WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum == 0)
            {
              //received the first MPDU in an MPDU
              m_mpdusNum = ampduTag.GetNoOfMpdus () - 1;
            }
          else if (preamble == WIFI_PREAMBLE_NONE && packet->PeekPacketTag (ampduTag) && m_mpdusNum > 0)
            {
              //received the other MPDUs that are part of the A-MPDU
              if (ampduTag.GetNoOfMpdus () < m_mpdusNum)
                {
                  NS_LOG_DEBUG ("Missing MPDU from the A-MPDU " << m_mpdusNum - ampduTag.GetNoOfMpdus ());
                  m_mpdusNum = ampduTag.GetNoOfMpdus ();
                }
              else
                {
                  m_mpdusNum--;
                }
            }
          else if (preamble != WIFI_PREAMBLE_NONE && m_mpdusNum > 0 )
            {
              NS_LOG_DEBUG ("Didn't receive the last MPDUs from an A-MPDU " << m_mpdusNum);
              m_mpdusNum = 0;
            }

          NS_LOG_DEBUG ("sync to signal (power=" << rxPowerW << "W)");
          //sync to signal
          m_state->SwitchToRx (rxDuration);
          NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
          NotifyRxBegin (packet);
          m_interference.NotifyRxStart ();

          if (preamble != WIFI_PREAMBLE_NONE)
            {
              NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
              m_endPlcpRxEvent = Simulator::Schedule (preambleAndHeaderDuration, &YansWifiPhy::StartReceivePacket, this,
                                                      packet, txVector, preamble, packetType, event);
            }

          NS_ASSERT (m_endRxEvent.IsExpired ());
          m_endRxEvent = Simulator::Schedule (rxDuration, &YansWifiPhy::EndReceive, this,
                                              packet, preamble, packetType, event);
        }
      else
        {
          NS_LOG_DEBUG ("drop packet because signal power too Small (" <<
                        rxPowerW << "<" << m_edThresholdW << ")");
            //NS_LOG_UNCOND ("drop packet because signal power too Small (" <<
            //              rxPowerW << "<" << m_edThresholdW << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
          goto maybeCcaBusy;
        }
      break;
    case YansWifiPhy::SLEEP:
      NS_LOG_DEBUG ("drop packet because in sleep mode");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
      break;
    }

  return;

maybeCcaBusy:
  //We are here because we have received the first bit of a packet and we are
  //not going to be able to synchronize on it
  //In this model, CCA becomes busy when the aggregation of all signals as
  //tracked by the InterferenceHelper class is higher than the CcaBusyThreshold

  Time delayUntilCcaEnd = m_interference.GetEnergyDuration (m_ccaMode1ThresholdW);
  if (!delayUntilCcaEnd.IsZero ())
    {
      m_state->SwitchMaybeToCcaBusy (delayUntilCcaEnd);
    }
}

void
YansWifiPhy::StartReceivePacket (Ptr<Packet> packet,
                                 WifiTxVector txVector,
                                 enum WifiPreamble preamble,
                                 uint8_t packetType,
                                 Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode () << preamble << (uint32_t)packetType);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (m_endPlcpRxEvent.IsExpired ());
  AmpduTag ampduTag;
  WifiMode txMode = txVector.GetMode ();

  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpHeaderSnrPer (event);

  NS_LOG_DEBUG ("snr=" << snrPer.snr << ", per=" << snrPer.per);

  if (m_random->GetValue () > snrPer.per)   //plcp reception succeeded
    {
      if (IsModeSupported (txMode) || IsMcsSupported (txMode))
        {
          NS_LOG_DEBUG ("receiving plcp payload");     //endReceive is already scheduled
          m_plcpSuccess = true;
        }
      else     //mode is not allowed
        {
          NS_LOG_DEBUG ("drop packet because it was sent using an unsupported mode (" << txMode << ")");
          //NS_LOG_UNCOND ("drop packet because it was sent using an unsupported mode (" << txMode << ")");
          NotifyRxDrop (packet);
          m_plcpSuccess = false;
        }
    }
  else   //plcp reception failed
    {
      NS_LOG_DEBUG ("drop packet because plcp preamble/header reception failed");
      NotifyRxDrop (packet);
      m_plcpSuccess = false;
    }
}

void
YansWifiPhy::SendPacket (Ptr<const Packet> packet, WifiTxVector txVector, WifiPreamble preamble, uint8_t packetType)
{
  NS_LOG_FUNCTION (this << packet << txVector.GetMode () << preamble << (uint32_t)txVector.GetTxPowerLevel () << (uint32_t)packetType);
  /* Transmission can happen if:
   *  - we are syncing on a packet. It is the responsability of the
   *    MAC layer to avoid doing this but the PHY does nothing to
   *    prevent it.
   *  - we are idle
   */
  NS_ASSERT (!m_state->IsStateTx () && !m_state->IsStateSwitching ());

  if (m_state->IsStateSleep ())
    {
      NS_LOG_DEBUG ("Dropping packet because in sleep mode");
      NotifyTxDrop (packet);
      return;
    }

  Time txDuration = CalculateTxDuration (packet->GetSize (), txVector, preamble, GetFrequency (), packetType, 1);
  if (m_state->IsStateRx ())
    {
      m_endPlcpRxEvent.Cancel ();
      m_endRxEvent.Cancel ();
      m_interference.NotifyRxEnd ();
    }
  NotifyTxBegin (packet);
  uint32_t dataRate500KbpsUnits;
  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT || txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_S1G)
    {
      dataRate500KbpsUnits = 128 + WifiModeToMcs (txVector.GetMode ());
    }
  else
    {
      dataRate500KbpsUnits = txVector.GetMode ().GetDataRate () * txVector.GetNss () / 500000;
    }
  bool isShortPreamble = (WIFI_PREAMBLE_SHORT == preamble);
  NotifyMonitorSniffTx (packet, (uint16_t)GetChannelFrequencyMhz (), GetChannelNumber (), dataRate500KbpsUnits, isShortPreamble, txVector);
  m_state->SwitchToTx (txDuration, packet, GetPowerDbm (txVector.GetTxPowerLevel ()), txVector, preamble);
  m_channel->Send (this, packet, GetPowerDbm (txVector.GetTxPowerLevel ()) + m_txGainDb, txVector, preamble, packetType, txDuration);
}

uint32_t
YansWifiPhy::GetNModes (void) const
{
  return m_deviceRateSet.size ();
}

WifiMode
YansWifiPhy::GetMode (uint32_t mode) const
{
  return m_deviceRateSet[mode];
}

bool
YansWifiPhy::IsModeSupported (WifiMode mode) const
{
  for (uint32_t i = 0; i < GetNModes (); i++)
    {
      if (mode == GetMode (i))
        {
          return true;
        }
    }
  return false;
}
bool
YansWifiPhy::IsMcsSupported (WifiMode mode)
{
    //NS_LOG_UNCOND ("IsMcsSupported, " << mode << "\t" << GetNMcs ());
  for (uint32_t i = 0; i < GetNMcs (); i++)
    {
      //NS_LOG_UNCOND ("IsMcsSupported-aa, " << mode << "\t" << McsToWifiMode (GetMcs (i)));
      if (mode == McsToWifiMode (GetMcs (i)))
        {
          return true;
        }
    }
    //NS_LOG_UNCOND ("IsMcsSupported not supported, " << mode);
  return false;
}

uint32_t
YansWifiPhy::GetNTxPower (void) const
{
  return m_nTxPower;
}

void
YansWifiPhy::Configure80211a (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; //5.000 GHz

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
YansWifiPhy::Configure80211b (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 2407; //2.407 GHz

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
}

void
YansWifiPhy::Configure80211g (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 2407; //2.407 GHz

  m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate9Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate48Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate54Mbps ());
}

void
YansWifiPhy::Configure80211_10Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; //5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24MbpsBW10MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate27MbpsBW10MHz ());
}

void
YansWifiPhy::Configure80211_5Mhz (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; //5.000 GHz, suppose 802.11a

  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate1_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate2_25MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate3MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate4_5MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate9MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12MbpsBW5MHz ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate13_5MbpsBW5MHz ());
}

void
YansWifiPhy::ConfigureHolland (void)
{
  NS_LOG_FUNCTION (this);
  m_channelStartingFrequency = 5e3; //5.000 GHz
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate18Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate36Mbps ());
  m_deviceRateSet.push_back (WifiPhy::GetOfdmRate54Mbps ());
}

void
YansWifiPhy::Configure80211n (void)
{
  NS_LOG_FUNCTION (this);
  if (m_channelStartingFrequency >= 2400 && m_channelStartingFrequency <= 2500) //at 2.4 GHz
    {
      m_deviceRateSet.push_back (WifiPhy::GetDsssRate1Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetDsssRate2Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetDsssRate5_5Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate6Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetDsssRate11Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate12Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetErpOfdmRate24Mbps ());
    }
  if (m_channelStartingFrequency >= 5000 && m_channelStartingFrequency <= 6000) //at 5 GHz
    {
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
      m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
    }
  m_bssMembershipSelectorSet.push_back (HT_PHY);
  for (uint8_t i = 0; i < 8; i++)
    {
      m_deviceMcsSet.push_back (i);
    }
}
    
void
YansWifiPhy::Configure80211ah (void)
{
    NS_LOG_FUNCTION (this);
    m_channelStartingFrequency = 9e2;
    
    // need to check for 802.11ah
    m_deviceRateSet.push_back (WifiPhy::GetOfdmRate6Mbps ());
    m_deviceRateSet.push_back (WifiPhy::GetOfdmRate300KbpsBW1MHz ());
    m_deviceRateSet.push_back (WifiPhy::GetOfdmRate650KbpsBW2MHz ());
    
    //m_deviceRateSet.push_back (WifiPhy::GetOfdmRate12Mbps ());
    //m_deviceRateSet.push_back (WifiPhy::GetOfdmRate24Mbps ());
    m_bssMembershipSelectorSet.push_back(S1G_PHY);
    for (uint8_t i=0; i <11; i++)
    {
        //NS_LOG_UNCOND ("YansWifiPhy::Configure80211ah");
        m_deviceMcsSet.push_back(i);
    }
}


void
YansWifiPhy::RegisterListener (WifiPhyListener *listener)
{
  m_state->RegisterListener (listener);
}

void
YansWifiPhy::UnregisterListener (WifiPhyListener *listener)
{
  m_state->UnregisterListener (listener);
}

bool
YansWifiPhy::IsStateCcaBusy (void)
{
  return m_state->IsStateCcaBusy ();
}

bool
YansWifiPhy::IsStateIdle (void)
{
  return m_state->IsStateIdle ();
}

bool
YansWifiPhy::IsStateBusy (void)
{
  return m_state->IsStateBusy ();
}

bool
YansWifiPhy::IsStateRx (void)
{
  return m_state->IsStateRx ();
}

bool
YansWifiPhy::IsStateTx (void)
{
  return m_state->IsStateTx ();
}

bool
YansWifiPhy::IsStateSwitching (void)
{
  return m_state->IsStateSwitching ();
}

bool
YansWifiPhy::IsStateSleep (void)
{
  return m_state->IsStateSleep ();
}

Time
YansWifiPhy::GetStateDuration (void)
{
  return m_state->GetStateDuration ();
}

Time
YansWifiPhy::GetDelayUntilIdle (void)
{
  return m_state->GetDelayUntilIdle ();
}

Time
YansWifiPhy::GetLastRxStartTime (void) const
{
  return m_state->GetLastRxStartTime ();
}

double
YansWifiPhy::DbToRatio (double dB) const
{
  double ratio = std::pow (10.0, dB / 10.0);
  return ratio;
}

double
YansWifiPhy::DbmToW (double dBm) const
{
  double mW = std::pow (10.0, dBm / 10.0);
  return mW / 1000.0;
}

double
YansWifiPhy::WToDbm (double w) const
{
  return 10.0 * std::log10 (w * 1000.0);
}

double
YansWifiPhy::RatioToDb (double ratio) const
{
  return 10.0 * std::log10 (ratio);
}

double
YansWifiPhy::GetEdThresholdW (void) const
{
  return m_edThresholdW;
}

double
YansWifiPhy::GetPowerDbm (uint8_t power) const
{
  NS_ASSERT (m_txPowerBaseDbm <= m_txPowerEndDbm);
  NS_ASSERT (m_nTxPower > 0);
  double dbm;
  if (m_nTxPower > 1)
    {
      dbm = m_txPowerBaseDbm + power * (m_txPowerEndDbm - m_txPowerBaseDbm) / (m_nTxPower - 1);
    }
  else
    {
      NS_ASSERT_MSG (m_txPowerBaseDbm == m_txPowerEndDbm, "cannot have TxPowerEnd != TxPowerStart with TxPowerLevels == 1");
      dbm = m_txPowerBaseDbm;
    }
  return dbm;
}

void
YansWifiPhy::EndReceive (Ptr<Packet> packet, enum WifiPreamble preamble, uint8_t packetType, Ptr<InterferenceHelper::Event> event)
{
  NS_LOG_FUNCTION (this << packet << event);
  NS_ASSERT (IsStateRx ());
  NS_ASSERT (event->GetEndTime () == Simulator::Now ());

  struct InterferenceHelper::SnrPer snrPer;
  snrPer = m_interference.CalculatePlcpPayloadSnrPer (event);
  m_interference.NotifyRxEnd ();

    //NS_LOG_UNCOND ("YansWifiPhy::EndReceive, mode=" << (event->GetPayloadMode ().GetDataRate ()) <<
               //   ", snr=" << snrPer.snr << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
  if (m_plcpSuccess == true)
    {
      NS_LOG_DEBUG ("mode=" << (event->GetPayloadMode ().GetDataRate ()) << ", snr=" << snrPer.snr << ", per=" << snrPer.per << ", size=" << packet->GetSize ());
        
        //double snrtest1 = CalculateSnr (WifiPhy::GetOfdmRate300KbpsBW1MHz (), 0.1); //test
        //double snrtest2 = CalculateSnr (WifiPhy::GetOfdmRate300KbpsBW1MHz (), 5.07867e-11); //test
        //double snrtest3 = CalculateSnr (WifiPhy::GetOfdmRate300KbpsBW1MHz (), 0); //test
        //NS_LOG_UNCOND ("1153--YansWifiPhy::EndReceive, snrtest1=" << snrtest1 << ", snrtest2=" << snrtest2 << ", snrtest3=" << snrtest3);
        
       // NS_LOG_UNCOND ("YansWifiPhy::EndReceive, mode=" << (event->GetPayloadMode ().GetDataRate ()) << ", snr=" << snrPer.snr << ", per=" << snrPer.per << ", size=" << packet->GetSize () << "," << packet);

      if (m_random->GetValue () > snrPer.per)
        {
          NotifyRxEnd (packet);
          uint32_t dataRate500KbpsUnits;
          if ((event->GetPayloadMode ().GetModulationClass () == WIFI_MOD_CLASS_HT) || (event->GetPayloadMode ().GetModulationClass () == WIFI_MOD_CLASS_S1G))
            {
              dataRate500KbpsUnits = 128 + WifiModeToMcs (event->GetPayloadMode ());
            }
          else
            {
              dataRate500KbpsUnits = event->GetPayloadMode ().GetDataRate () * event->GetTxVector ().GetNss () / 500000;
            }
          bool isShortPreamble = (WIFI_PREAMBLE_SHORT == event->GetPreambleType ());
          double signalDbm = RatioToDb (event->GetRxPowerW ()) + 30;
          double noiseDbm = RatioToDb (event->GetRxPowerW () / snrPer.snr) - GetRxNoiseFigure () + 30;
          NotifyMonitorSniffRx (packet, (uint16_t)GetChannelFrequencyMhz (), GetChannelNumber (), dataRate500KbpsUnits, isShortPreamble, event->GetTxVector (), signalDbm, noiseDbm);
          m_state->SwitchFromRxEndOk (packet, snrPer.snr, event->GetTxVector (), event->GetPreambleType ());
            
          //NS_LOG_UNCOND ("YansWifiPhy::EndReceive, SwitchFromRxEndOk, "  << packet);
        }
      else
        {
          /* failure. */
          //NS_LOG_UNCOND ("YansWifiPhy::EndReceive-reason1");
          NotifyRxDrop (packet);
          m_state->SwitchFromRxEndError (packet, snrPer.snr);
        }
    }
  else
    {
      //notify rx end
      NS_LOG_UNCOND ("YansWifiPhy::EndReceive-reason2");
      m_state->SwitchFromRxEndError (packet, snrPer.snr);
    }

  if (preamble == WIFI_PREAMBLE_NONE && packetType == 2)
    {
      m_plcpSuccess = false;
    }
}

int64_t
YansWifiPhy::AssignStreams (int64_t stream)
{
  NS_LOG_FUNCTION (this << stream);
  m_random->SetStream (stream);
  return 1;
}

void
YansWifiPhy::SetFrequency (uint32_t freq)
{
  m_channelStartingFrequency = freq;
}

void
YansWifiPhy::SetNumberOfTransmitAntennas (uint32_t tx)
{
  m_numberOfTransmitters = tx;
}

void
YansWifiPhy::SetNumberOfReceiveAntennas (uint32_t rx)
{
  m_numberOfReceivers = rx;
}

void
YansWifiPhy::SetLdpc (bool Ldpc)
{
  m_ldpc = Ldpc;
}

void
YansWifiPhy::SetStbc (bool stbc)
{
  m_stbc = stbc;
}

void
YansWifiPhy::SetGreenfield (bool greenfield)
{
  m_greenfield = greenfield;
}
    
void
YansWifiPhy::SetS1g1Mfield (bool s1g1mfield)
{
  m_s1g1mfield = s1g1mfield;
}

void
YansWifiPhy::SetS1gShortfield (bool s1gshortfield)
{
  m_s1gshortfield = s1gshortfield;
}

void
YansWifiPhy::SetS1gLongfield (bool s1glongfield)
{
  m_s1glongfield = s1glongfield;
}

bool
YansWifiPhy::GetGuardInterval (void) const
{
  return m_guardInterval;
}

void
YansWifiPhy::SetGuardInterval (bool guardInterval)
{
  m_guardInterval = guardInterval;
}

uint32_t
YansWifiPhy::GetFrequency (void) const
{
  return m_channelStartingFrequency;
}

uint32_t
YansWifiPhy::GetNumberOfTransmitAntennas (void) const
{
  return m_numberOfTransmitters;
}

uint32_t
YansWifiPhy::GetNumberOfReceiveAntennas (void) const
{
  return m_numberOfReceivers;
}

bool
YansWifiPhy::GetLdpc (void) const
{
  return m_ldpc;
}

bool
YansWifiPhy::GetStbc (void) const
{
  return m_stbc;
}

bool
YansWifiPhy::GetGreenfield (void) const
{
  return m_greenfield;
}

bool
YansWifiPhy::GetS1g1Mfield (void) const
{
    return m_s1g1mfield;
}

bool
YansWifiPhy::GetS1gShortfield (void) const
{
    return m_s1gshortfield;
}

bool
YansWifiPhy::GetS1gLongfield (void) const
{
    return m_s1glongfield;
}

void
YansWifiPhy::SetChannelWidth(uint32_t channelwidth)
{
    NS_ASSERT_MSG (channelwidth == 1 || channelwidth == 2 || channelwidth == 4| channelwidth == 8| channelwidth ==16|| channelwidth == 20 || channelwidth == 40 || channelwidth == 80 || channelwidth == 160, "wrong channel width value");
    m_channelWidth = channelwidth;
}

uint32_t
YansWifiPhy::GetChannelWidth(void) const
{
    return m_channelWidth;
}

uint32_t
YansWifiPhy::GetNBssMembershipSelectors (void) const
{
  return m_bssMembershipSelectorSet.size ();
}

uint32_t
YansWifiPhy::GetBssMembershipSelector (uint32_t selector) const
{
  return m_bssMembershipSelectorSet[selector];
}

WifiModeList
YansWifiPhy::GetMembershipSelectorModes (uint32_t selector)
{
  uint32_t id = GetBssMembershipSelector (selector);
    //NS_LOG_UNCOND ("YansWifiPhy id " << id);
  WifiModeList supportedmodes;
  if (id == HT_PHY)
    {
      //mandatory MCS 0 to 7
      supportedmodes.push_back (WifiPhy::GetOfdmRate6_5MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate13MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate19_5MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate26MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate39MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate52MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate58_5MbpsBW20MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate65MbpsBW20MHz ());
    }
  if (id == S1G_PHY)
    {
      //mandatory MCS 0 to 7, 1Mhz
      supportedmodes.push_back (WifiPhy::GetOfdmRate300KbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate600KbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate900KbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate1_2MbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate1_8MbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate2_4MbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate2_7MbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate3MbpsBW1MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate150KbpsBW1MHz ());
    //mandatory MCS 0 to 7, 2Mhz
      supportedmodes.push_back (WifiPhy::GetOfdmRate650KbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate1_3MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate1_95MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate2_6MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate3_9MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate5_2MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate5_85MbpsBW2MHz ());
      supportedmodes.push_back (WifiPhy::GetOfdmRate6_5MbpsBW2MHz ());
    }

  return supportedmodes;
}

uint8_t
YansWifiPhy::GetNMcs (void) const
{
  return m_deviceMcsSet.size ();
}

uint8_t
YansWifiPhy::GetMcs (uint8_t mcs) const
{
  return m_deviceMcsSet[mcs];
}

uint32_t
YansWifiPhy::WifiModeToMcs (WifiMode mode)
{
    uint32_t mcs = 0;
    if (mode.GetUniqueName() == "OfdmRate5_85MbpsBW16MHz" || mode.GetUniqueName() == "OfdmRate6_5MbpsBW16MHz" )
    {
        mcs = 0;
    }
    else if (mode.GetUniqueName() == "OfdmRate3MbpsBW4MHz" || mode.GetUniqueName() == "OfdmRate5_85MbpsBW8MHz" )
    {
        mcs = 1;
    }
    else if (mode.GetUniqueName() == "OfdmRate17_55MbpsBW16MHz" )
    {
        mcs =2;
    }
    else if (mode.GetUniqueName() == "OfdmRate11_7MbpsBW8MHz" || mode.GetUniqueName() == "OfdmRate13MbpsBW8MHz"  || mode.GetUniqueName() == "OfdmRate23_4MbpsBW16MHz" || mode.GetUniqueName() == "OfdmRate26MbpsBW16MHz")
    {
        mcs = 3;
    }
    else if (mode.GetUniqueName() == "OfdmRate19_5MbpsBW8MHz" || mode.GetUniqueName() == "OfdmRate35_1MbpsBW16MHz"  || mode.GetUniqueName() == "OfdmRate39MbpsBW16MHz" )
    {
        mcs = 4;
    }
    else if (mode.GetUniqueName() == "OfdmRate2_7MbpsBW1MHz" || mode.GetUniqueName() == "OfdmRate3MbpsBW1MHzShGi" || mode.GetUniqueName() == "OfdmRate6_5MbpsBW2MHzShGi" || mode.GetUniqueName() == "OfdmRate13_5MbpsBW4MHzShGi" || mode.GetUniqueName() == "OfdmRate29_25MbpsBW8MHzShGi" ||
             mode.GetUniqueName() == "OfdmRate58_5MbpsBW16MHzShGi" || mode.GetUniqueName() == "OfdmRate135MbpsBW40MHzShGi" || mode.GetUniqueName() == "OfdmRate65MbpsBW20MHzShGi" )
    {
        mcs = 6;
    }
    else if (mode.GetUniqueName() == "OfdmRate6_5MbpsBW2MHz" )
    {
        mcs = 7;
    }
    else if (mode.GetUniqueName() == "OfdmRate4MbpsBW1MHzShGi" || mode.GetUniqueName() == "OfdmRate18MbpsBW4MHzShGi" || mode.GetUniqueName() == "OfdmRate39MbpsBW8MHzShGi" || mode.GetUniqueName() == "OfdmRate78MbpsBW16MHzShGi")
    {
        mcs = 8;
    }
    else if (mode.GetModulationClass() == WIFI_MOD_CLASS_S1G )
    {
        switch (mode.GetDataRate ())
        {
            case 300000:
            case 333300:
            case 650000:
            case 722200:
            case 1350000:
            case 1500000:
            case 2925000:
            case 3250000:
                mcs = 0;
                break;
            case 600000:
            case 666700:
            case 1300000:
            case 1444400:
            case 2700000:
            case 6500000:
            case 11700000:
            case 13000000:
                mcs = 1;
                break;
            case 900000:
            case 1000000:
            case 1950000:
            case 2166700:
            case 4050000:
            case 4500000:
            case 8775000:
            case 9750000:
            case 19500000:
                mcs=2;
                break;
            case 1200000:
            case 1333300:
            case 2600000:
            case 2888900:
            case 5400000:
            case 6000000:
                mcs=3;
                break;
            case 1800000:
            case 2000000:
            case 3900000:
            case 4333300:
            case 8100000:
            case 9000000:
            case 17550000:
                mcs = 4;
                break;
            case 2400000:
            case 2666700:
            case 5200000:
            case 5777800:
            case 10800000:
            case 12000000:
            case 23400000:
            case 26000000:
            case 46800000:
            case 52000000:
                mcs=5;
                break;
            case 5850000:
            case 12150000:
            case 26325000:
            case 52650000:
                mcs=6;
                break;
            case 3000000:
            case 3333300:
            case 7222200:
            case 13500000:
            case 15000000:
            case 29250000:
            case 32500000:
            case 58500000:
            case 65000000:
                mcs=7;
                break;
            case 3600000:
            case 7800000:
            case 8666700:
            case 16200000:
            case 35100000:
            case 70200000:
                mcs=8;
                break;
            case 4000000:
            case 4444400:
            case 18000000:
            case 20000000:
            case 39000000:
            case 43333300:
            case 78000000:
            case 86666700:
                mcs=9;
                break;
            case 150000:
            case 166700:
                mcs=10;
                break;
        }
    }
    else
    {
        switch (mode.GetDataRate())
        {
            case 6500000:
            case 7200000:
            case 13500000:
            case 15000000:
                mcs=0;
                break;
            case 13000000:
            case 14400000:
            case 27000000:
            case 30000000:
                mcs=1;
                break;
            case 19500000:
            case 21700000:
            case 40500000:
            case 45000000:
                mcs=2;
                break;
            case 26000000:
            case 28900000:
            case 54000000:
            case 60000000:
                mcs=3;
                break;
            case 39000000:
            case 43300000:
            case 81000000:
            case 90000000:
                mcs=4;
                break;
            case 52000000:
            case 57800000:
            case 108000000:
            case 120000000:
                mcs=5;
                break;
            case 58500000:
            case 121500000:
                mcs=6;
                break;
            case 65000000:
            case 72200000:
            case 135000000:
            case 150000000:
                mcs=7;
                break;
        }
    }
    return mcs;
}


WifiMode
YansWifiPhy::McsToWifiMode (uint8_t mcs)
{
    //NS_LOG_UNCOND ("YansWifiPhy::McsToWifiMode (uint8_t mcs) " << mcs);
    WifiMode mode;
    switch (mcs)
    {
        case 10:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate150KbpsBW1MHz ();
            }
            else
            {
                mode = WifiPhy::GetOfdmRate166_7KbpsBW1MHz ();
            }
            break;
        case 9:
            
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate4MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate4_444_4MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate18MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate20MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate39MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate43_333_3MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate78MbpsBW16MHz ();
            }
            else
            {
                mode = WifiPhy::GetOfdmRate86_666_7MbpsBW16MHz ();
            }
            break;
        case 8:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate3_6MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate4MbpsBW1MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate7_8MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate8_666_7MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate16_2MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate18MbpsBW4MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate35_1MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate39MbpsBW8MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate70_2MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate78MbpsBW16MHzShGi ();
            }
            break;
        case 7:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate3MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate3_333_3MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate6_5MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate7_222_2MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate13_5MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate15MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate29_25MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate32_5MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate58_5MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate65MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode =  WifiPhy::GetOfdmRate65MbpsBW20MHz ();
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate72_2MbpsBW20MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate135MbpsBW40MHz ();
            }
            else
            {
                mode = WifiPhy::GetOfdmRate150MbpsBW40MHz ();
            }
            break;
        case 6:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate2_7MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate3MbpsBW1MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate5_85MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate6_5MbpsBW2MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate12_15MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate13_5MbpsBW4MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate26_325MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate29_25MbpsBW8MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate52_65MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate58_5MbpsBW16MHzShGi ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate58_5MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode =  WifiPhy::GetOfdmRate65MbpsBW20MHzShGi ();
                
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate121_5MbpsBW40MHz ();
                
            }
            else
            {
                mode= WifiPhy::GetOfdmRate135MbpsBW40MHzShGi ();
                
            }
            break;
        case 5:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate2_4MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate2_666_7MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate5_2MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate5_777_8MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate10_8MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate12MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate23_4MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate26MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate46_8MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate52MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate52MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate57_8MbpsBW20MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate108MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate120MbpsBW40MHz ();
                
            }
            break;
        case 4:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate1_8MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate2MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate3_9MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate4_333_3MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate8_1MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate9MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate17_55MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate19_5MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate35_1MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate39MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate39MbpsBW20MHz ();
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate43_3MbpsBW20MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate81MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate90MbpsBW40MHz ();
                
            }
            break;
        case 3:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate1_2MbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate1_333_3MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate2_6MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate2_8889MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate5_4MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate6MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate11_7MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate13MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate23_4MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate26MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode =  WifiPhy::GetOfdmRate26MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate28_9MbpsBW20MHz ();
                
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate54MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate60MbpsBW40MHz ();
            }
            break;
        case 2:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate900KbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate1MbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate1_95MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate2_166_7MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate4_05MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate4_5MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate8_775MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate9_75MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate17_55MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate19_5MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate19_5MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate21_7MbpsBW20MHz ();
                
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode =  WifiPhy::GetOfdmRate40_5MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate45MbpsBW40MHz ();
                
            }
            break;
        case 1:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate600KbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate666_7KbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate1_3MbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate1_444_4MbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate2_7MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate3MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate5_85MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate6_5MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate11_7MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate13MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate13MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode =  WifiPhy::GetOfdmRate14_4MbpsBW20MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate27MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate30MbpsBW40MHz ();
            }
            break;
        case 0:
        default:
            if (!GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate300KbpsBW1MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 1)
            {
                mode = WifiPhy::GetOfdmRate333_3KbpsBW1MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate650KbpsBW2MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 2)
            {
                mode = WifiPhy::GetOfdmRate722_2KbpsBW2MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate1_35MbpsBW4MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 4)
            {
                mode = WifiPhy::GetOfdmRate1_5MbpsBW4MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate2_925MbpsBW8MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 8)
            {
                mode = WifiPhy::GetOfdmRate3_25MbpsBW8MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate5_85MbpsBW16MHz ();
            }
            else if (GetGuardInterval() && GetChannelWidth() == 16)
            {
                mode = WifiPhy::GetOfdmRate6_5MbpsBW16MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate6_5MbpsBW20MHz ();
                
            }
            else if(GetGuardInterval() && GetChannelWidth() == 20)
            {
                mode = WifiPhy::GetOfdmRate7_2MbpsBW20MHz ();
            }
            else if (!GetGuardInterval() && GetChannelWidth() == 40)
            {
                mode = WifiPhy::GetOfdmRate13_5MbpsBW40MHz ();
                
            }
            else
            {
                mode = WifiPhy::GetOfdmRate15MbpsBW40MHz ();
            }
            break;
    }
    return mode;
}


} //namespace ns3
