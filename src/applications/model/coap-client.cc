#include "ns3/log.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/network-module.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"

#include "ns3/coap-client.h"

#include "ns3/output-stream-wrapper.h"
#include <cstdlib>
#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>
#include <climits>
#include "arpa/inet.h"
#include "seq-ts-header.h"


#define WITH_SEQ_TS true

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CoapClient");

NS_OBJECT_ENSURE_REGISTERED (CoapClient);

TypeId
CoapClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CoapClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<CoapClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&CoapClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packets", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&CoapClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&CoapClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (COAP_DEFAULT_PORT),
                   MakeUintegerAccessor (&CoapClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
	.AddAttribute ("PayloadSize", "Size of payload in sensor measurements.",
				   UintegerValue (64),
				   MakeUintegerAccessor (&CoapClient::m_size),
				   MakeUintegerChecker<uint32_t> (4,COAP_MAX_PDU_SIZE))
	.AddAttribute ("Payload", "Payload string.",
					StringValue (),
					MakeStringAccessor (&CoapClient::m_payload),
					MakeStringChecker ())
	.AddAttribute ("IntervalDeviation",
					"The possible deviation from the interval to send packets",
					TimeValue (Seconds (0)),
					MakeTimeAccessor (&CoapClient::m_intervalDeviation),
					MakeTimeChecker ())
	.AddAttribute ("RequestMethod",
				   "COAP_REQUEST_GET = 1, COAP_REQUEST_POST = 2, COAP_REQUEST_PUT = 3, COAP_REQUEST_DELETE = 4",
					UintegerValue (3),
					MakeUintegerAccessor (&CoapClient::m_method),
					MakeUintegerChecker<uint16_t> (1,4))
	.AddAttribute ("MessageType",
				   "COAP_MESSAGE_CON = 0, COAP_MESSAGE_NON = 1, COAP_MESSAGE_ACK = 2, COAP_MESSAGE_RST = 3",
					UintegerValue (3),
					MakeUintegerAccessor (&CoapClient::m_type),
					MakeUintegerChecker<uint16_t> (0,3))
	.AddAttribute ("CooldownTime",
					"Time from canceling sending events to closing the socket to give a chance for reception of responses to sent packets.",
					TimeValue (Seconds (0)),
					MakeTimeAccessor (&CoapClient::m_cooldownTime),
					MakeTimeChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&CoapClient::m_packetSent),
                     "ns3::Packet::TracedCallback")
	.AddTraceSource("Rx","Response from CoAP server received",
					MakeTraceSourceAccessor(&CoapClient::m_packetReceived),
					"ns3::CoapClient::PacketReceivedCallback")
  ;
  return tid;
}

/*uint8_t CoapClient::m_tokenPrevious[8] = {0,250,0,0,0,0,0,0};
uint8_t CoapClient::m_tklPrevious = 2;*/

CoapClient::CoapClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();

  m_rv = CreateObject<UniformRandomVariable> ();
  m_coapCtx = NULL;
  m_peerPort = COAP_DEFAULT_PORT;
  //m_tokenLength = 0;
  //m_tokenMsf = 0;
  //m_payload = { 0, NULL };
  //m_optList = NULL;
  // Make token
  		/*int full = 0;
  		for (int i = 7; i >= 0; i--)
  		{
  			if (CoapClient::m_tokenPrevious[i] == 0 && i != 0)
  				continue;
  			else if (i == 0 && CoapClient::m_tokenPrevious[i] == 0){
  				CoapClient::m_tklPrevious = i+1;
  				CoapClient::m_tokenPrevious[i]++;
  				break;
  			}
  			else { //non zero
  				if (CoapClient::m_tokenPrevious[i] < 255){
  					CoapClient::m_tokenPrevious[i]++;
  					break;
  				}
  				else { // if 255
  					if (i < 7 && CoapClient::m_tokenPrevious[i+1] == 0){
  						CoapClient::m_tokenPrevious[i] = 0;
  						CoapClient::m_tokenPrevious[i+1] = 1;
  						CoapClient::m_tklPrevious += 1;
  						break;
  					}
  					else if (i < 7 && CoapClient::m_tokenPrevious[i+1] < 255)
  					{
  						CoapClient::m_tokenPrevious[i] = 0;
  						CoapClient::m_tokenPrevious[i+1]++;
  						break;
  					}
  					else{
  						if (full < 7)
  							full++;
  						i = 7 - full;
  					}
  				}
  			}
  		}
  		m_tokenLength = CoapClient::m_tklPrevious;
  		std::copy (CoapClient::m_tokenPrevious, CoapClient::m_tokenPrevious + CoapClient::m_tklPrevious, m_token);
  		//m_coapMsg.SetToken(m_coapMsg.GetTokenLength(), m_token);*/
  //m_token = 0x56ac;
  //m_tokenLength = m_token;
}

