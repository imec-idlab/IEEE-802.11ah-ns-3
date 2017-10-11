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
#include "tcp-client.h"
#include "seq-ts-header.h"
#include "ns3/tcp-socket-base.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("TcpClientApplication");
NS_OBJECT_ENSURE_REGISTERED(TcpClient);

TypeId TcpClient::GetTypeId(void) {
	static TypeId tid = TypeId("ns3::TcpClient")
			.SetParent<Application>()
			.AddConstructor<TcpClient>()

		.AddAttribute("RemoteAddress",
			"The destination Ipv4Address of the outbound packets",
			Ipv4AddressValue(),
			MakeIpv4AddressAccessor(&TcpClient::m_peerAddress),
			MakeIpv4AddressChecker()).AddAttribute("RemotePort",
			"The destination port of the outbound packets", UintegerValue(0),
			MakeUintegerAccessor(&TcpClient::m_peerPort),
			MakeUintegerChecker<uint16_t>())
	.AddAttribute("PacketSize",
			"Size of data in outbound packets", UintegerValue(100),
			MakeUintegerAccessor(&TcpClient::SetDataSize,
					&TcpClient::GetDataSize), MakeUintegerChecker<uint32_t>())

	.AddTraceSource("Tx", "A new packet is created and is sent",
			MakeTraceSourceAccessor(&TcpClient::m_txTrace)).AddTraceSource("Rx",
			"A packet is received",
			MakeTraceSourceAccessor(&TcpClient::m_packetReceived),
			"ns3::TcpClient::PacketReceivedCallback").AddTraceSource(
			"CongestionWindow", "The TCP connection's congestion window",
			MakeTraceSourceAccessor(&TcpClient::m_cWnd),
			"ns3::TracedValue::Uint32Callback")

	.AddTraceSource("RTO", "The TCP connection's RTO",
			MakeTraceSourceAccessor(&TcpClient::m_rtoChanged),
			"ns3::Time::TracedValueCallback")

	.AddTraceSource("RTT", "Last sample of the RTT",
			MakeTraceSourceAccessor(&TcpClient::m_rttChanged),
			"ns3::Time::TracedValueCallback")

	.AddTraceSource("Retransmission",
			"Occurs when a packet has to be scheduled for retransmission",
			MakeTraceSourceAccessor(&TcpClient::m_retransmission),
			"ns3::TcpClient::RetransmissionCallBack")

	.AddTraceSource("PacketDropped",
			"Occurs when a packet is dropped",
			MakeTraceSourceAccessor(&TcpClient::m_packetdropped),
			"ns3::TcpClient::PacketDroppedCallback")

	.AddTraceSource("TCPStateChanged", "TCP state changed",
			MakeTraceSourceAccessor(&TcpClient::m_tcpStateChanged),
			"ns3::TcpStatesTracedValueCallback")

	.AddTraceSource("SlowStartThreshold", "TCP slow start threshold (bytes)",
			MakeTraceSourceAccessor(&TcpClient::m_slowStartThresholdChanged),
			"ns3::TracedValue::Uint32Callback")

	.AddTraceSource("EstimatedBW", "The estimated bandwidth (in segments)",
			MakeTraceSourceAccessor(
					&TcpClient::m_currentEstimatedBandwidthChanged),
			"ns3::TracedValue::DoubleCallback");

	;
	return tid;
}

TcpClient::TcpClient() {
	NS_LOG_FUNCTION_NOARGS ();
	m_sent = 0;
	m_bytesSent = 0;
	m_recv = 0;
	m_bytesRecv = 0;
	m_socket = 0;
	m_rv = CreateObject<UniformRandomVariable>();
}

TcpClient::~TcpClient() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
}

void TcpClient::SetRemote(Ipv4Address ip, uint16_t port) {
	m_peerAddress = ip;
	m_peerPort = port;
}

