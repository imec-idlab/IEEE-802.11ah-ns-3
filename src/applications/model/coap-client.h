/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
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
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef COAP_CLIENT_H
#define COAP_CLIENT_H

#include "ns3-dev/ns3/application.h"
#include "ns3-dev/ns3/event-id.h"
#include "ns3-dev/ns3/ptr.h"
#include "ns3-dev/ns3/ipv4-address.h"
#include "ns3-dev/ns3/traced-callback.h"
#include "ns3-dev/ns3/random-variable-stream.h"

#include "coap-pdu.h"
#define WITH_NS3 1
#include "coap/coap.h"
#include "string.h"


namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup coapclientserver
 * \class CoapClient
 * \brief A Coap client. Sends CoAP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class CoapClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  CoapClient ();

  virtual ~CoapClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IPv4 address
   * \param port remote port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);
  /**
   * \brief set the remote address and port
   * \param ip remote IPv6 address
   * \param port remote port
   */
  void SetRemote (Ipv6Address ip, uint16_t port);
  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);



  coap_context_t* GetContext(void);

  typedef void (* PacketReceivedCallback)
            (Ptr<const Packet>, Address from);

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */
static ssize_t coap_network_send(struct coap_context_t *context UNUSED_PARAM,
  		  const coap_endpoint_t *local_interface,
  		  const coap_address_t *dst,
  		  unsigned char *data,
  		  size_t datalen);

protected:
  virtual void DoDispose (void);

private:
  bool PrepareContext(void);
  ssize_t CoapHandleMessage(Address from, Ptr<Packet> packet);
  void PrepareMsg (void);
  void coap_transaction_id(Address from, const coap_pdu_t *pdu, coap_tid_t *id);
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  TracedCallback<Ptr<const Packet> > m_packetSent;
  TracedCallback<Ptr<const Packet>, Address> m_packetReceived;

  /**
   * \brief Send a packet
   */
  ssize_t Send (uint8_t *data, size_t datalen);
  void HandleRead (Ptr<Socket> socket);
  uint32_t m_count; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  Time m_intervalDeviation;

  //uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  //uint8_t *m_data; //!< packet payload data
  uint32_t m_size; //!< Size of the sent packet (including the SeqTsHeader)
  std::string m_payload; //!< Payload - if empty string then payload of size m_size is automatically generated

  uint32_t m_sent; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet

  Ptr<UniformRandomVariable> m_rv;

  coap_context_t* m_coapCtx;
  coap_pdu_t* m_coapPdu;
  CoapPdu m_coapMsg;
  coap_address_t m_dst_addr;
  coap_tid_t m_tid = COAP_INVALID_TID;
  uint16_t m_method;    // get, post, put, delete
  uint16_t m_type;    // confirmable, non-confirmable, ACK, reset
  static uint8_t m_tokenPrevious[8];
  static uint8_t m_tklPrevious;
  uint8_t m_tokenLength;
  uint64_t m_token;

};

} // namespace ns3

#endif /* COAP_CLIENT_H */