CoapClient::~CoapClient ()
{
  NS_LOG_FUNCTION (this);
  /*if (m_coapCtx)
	  coap_free_context(m_coapCtx);*/
}

void
CoapClient::SetRemote (Ipv4Address ip, uint16_t port = COAP_DEFAULT_PORT)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
CoapClient::SetRemote (Ipv6Address ip, uint16_t port = COAP_DEFAULT_PORT)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = Address(ip);
  m_peerPort = port;
}

void
CoapClient::SetRemote (Address ip, uint16_t port = COAP_DEFAULT_PORT)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

coap_context_t* CoapClient::GetContext(void)
{
	  NS_LOG_FUNCTION (this);
	  return m_coapCtx;
}

// Context in libcoap defines src and dst addresses, sockets and all network parameters needed for successfull communication
// This CoapClient uses libcoap at the application layer, but uses ns3's sockets and all layers below application layer
// One example of simple message exchange is:
// CoapClient::StartApplication ------> CoapClient::PrepareMsg ------> libcoap: coap_send
//																				|
//																				V
//																	   libcoap: coap_network_send
//																				|
//																				V
//																	   CoapClient::coap_network_send ------> CoapClient::Send
//																													|
//																													V	ns-3 socket
//																									 _______________________________
//																									|		       UDP				|
//																									|_______________________________|
//																									|		       IP				|
//																									|_______________________________|
//																									|		   802.11ah (MAC+PHY)	|
//																									|_______________________________|
//
//
bool CoapClient::PrepareContext(void)
{
	  // Prepare dummy source address for libcoap
	  coap_address_t srcAddr;
	  coap_address_init(&srcAddr);

	  if (Ipv4Address::IsMatchingType (m_peerAddress))
	  {
		  Ptr<Ipv4> ip = GetNode()->GetObject<Ipv4>();
		  Ipv4InterfaceAddress iAddr = ip->GetAddress(1,0);
		  std::stringstream addressStringStream;
		  addressStringStream << Ipv4Address::ConvertFrom (iAddr.GetLocal());

		  srcAddr.addr.sin.sin_family      = AF_INET;
		  srcAddr.addr.sin.sin_port        = htons(0);
		  srcAddr.addr.sin.sin_addr.s_addr = inet_addr("0.0.0.0");
		  m_coapCtx = coap_new_context(&srcAddr);
	  }
	  else if (Ipv6Address::IsMatchingType (m_peerAddress))
	  {
		  srcAddr.addr.sin.sin_family      = AF_INET6;
		  srcAddr.addr.sin.sin_port        = htons(0);
		  srcAddr.addr.sin.sin_addr.s_addr = inet_addr("::");
		  m_coapCtx = coap_new_context(&srcAddr);
	  }
	  NS_ASSERT(m_coapCtx != NULL);

	  try
	  {
		  m_coapCtx->network_send = CoapClient::coap_network_send;
		  m_coapCtx->ns3_coap_client_obj_ptr = this;
		  if (m_coapCtx) return true;
		  else return false;
	  }
	  catch(...)
	  {
		  std::cout << "m_coapCtx->network_send = CoapClient::coap_network_send raises an exception. Try again..." << std::endl;
		  PrepareContext();
	  }
}

void
CoapClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_rv = 0;
  Application::DoDispose ();
}