void TcpClient::DoDispose(void) {
	NS_LOG_FUNCTION_NOARGS ();
	m_rv = 0;
	Application::DoDispose();
}

void TcpClient::StartApplication(void) {
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName("ns3::TcpSocketFactory");
		m_socket = Socket::CreateSocket(GetNode(), tid);

		m_socket->TraceConnectWithoutContext("CongestionWindow",
				MakeCallback(&TcpClient::OnCongestionWindowChanged, this));
		m_socket->TraceConnectWithoutContext("Retransmission",
				MakeCallback(&TcpClient::OnRetransmission, this));

		m_socket->TraceConnectWithoutContext("PacketSent",
						MakeCallback(&TcpClient::OnTCPDataPacketSent, this));

		m_socket->TraceConnectWithoutContext("RTO",
				MakeCallback(&TcpClient::OnRTOChanged, this));
		m_socket->TraceConnectWithoutContext("RTT",
				MakeCallback(&TcpClient::OnRTTChanged, this));
		m_socket->TraceConnectWithoutContext("State",
				MakeCallback(&TcpClient::OnTCPStateChanged, this));
		m_socket->TraceConnectWithoutContext("SlowStartThreshold",
				MakeCallback(&TcpClient::OnTCPSlowStartThresholdChanged,
						this));
		m_socket->GetObject<TcpSocketBase>()->GetCongestionControlAlgorithm()->TraceConnectWithoutContext(
				"EstimatedBW",
				MakeCallback(&TcpClient::OnTCPEstimatedBWChanged, this));

		m_socket->Bind();
		m_socket->Connect(InetSocketAddress(m_peerAddress, m_peerPort));
	}

	m_socket->SetRecvCallback(
			MakeCallback(&TcpClient::ReceivePacket, this));
}

void TcpClient::OnRetransmission(Address a) {
	m_retransmission(a);
}

void TcpClient::OnTCPDataPacketSent(Ptr<const Packet> packet, const TcpHeader& header, Ptr<const TcpSocketBase> tcpSocket, bool isRetransmission) {
	if(!isRetransmission)
		m_txTrace(packet);
}

void TcpClient::OnCongestionWindowChanged(uint32_t oldval, uint32_t newval) {
	m_cWnd(oldval, newval / m_socket->GetObject<TcpSocketBase>()->GetSegSize());
}

void TcpClient::OnRTOChanged(Time oldval, Time newval) {
	m_rtoChanged(oldval, newval);
}

void TcpClient::OnRTTChanged(Time oldval, Time newval) {
	m_rttChanged(oldval, newval);
}

void TcpClient::OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,
		TcpSocket::TcpStates_t newVal) {
	m_tcpStateChanged(oldVal, newVal);
}

void TcpClient::OnTCPSlowStartThresholdChanged(uint32_t oldVal,
		uint32_t newVal) {
	m_slowStartThresholdChanged(oldVal, newVal);
}

void TcpClient::OnTCPEstimatedBWChanged(double oldVal, double newVal) {
	m_currentEstimatedBandwidthChanged(oldVal, newVal);
}

void TcpClient::StopApplication() {
	NS_LOG_FUNCTION_NOARGS ();

	// leave the connection open in case there is remaining data
	/*
	if (m_socket != 0) {
		m_socket->Close();
		m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket> >());
		m_socket = 0;
	}
	*/

}

void TcpClient::SetDataSize(uint32_t dataSize) {
	NS_LOG_FUNCTION(dataSize);
	m_size = dataSize;
}

uint32_t TcpClient::GetDataSize(void) const {
	NS_LOG_FUNCTION_NOARGS ();
	return m_size;
}

