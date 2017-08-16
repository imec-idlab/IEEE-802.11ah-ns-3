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
#include "seq-ts-header.h"

#include "tcp-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpServerApplication");
NS_OBJECT_ENSURE_REGISTERED(TcpServer);

TypeId TcpServer::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::TcpServer")
		.SetParent<Application>().AddConstructor<TcpServer>()
		.AddAttribute("Port",
			"Port on which we listen for incoming connections.",
			UintegerValue(7), MakeUintegerAccessor(&TcpServer::m_port),
			MakeUintegerChecker<uint16_t>())

		.AddAttribute("PacketSize",
					"Size of data in outbound packets", UintegerValue(100),
					 MakeUintegerAccessor(&TcpServer::m_size), MakeUintegerChecker<uint32_t>())

		.AddTraceSource("Rx",
			"A packet is received",
			MakeTraceSourceAccessor(&TcpServer::m_packetReceived),
			"ns3::TcpServer::PacketReceivedCallback")

		.AddTraceSource("PacketDropped",
					"Occurs when a packet is dropped",
					MakeTraceSourceAccessor(&TcpServer::m_packetdropped),
					"ns3::TcpServer::PacketDroppedCallback")

		.AddTraceSource(
			"Retransmission", "Occurs when TCP socket does a retransmission",
			MakeTraceSourceAccessor(&TcpServer::m_retransmission),
			"ns3::TcpServer::RetransmissionCallBack")

	.AddTraceSource("TCPStateChanged", "TCP state changed",
			MakeTraceSourceAccessor(&TcpServer::m_tcpStateChanged),
			"ns3::TcpServer::TCPStateChangedCallBack")

			;
	return tid;
}

TcpServer::TcpServer() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	m_running = false;
}

TcpServer::~TcpServer() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void TcpServer::DoDispose(void) {
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose();
}

void TcpServer::StartApplication(void) {
	NS_LOG_FUNCTION_NOARGS ();

	m_running = true;

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);

		InetSocketAddress listenAddress = InetSocketAddress(
				Ipv4Address::GetAny(), m_port);
		m_socket->Bind(listenAddress);
		m_socket->Listen();
	}

	m_socket->SetAcceptCallback(
			MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
			MakeCallback(&TcpServer::HandleAccept, this));
}

void TcpServer::OnRetransmission(Address a) {
	m_retransmission(a);
}

void TcpServer::OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,
		TcpSocket::TcpStates_t newVal, Address a) {
	m_tcpStateChanged(oldVal, newVal, a);
}

void TcpServer::StopApplication() {
	NS_LOG_FUNCTION_NOARGS ();

	m_running = false;

	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetAcceptCallback(
				MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
				MakeNullCallback<void, Ptr<Socket>, const Address &>());
	}
}

void TcpServer::OnConnected(Address from) {

}

void TcpServer::OnDataReceived(Address from) {

}

