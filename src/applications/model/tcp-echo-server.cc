/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2012
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
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "tcp-echo-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpEchoServerApplication");
NS_OBJECT_ENSURE_REGISTERED (TcpEchoServer);

TypeId
TcpEchoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpEchoServer")
    .SetParent<Application> ()
    .AddConstructor<TcpEchoServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming connections.",
                   UintegerValue (7),
                   MakeUintegerAccessor (&TcpEchoServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
   .AddTraceSource("Rx",
					"A packet is received",
					MakeTraceSourceAccessor(&TcpEchoServer::m_packetReceived),
					"ns3::TcpEchoServer::PacketReceivedCallback")
	.AddTraceSource ("Retransmission",
						  "Occurs when TCP socket does a retransmission",
						  MakeTraceSourceAccessor (&TcpEchoServer::m_retransmission),
						  "ns3::TcpEchoServer::RetransmissionCallBack")

	  .AddTraceSource ("TCPStateChanged",
						 "TCP state changed",
						 MakeTraceSourceAccessor (&TcpEchoServer::m_tcpStateChanged),
						 "ns3::TcpEchoServer::TCPStateChangedCallBack")

  ;
  return tid;
}

TcpEchoServer::TcpEchoServer ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
  m_running = false;
}

TcpEchoServer::~TcpEchoServer()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_socket = 0;
}

void
TcpEchoServer::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
TcpEchoServer::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_running = true;

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);

      InetSocketAddress listenAddress = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      m_socket->Bind (listenAddress);
      m_socket->Listen();
    }




  m_socket->SetAcceptCallback (
      MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
      MakeCallback (&TcpEchoServer::HandleAccept, this));
}

void TcpEchoServer::OnRetransmission(Address a) {
	m_retransmission(a);
}


void TcpEchoServer::OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal, Address a) {
	m_tcpStateChanged(oldVal, newVal, a);
}


void
TcpEchoServer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_running = false;

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeNullCallback<void, Ptr<Socket>, const Address &> () );
    }
}

void
TcpEchoServer::ReceivePacket (Ptr<Socket> s)
{
  NS_LOG_FUNCTION (this << s);

  Ptr<Packet> packet;
  Address from;
  while (packet = s->RecvFrom (from))
    {
      if (packet->GetSize () > 0)
        {

    	  m_packetReceived(packet, from);

    	  NS_LOG_INFO ("Server Received " << packet->GetSize () << " bytes from " <<
    	                         InetSocketAddress::ConvertFrom (from).GetIpv4 ());

    	  packet->RemoveAllPacketTags ();
    	  packet->RemoveAllByteTags ();

    	  NS_LOG_LOGIC ("Echoing packet");
    	  s->Send (packet);
        }
    }
}


void TcpEchoServer::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);

  handles[from].address = from;
  handles[from].callback = MakeCallback(&TcpEchoServer::OnTCPStateChanged, this);

  s->TraceConnectWithoutContext("Retransmission", MakeCallback(&TcpEchoServer::OnRetransmission, this));
  s->TraceConnectWithoutContext("State", MakeCallback(&TcpEchoServer::Handler::OnTCPStateChanged, &handles[from]));

  s->SetRecvCallback (MakeCallback (&TcpEchoServer::ReceivePacket, this));
  s->SetCloseCallbacks(MakeCallback (&TcpEchoServer::HandleSuccessClose, this),
    MakeNullCallback<void, Ptr<Socket> > () );
}

void TcpEchoServer::HandleSuccessClose(Ptr<Socket> s)
{
  NS_LOG_FUNCTION (this << s);
  NS_LOG_LOGIC ("Client close received");
  s->Close();
  s->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > () );
  s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> > (),
      MakeNullCallback<void, Ptr<Socket> > () );
}

} // Namespace ns3
