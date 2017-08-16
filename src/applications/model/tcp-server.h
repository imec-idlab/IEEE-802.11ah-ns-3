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

#ifndef TCP_SERVER
#define TCP_SERVER

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/traced-callback.h"
#include "ns3/tcp-socket.h"
#include <map>
#include <queue>
#include "ns3/drop-reason.h"

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup applications
 * \defgroup TcpEcho 
 */

/**
 * \ingroup tcpecho
 * \brief A Tcp Echo server
 *
 * Every packet received is sent back to the client.
 */
class TcpServer : public Application
{
public:

	typedef void (* PacketReceivedCallback)
	            (Ptr<const Packet>, Address from);
	typedef void (* RetransmissionCallBack)
		            (Address);

	typedef void (* TCPStateChangedCallBack)
			(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal, Address address);

	  typedef void (* PacketDroppedCallback)
	                      (Address to, Ptr<Packet> packet, DropReason reason);



  static TypeId GetTypeId (void);
  TcpServer ();
  virtual ~TcpServer ();

protected:
  virtual void DoDispose (void);

  virtual void Flush(Address to);
  virtual void Write(Address to, char* data, int size);
  virtual int Read(Address from, char* data, int size);

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  virtual void OnConnected(Address from);
  virtual void OnDataReceived(Address from);

  /**
   * Will read up to size bytes from the stream and convert it to a string
   * might be smaller if data is not yet available, won't block
   */
  std::string ReadString(Address from, int size);

  void WriteString(Address from, std::string, bool flush);

private:
  void ReceivePacket(Ptr<Socket> socket);
  
  void HandleAccept (Ptr<Socket> s, const Address& from);
  
  void HandleSuccessClose(Ptr<Socket> s);



  void Send(Address to, uint8_t* data, int size);

  void HandleRead (Ptr<Socket> socket);

  void OnRetransmission(Address a);

  void OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal, Address address);

  uint32_t m_size;


  Ptr<Socket> m_socket;
  uint16_t m_port;
  bool m_running;

  TracedCallback<Ptr<const Packet>, Address> m_packetReceived;
  TracedCallback<Address> m_retransmission;
  TracedCallback<Address, Ptr<Packet>, DropReason> m_packetdropped;

  TracedCallback<TcpSocket::TcpStates_t,TcpSocket::TcpStates_t, Address> m_tcpStateChanged;

  struct Handler {
	  Address address;
	  Callback<void, ns3::TcpSocket::TcpStates_t, ns3::TcpSocket::TcpStates_t, Address> callback;

	  Ptr<Socket> socket;
	  std::queue<char> rxBuffer;
	  std::queue<char> txBuffer;

	  int sent = 0;

	  void OnTCPStateChanged(TcpSocket::TcpStates_t oldVal,TcpSocket::TcpStates_t newVal) {
		  callback(oldVal,newVal, address);
	  }
  };

  std::map<Address, Handler> handles;
};

} // namespace ns3

#endif /* TCP_SERVER */