void
CoapClient::StartApplication (void)
{
	NS_LOG_FUNCTION (this);

	if (m_socket == 0)
	{
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
		{
			m_socket->Bind ();
			m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
		}
		else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
		{
			m_socket->Bind6 ();
			m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
		}
	}

	// Prepare context for IPv4 and IPv6
	if (!m_coapCtx){
		if (PrepareContext()) {
			//NS_LOG_DEBUG("CoapClient::Coap context ready.");

		} else{
			NS_LOG_WARN("CoapClient::No context available for the interface. Abort.");
			return;
		}
	}
	// BLOCK2 pertrains to the response payload
	coap_register_option(m_coapCtx, COAP_OPTION_BLOCK2);
	m_socket->SetRecvCallback (MakeCallback (&CoapClient::HandleRead, this));


	PrepareMsg();
}

void
CoapClient::PrepareMsg (void)
{
	std::string ipv4AdrString(""), ipv6AdrString("");

	// Prepare URI
	std::string serverUri("coap://");

	std::stringstream peerAddressStringStream;
	if (Ipv4Address::IsMatchingType (m_peerAddress)){
		peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
		ipv4AdrString = peerAddressStringStream.str ();
		serverUri.append(ipv4AdrString);
	}
	else if (Ipv6Address::IsMatchingType (m_peerAddress)){
		peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
		serverUri.append("[");
		ipv6AdrString = peerAddressStringStream.str ();
		serverUri.append(ipv6AdrString);
		serverUri.append("]");
	}

	// Implemented resources are "control", "hello" and empty resource.
	// For "control" resources implemented methods are GET, PUT and DELETE.
	// For empty resource and "hello" implementeh method is GET.
	serverUri.append("/control"); //.well-known/core
	static coap_uri_t uri;
	int res = coap_split_uri((const unsigned char*)serverUri.c_str(), strlen(serverUri.c_str()), &uri);
	if (res < 0){
		NS_LOG_ERROR("Invalid CoAP URI. Abort.");
		return;
	}

	// Prepare request
	m_coapMsg.SetType(m_type);
	m_coapMsg.SetCode(m_method);
	m_coapMsg.SetId(coap_new_message_id(m_coapCtx));

	// Measurement size of 64B - 12(seqTs) -2-4 m_size - 12
	if (m_size < 14)
	{
		NS_LOG_WARN("Minimum payload needs to be 14 because we store timing information in that payload.");
		return;
	}
	std::string payload(m_size - 2 - 12 - sizeof(coap_hdr_t) - uri.path.length, '1');
	//std::string payload("6740");
	m_coapMsg.SetSize(sizeof(coap_hdr_t) + uri.path.length + strlen(payload.c_str()) + 2); //allocate space for the options (uri) and payload
	m_coapMsg.AddOption(COAP_OPTION_URI_PATH, uri.path.length, uri.path.s);
	coap_add_data(m_coapMsg.GetPdu(), strlen(payload.c_str()), (const unsigned char *)payload.c_str());

	coap_address_init(&m_dst_addr);
	// Prepare destination for libcoap
	m_dst_addr.addr.sin.sin_family = AF_INET;
	m_dst_addr.addr.sin.sin_port = htons(m_peerPort);

	if (ipv4AdrString != "")
		m_dst_addr.addr.sin.sin_addr.s_addr = inet_addr(ipv4AdrString.c_str());
	else
		m_dst_addr.addr.sin.sin_addr.s_addr = inet_addr(ipv6AdrString.c_str());

	// Send CoAP message trough libcoap
	if (m_coapMsg.GetType() == COAP_MESSAGE_CON)
	{
		m_tid = coap_send_confirmed(m_coapCtx, m_coapCtx->endpoint, &m_dst_addr, m_coapMsg.GetPdu());
	}
	else {
		m_tid = coap_send(m_coapCtx, m_coapCtx->endpoint, &m_dst_addr, m_coapMsg.GetPdu());
	}
}

void
CoapClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
  Simulator::Schedule(m_cooldownTime, &CoapClient::DoStopApplication, this);
}

void
CoapClient::DoStopApplication (void)
{
  if (m_socket != 0)
  {
	m_socket->Close ();
	m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	m_socket = 0;
  }
}