void TcpClient::Send(uint8_t* data, int size) {
	NS_LOG_FUNCTION_NOARGS ();



	Ptr<Packet> p;
	p = Create<Packet>(data, size);


	// There's no point adding sequence numbers to packets because TCP packets are a stream
	// which can be fragmented. It's impossible to track sequence numbers as soon as packets are fragmented
	// Keep track of the number of data packets sent / received instead and possibly the TCP head sequence
	// of the socket

	// add sequence header to the packet, purely for the time diff calculation
	// TODO find better way to send SeqTs in the stream, it corrupts the stream when TCP fragmentation occurs
	SeqTsHeader seqTs;
	seqTs.SetSeq(m_sent);

	p->AddHeader(seqTs);

	// call to the trace sinks before the packet is actually sent,
	// so that tags added to the packet can be sent as well
	//	m_txTrace(p);

	int retVal = m_socket->Send(p);
	if(retVal == -1) {
		Socket::SocketErrno err = m_socket->GetErrno();

		if(err == Socket::SocketErrno::ERROR_MSGSIZE) {
			m_packetdropped(p,DropReason::TCPTxBufferExceeded);
		}
	}

		++m_sent;

	NS_LOG_INFO("Sent " << p->GetSize() << " bytes to " << m_peerAddress);

}

void TcpClient::OnDataReceived() {

}


void TcpClient::ReceivePacket(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	Ptr<Packet> packet;
	Address from;
	while (packet = socket->RecvFrom(from)) {
		m_packetReceived(packet, from);

		NS_LOG_INFO(
				"Received " << packet->GetSize () << " bytes from " << InetSocketAddress::ConvertFrom (from).GetIpv4 ());

		m_recv++;
		m_bytesRecv += packet->GetSize();

		SeqTsHeader hdr;
		packet->RemoveHeader(hdr);
		packet->RemoveAllPacketTags();
		packet->RemoveAllByteTags();

		uint8_t * buf = new uint8_t[packet->GetSize()];
		packet->CopyData(buf, packet->GetSize());
		for(uint32_t i = 0; i < packet->GetSize(); i++)
			rxBuffer.push((char)buf[i]);
		delete buf;

		OnDataReceived();
	}
}

void TcpClient::Write(char* data, int size) {
	NS_LOG_FUNCTION(this << *data << " , " << size);
	for (int i = 0; i < size; i++)
		txBuffer.push(data[i]);

	// send blocks if queue exceeds packet size
	while (txBuffer.size() > m_size) {
		uint8_t* block = new uint8_t[m_size];
		for (uint32_t i = 0; i < m_size; i++) {
			block[i] = (uint8_t)txBuffer.front();
			txBuffer.pop();
		}
		Send(block, m_size);
		delete block;
	}
}

int TcpClient::Read(char* data, int size) {

	if(rxBuffer.size() == 0)
		return 0;

	int nrOfBytesRead = (static_cast<int>(rxBuffer.size()) > size ? size : static_cast<int>(rxBuffer.size()));

	for(int i = 0; i < nrOfBytesRead; i++) {
		data[i] = rxBuffer.front();
		rxBuffer.pop();
	}

	return nrOfBytesRead;
}

void TcpClient::Flush() {
	NS_LOG_FUNCTION(this);
	if (txBuffer.size() > 0) {
		int size = txBuffer.size();

		uint8_t* block = new uint8_t[size];
		for (int i = 0; i < size; i++) {
			block[i] = (uint8_t)txBuffer.front();
			txBuffer.pop();
		}
		Send(block, size);
		delete block;
	}
}

std::string TcpClient::ReadString(int size) {


	char* buf = new char[size];
	int nrOfBytesRead = Read(buf, size);
	auto msg = std::string(buf,nrOfBytesRead);
	delete buf;

	//std::cout << "C: Read string (length: " << std::to_string((int)msg.size()) << ") '" << msg << "' " << std::endl;
	return msg;
}

void TcpClient::WriteString(std::string str, bool flush) {

	//std::cout << "C: Write string (length: " << std::to_string((int)str.size()) << ") '" << str << "' " << std::endl;

	char * buf = (char*)str.c_str();
	Write(buf, (int)str.size());
	if(flush)
		Flush();
}



} // Namespace ns3