void TcpServer::ReceivePacket(Ptr<Socket> s) {
	NS_LOG_FUNCTION(this << s);

	Ptr<Packet> packet;
	Address from;
	while (packet = s->RecvFrom(from)) {
		if (packet->GetSize() > 0) {

			m_packetReceived(packet, from);


			NS_LOG_INFO(
					"Server Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());

			// TODO find better way to send SeqTs in the stream, it corrupts the stream when TCP fragmentation occurs
			SeqTsHeader hdr;
			packet->RemoveHeader(hdr);
			packet->RemoveAllPacketTags();
			packet->RemoveAllByteTags();

			int remainingSize = packet->GetSize();

			uint8_t* buf = new uint8_t[remainingSize];

			packet->CopyData(buf, remainingSize);
			for (int i = 0; i < remainingSize; i++) {
				handles[from].rxBuffer.push((char) buf[i]);
			}
			delete buf;
		}

		OnDataReceived(from);
	}
}

void TcpServer::HandleAccept(Ptr<Socket> s, const Address& from) {
	NS_LOG_FUNCTION(this << s << from);

	handles[from].address = from;
	handles[from].callback = MakeCallback(&TcpServer::OnTCPStateChanged, this);

	handles[from].socket = Ptr<Socket>(s);

	s->TraceConnectWithoutContext("Retransmission",
			MakeCallback(&TcpServer::OnRetransmission, this));
	s->TraceConnectWithoutContext("State",
			MakeCallback(&TcpServer::Handler::OnTCPStateChanged,
					&handles[from]));

	s->SetRecvCallback(MakeCallback(&TcpServer::ReceivePacket, this));
	s->SetCloseCallbacks(MakeCallback(&TcpServer::HandleSuccessClose, this),
			MakeNullCallback<void, Ptr<Socket> >());

	OnConnected(from);

}

void TcpServer::HandleSuccessClose(Ptr<Socket> s) {
	NS_LOG_FUNCTION(this << s);
	NS_LOG_LOGIC("Client close received");
	s->Close();
	s->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
	s->SetCloseCallbacks(MakeNullCallback<void, Ptr<Socket> >(),
			MakeNullCallback<void, Ptr<Socket> >());
}

void TcpServer::Write(Address to, char* data, int size) {
	NS_LOG_FUNCTION(to);
	for (int i = 0; i < size; i++)
		handles[to].txBuffer.push(data[i]);

	// send blocks if queue exceeds packet size
	while (handles[to].txBuffer.size() > m_size) {
		uint8_t* block = new uint8_t[m_size];
		for (int i = 0; i < m_size; i++) {
			block[i] = (uint8_t) handles[to].txBuffer.front();
			handles[to].txBuffer.pop();
		}
		Send(to,block, m_size);
		delete block;
	}
}

int TcpServer::Read(Address from, char* data, int size) {
	NS_LOG_FUNCTION(from);
	if (handles[from].rxBuffer.size() == 0)
		return 0;

	int nrOfBytesRead = (
			handles[from].rxBuffer.size() > size ?
					size : handles[from].rxBuffer.size());

	for (int i = 0; i < nrOfBytesRead; i++) {
		data[i] = handles[from].rxBuffer.front();
		handles[from].rxBuffer.pop();
	}

	return nrOfBytesRead;
}

void TcpServer::Flush(Address to) {
	NS_LOG_FUNCTION(to);
	if (handles[to].txBuffer.size() > 0) {
		int size = handles[to].txBuffer.size();

		uint8_t* block = new uint8_t[size];
		for (int i = 0; i < size; i++) {
			block[i] = (uint8_t) handles[to].txBuffer.front();
			handles[to].txBuffer.pop();
		}

		Send(to, block, size);
		delete block;
	}
}


void TcpServer::Send(Address to, uint8_t* data, int size) {
	NS_LOG_FUNCTION_NOARGS ();

	Ptr<Packet> p;
	p = Create<Packet>(data, size);

	// add sequence header to the packet
	SeqTsHeader seqTs;
	seqTs.SetSeq(handles[to].sent);

	p->AddHeader(seqTs);

	int retVal = ((handles[to].socket))->Send(p);
	if(retVal == -1) {
		Socket::SocketErrno err = ((handles[to].socket))->GetErrno();

		if(err == Socket::SocketErrno::ERROR_MSGSIZE) {
			m_packetdropped(to, p, DropReason::TCPTxBufferExceeded);
		}
	}

	handles[to].sent++;

	NS_LOG_INFO("Sent " << m_size << " bytes to " << to);

}


std::string TcpServer:: ReadString(Address from, int size) {

	char* buf = new char[size];
	int nrOfBytesRead = Read(from, buf, size);
	auto msg = std::string(buf,nrOfBytesRead);
	delete buf;

	//std::cout << "S: Read string (length: " << std::to_string((int)msg.size()) << ") '" << msg << "' " << std::endl;

	return msg;
}

void TcpServer::WriteString(Address from, std::string str, bool flush) {

	//std::cout << "S: Write string (length: " << std::to_string((int)str.size()) << ") '" << str << "' " << std::endl;

	char * buf = (char*)str.c_str();
	Write(from, buf, (int)str.size());
	if(flush)
		Flush(from);
}

} // Namespace ns3