ssize_t
CoapClient::Send (uint8_t *data, size_t datalen)
{
	NS_LOG_FUNCTION (this);
	NS_ASSERT (m_sendEvent.IsExpired ());

	Ptr<Packet> p = Create<Packet> (data, datalen);

	// add sequence header to the packet
#ifdef WITH_SEQ_TS
	SeqTsHeader seqTs;
	seqTs.SetSeq (m_sent);
	p->AddHeader (seqTs);
#endif
	std::stringstream peerAddressStringStream;
	if (Ipv4Address::IsMatchingType (m_peerAddress))
		peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
	else if (Ipv6Address::IsMatchingType (m_peerAddress))
		peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);

	// call to the trace sinks before the packet is actually sent,
	// so that tags added to the packet can be sent as well
	m_packetSent(p);

	ssize_t retval;
	if (m_sent < m_count)
	{
		if ((m_socket->Send (p)) >= 0)
		{
			  if (Ipv4Address::IsMatchingType (m_peerAddress))
			    {
				  Ipv4InterfaceAddress iAddr = (GetNode()->GetObject<Ipv4>())->GetAddress(1,0);
				  std::stringstream myaddr;
				  myaddr << Ipv4Address::ConvertFrom (iAddr.GetLocal());
			      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client=" << myaddr.str() << " sent " << p->GetSize() << " bytes to " <<
			                   Ipv4Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort << " seq = " << seqTs.GetSeq() << " ts = " << seqTs.GetTs());
			    }
			  else if (Ipv6Address::IsMatchingType (m_peerAddress))
			    {
			      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client sent " << p->GetSize() << " bytes to " <<
			                   Ipv6Address::ConvertFrom (m_peerAddress) << " port " << m_peerPort);
			    }
			//NS_LOG_INFO ("At time "<< (Simulator::Now ()).GetSeconds ()<< "s client sent " << p->GetSize() << " bytes to " << peerAddressStringStream.str () << " Uid: " << p->GetUid () << " seq = " << m_sent);
			++m_sent;
			retval = p->GetSize();
			/*std::cout << "+++++++++++++++++++++++++++++++++++++Token for dst " << peerAddressStringStream.str () << " is " << m_token;
			for (uint32_t l = 0; l < 8; l++){
				std::cout << std::to_string(m_token[l]) << "   ";
			}
			std::cout << std::endl;*/
		}
		else
		{
			NS_LOG_INFO ("Error while sending " << m_size << " bytes to " << peerAddressStringStream.str ());
			retval = -1;
		}
	}
	if (m_sent < m_count)
	{
		double deviation = m_rv->GetValue(-m_intervalDeviation.GetMicroSeconds(), m_intervalDeviation.GetMicroSeconds());
		m_sendEvent = Simulator::Schedule (m_interval + MicroSeconds(deviation), &CoapClient::PrepareMsg, this);
	}
	return retval;
}

// This method needs to have exactly this signature because it is pointed by a function pointer from coap context (libcoap)
ssize_t
CoapClient::coap_network_send(struct coap_context_t *context UNUSED_PARAM,
		  const coap_endpoint_t *local_interface,
		  const coap_address_t *dst,
		  unsigned char *data,
		  size_t datalen)
{
	CoapClient* clientPtr = static_cast<CoapClient*>(context->ns3_coap_client_obj_ptr);
	return (*clientPtr).Send(data, datalen);
}

ssize_t CoapClient::CoapHandleMessage(Address from, Ptr<Packet> packet){ //coap_network_read extracts coap_packet_t - this is coap_handle_message
	uint32_t msg_len (packet->GetSize());
	ssize_t bytesRead(0);
	uint8_t* msg = new uint8_t[msg_len];
	bytesRead = packet->CopyData (msg, msg_len);

	if (msg_len < sizeof(coap_hdr_t)) {
		NS_LOG_ERROR(this << "Discarded invalid frame CoAP" );
		return 0;
	}

	// check version identifier
	if (((*msg >> 6) & 0x03) != COAP_DEFAULT_VERSION) {
		NS_LOG_ERROR(this << "Unknown CoAP protocol version " << ((*msg >> 6) & 0x03));
		return 0;
	}

	coap_queue_t * node = coap_new_node();
	if (!node) {
		return 0;
	}

	node->pdu = coap_pdu_init(0, 0, 0, msg_len);
	if (!node->pdu) {
		coap_delete_node(node);
		return 0;
	}

	NS_ASSERT(node->pdu);
	coap_pdu_t* sent (m_coapMsg.GetPdu());
	coap_pdu_t* received (node->pdu);
	if (coap_pdu_parse(msg, msg_len, node->pdu)){
		//coap_show_pdu(node->pdu);
		if (COAP_RESPONSE_CLASS(node->pdu->hdr->code) == 2)	{
			// set observation timer TODO
		}
		else if(COAP_RESPONSE_CLASS(node->pdu->hdr->code) > 2 && COAP_RESPONSE_CLASS(node->pdu->hdr->code) <= 5)
		{
			// Error response received. Drop response.
			//std::cout << "---------------------------- "  << std::endl;
			coap_delete_pdu(node->pdu);
			node->pdu = NULL;
			return bytesRead;
		}
	}
	else {
		NS_LOG_ERROR("Coap client cannot parse CoAP packet. Abort.");
		return 0;
	}

	coap_transaction_id(from, node->pdu, &node->id);
	//std::cout << "---------------------------- "  << std::endl;

	// drop if this was just some message, or send reset in case of notification - TODO

	if (!sent && (received->hdr->type == COAP_MESSAGE_CON || received->hdr->type == COAP_MESSAGE_NON))
		coap_send_rst(m_coapCtx, m_coapCtx->endpoint, &m_dst_addr, node->pdu);
	if (received->hdr->type == COAP_MESSAGE_RST) {
		NS_LOG_INFO("Coap client got RST.");
		return 0;
	}
	return bytesRead;
}

