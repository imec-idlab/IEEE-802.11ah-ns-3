/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "tcp-echo-client.h"
#include "seq-ts-header.h"
#include "ns3/tcp-socket-base.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpEchoClientApplication");
NS_OBJECT_ENSURE_REGISTERED (TcpEchoClient);

TypeId
TcpEchoClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpEchoClient")
    .SetParent<Application> ()
    .AddConstructor<TcpEchoClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&TcpEchoClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&TcpEchoClient::m_interval),
                   MakeTimeChecker ())

	.AddAttribute ("IntervalDeviation",
				   "The possible deviation from the interval to send packets",
				   TimeValue (Seconds (0)),
				   MakeTimeAccessor (&TcpEchoClient::m_intervalDeviation),
				   MakeTimeChecker ())

    .AddAttribute ("RemoteAddress",
                   "The destination Ipv4Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&TcpEchoClient::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort",
                   "The destination port of the outbound packets",
                   UintegerValue (0),
                   MakeUintegerAccessor (&TcpEchoClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&TcpEchoClient::SetDataSize,
                                         &TcpEchoClient::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())



    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&TcpEchoClient::m_txTrace))
	.AddTraceSource("Rx",
					"An echo packet is received",
					MakeTraceSourceAccessor(&TcpEchoClient::m_packetReceived),
					"ns3::TcpEchoClient::PacketReceivedCallback")
	.AddTraceSource ("CongestionWindow",
					 "The TCP connection's congestion window",
					 MakeTraceSourceAccessor (&TcpEchoClient::m_cWnd),
					 "ns3::TracedValue::Uint32Callback")

	.AddTraceSource ("RTO",
					 "The TCP connection's RTO",
					 MakeTraceSourceAccessor (&TcpEchoClient::m_rtoChanged),
					 "ns3::Time::TracedValueCallback")

	.AddTraceSource ("RTT",
					 "Last sample of the RTT",
					 MakeTraceSourceAccessor (&TcpEchoClient::m_rttChanged),
					 "ns3::Time::TracedValueCallback")

	.AddTraceSource ("Retransmission",
						  "Occurs when a packet has to be scheduled for retransmission",
						  MakeTraceSourceAccessor (&TcpEchoClient::m_retransmission),
						  "ns3::TcpEchoClient::RetransmissionCallBack")

	.AddTraceSource ("TCPStateChanged",
					 "TCP state changed",
					 MakeTraceSourceAccessor (&TcpEchoClient::m_tcpStateChanged),
					 "ns3::TcpStatesTracedValueCallback")

	 .AddTraceSource ("SlowStartThreshold",
							"TCP slow start threshold (bytes)",
							MakeTraceSourceAccessor (&TcpEchoClient::m_slowStartThresholdChanged),
							"ns3::TracedValue::Uint32Callback")

     .AddTraceSource("EstimatedBW", "The estimated bandwidth (in segments)",
					MakeTraceSourceAccessor(&TcpEchoClient::m_currentEstimatedBandwidthChanged),
					  "ns3::TracedValue::DoubleCallback");

  ;
  return tid;
}

TcpEchoClient::TcpEchoClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sent = 0;
  m_bytesSent = 0;
  m_recvBack = 0;
  m_bytesRecvBack = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;

  m_rv = CreateObject<UniformRandomVariable> ();
}

TcpEchoClient::~TcpEchoClient()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;

  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
TcpEchoClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
TcpEchoClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rv = 0;
  Application::DoDispose ();
}

void
TcpEchoClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);


      m_socket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&TcpEchoClient::OnCongestionWindowChanged, this));
      m_socket->TraceConnectWithoutContext("Retransmission", MakeCallback(&TcpEchoClient::OnRetransmission, this));
      m_socket->TraceConnectWithoutContext("RTO", MakeCallback(&TcpEchoClient::OnRTOChanged, this));
      m_socket->TraceConnectWithoutContext("RTT", MakeCallback(&TcpEchoClient::OnRTTChanged, this));
      m_socket->TraceConnectWithoutContext("State", MakeCallback(&TcpEchoClient::OnTCPStateChanged, this));
      m_socket->TraceConnectWithoutContext("SlowStartThreshold", MakeCallback(&TcpEchoClient::OnTCPSlowStartThresholdChanged, this));
      m_socket->GetObject<TcpSocketBase>()->GetCongestionControlAlgorithm()->TraceConnectWithoutContext("EstimatedBW", MakeCallback(&TcpEchoClient::OnTCPEstimatedBWChanged, this));


      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }

    m_socket->SetRecvCallback (MakeCallback (&TcpEchoClient::ReceivePacket, this));

    // ensure randomization of transmissions
    double deviation = m_rv->GetValue(-m_intervalDeviation.GetMicroSeconds(), m_intervalDeviation.GetMicroSeconds());
    ScheduleTransmit (m_interval + MicroSeconds(deviation));
}

void TcpEchoClient::OnRetransmission(Address a) {
	m_retransmission(a);
}

void TcpEchoClient::OnCongestionWindowChanged(uint32_t oldval, uint32_t newval) {
	m_cWnd(oldval,newval/m_socket->GetObject<TcpSocketBase>()->GetSegSize());
}

void TcpEchoClient::OnRTOChanged(Time oldval, Time newval) {
	m_rtoChanged(oldval, newval);
}

void TcpEchoClient::OnRTTChanged(Time oldval, Time newval) {
	m_rttChanged(oldval, newval);
}


void TcpEchoClient::OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal) {
	m_tcpStateChanged(oldVal, newVal);
}

void TcpEchoClient::OnTCPSlowStartThresholdChanged(uint32_t oldVal,uint32_t newVal) {
	m_slowStartThresholdChanged(oldVal, newVal);
}

void TcpEchoClient::OnTCPEstimatedBWChanged(double oldVal, double newVal) {
	m_currentEstimatedBandwidthChanged(oldVal, newVal);
}


void
TcpEchoClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
TcpEchoClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
TcpEchoClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_size;
}

void
TcpEchoClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpEchoClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpEchoClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
TcpEchoClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendEvent = Simulator::Schedule (dt, &TcpEchoClient::Send, this);
}

void
TcpEchoClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size, "TcpEchoClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "TcpEchoClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
      m_bytesSent += m_dataSize;
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that she doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding atribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
      m_bytesSent += m_size;
    }


  // add sequence header to the packet
    SeqTsHeader seqTs;
    seqTs.SetSeq (m_sent);

    p->AddHeader (seqTs);

  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);

  m_socket->Send (p);

  ++m_sent;

  NS_LOG_INFO ("Sent " << m_size << " bytes to " << m_peerAddress);

  if (m_sent < m_count)
    {
	  double deviation = m_rv->GetValue(-m_intervalDeviation.GetMicroSeconds(), m_intervalDeviation.GetMicroSeconds());
	  ScheduleTransmit (m_interval + MicroSeconds(deviation));
    }
}

void
TcpEchoClient::ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while (packet = socket->RecvFrom (from))
    {
	  m_packetReceived(packet, from);

      NS_LOG_INFO ("Received " << packet->GetSize () << " bytes from " <<
             InetSocketAddress::ConvertFrom (from).GetIpv4 ());

      // dont check if data returned is the same data sent earlier
      m_recvBack++;
      m_bytesRecvBack += packet->GetSize ();
    }

  if (m_count == m_recvBack)
    {
	  socket->Close();
	  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	  socket = 0;
    }
}

} // Namespace ns3