void
CoapClient::coap_transaction_id(Address from, const coap_pdu_t *pdu, coap_tid_t *id) {
	coap_key_t h;

	memset(h, 0, sizeof(coap_key_t));

	if (InetSocketAddress::IsMatchingType(from)) {
		std::stringstream addressStringStream;
		addressStringStream << InetSocketAddress::ConvertFrom (from).GetPort();
		std::string port (addressStringStream.str());
		coap_hash((const unsigned char *)port.c_str(), sizeof(port.c_str()), h);
		addressStringStream.str("");
		addressStringStream << InetSocketAddress::ConvertFrom (from).GetIpv4 ();
		std::string ipv4 (addressStringStream.str());
		coap_hash((const unsigned char *)ipv4.c_str(), sizeof(ipv4.c_str()), h);
	}
	else if (Inet6SocketAddress::IsMatchingType(from)) {
		std::stringstream addressStringStream;
		addressStringStream << Inet6SocketAddress::ConvertFrom (from).GetPort();
		std::string port (addressStringStream.str());
		coap_hash((const unsigned char *)port.c_str(), sizeof(port.c_str()), h);
		addressStringStream.str("");
		addressStringStream << Inet6SocketAddress::ConvertFrom (from).GetIpv6();
		std::string ipv6 (addressStringStream.str());
		coap_hash((const unsigned char *)ipv6.c_str(), sizeof(ipv6.c_str()), h);
	}
	else return;

	coap_hash((const unsigned char *)&pdu->hdr->id, sizeof(unsigned short), h);

	*id = (((h[0] << 8) | h[1]) ^ ((h[2] << 8) | h[3])) & INT_MAX;
}

void CoapClient::HandleRead(Ptr<Socket> socket) {
	NS_LOG_FUNCTION(this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from)))
	{
		m_packetReceived(packet, from);
		uint32_t packetSize = packet->GetSize ();
#ifdef WITH_SEQ_TS
		SeqTsHeader seqTs;
		packet->RemoveHeader(seqTs);
		if (InetSocketAddress::IsMatchingType (from))
		{
			Ipv4InterfaceAddress iAddr = (GetNode()->GetObject<Ipv4>())->GetAddress(1,0);
			std::stringstream myaddr;
			myaddr << Ipv4Address::ConvertFrom (iAddr.GetLocal());
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client=" << myaddr.str() << " received " << packetSize << " bytes from " <<
					InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
					InetSocketAddress::ConvertFrom (from).GetPort () << " seq = " << seqTs.GetSeq() << " ts = " << seqTs.GetTs());
		}
		else if (Inet6SocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds () << "s client received " << packetSize << " bytes from " <<
					Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
					Inet6SocketAddress::ConvertFrom (from).GetPort () << " seq = " << seqTs.GetSeq() << " ts = " << seqTs.GetTs());
		}
#endif
		if (!this->CoapHandleMessage(from, packet)) {
			NS_LOG_ERROR("Cannot handle message. Abort.");
			return;
		}
	}
}

} // Namespace ns3
