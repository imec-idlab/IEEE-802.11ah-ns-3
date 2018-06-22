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

#ifndef COAP_SERVER_H
#define COAP_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "packet-loss-counter.h"
#include "ns3/traced-callback.h"

#include "coap/coap.h"
//#include "ns3/packet.h"



namespace ns3 {
class Socket;
class Packet;
/**
 * \ingroup applications
 * \defgroup coapclientserver CoapClientServer
 */

/**
 * \ingroup Coapclientserver
 * \class CoapServer
 * \brief A CoAP server, receives CoAP packets from a remote host.
 *
 * CoAP packets carry a 32bits sequence number followed by a 64bits time
 * stamp in their payloads. The application uses the sequence number
 * to determine if a packet is lost, and the time stamp to compute the delay.
 */
class CoapServer : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  CoapServer ();
  virtual ~CoapServer ();
  /**
   * \brief Returns the number of lost packets
   * \return the number of lost packets
   */
  uint32_t GetLost (void) const;

  /**
   * \brief Returns the number of received packets
   * \return the number of received packets
   */
  uint32_t GetReceived (void) const;

  /**
   * \brief Returns the size of the window used for checking loss.
   * \return the size of the window used for checking loss.
   */
  uint16_t GetPacketWindowSize () const;

  /**
   * \brief Set the size of the window used for checking loss. This value should
   *  be a multiple of 8
   * \param size the size of the window used for checking loss. This value should
   *  be a multiple of 8
   */
  void SetPacketWindowSize (uint16_t size);

  uint32_t GetPayloadSize (void);

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
  static uint32_t m_payloadSize; //!< Size of the sent packet (including the SeqTsHeader)

protected:
  virtual void DoDispose (void);

private:
  bool PrepareContext(void);
  //static void HelloHandler(coap_context_t* ctx, coap_resource_t* resource, const coap_endpoint_t* local_interface, coap_address_t* peer, coap_pdu_t* req, str *token, coap_pdu_t* response);

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  
  TracedCallback<Ptr<const Packet>, Address> m_packetReceived;

  /**
   * \brief Handle a packet reception.
   *
   * This function is called by lower layers.
   *
   * \param socket the socket the packet was received to.
   */
  void HandleRead (Ptr<Socket> socket);
  ssize_t CoapHandleMessage(/*Ptr<Packet> packet*/ void);
  ssize_t Send (uint8_t *data, size_t datalen);
  void coap_transaction_id(const coap_pdu_t *pdu, coap_tid_t *id);
#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */
  static void HndGetIndex(coap_context_t *ctx UNUSED_PARAM,
	        struct coap_resource_t *resource UNUSED_PARAM,
	        const coap_endpoint_t *local_interface UNUSED_PARAM,
	        coap_address_t *peer UNUSED_PARAM,
	        coap_pdu_t *request UNUSED_PARAM,
	        str *token UNUSED_PARAM,
	        coap_pdu_t *response);
  static void HndHello(coap_context_t *ctx UNUSED_PARAM,
  	        struct coap_resource_t *resource UNUSED_PARAM,
  	        const coap_endpoint_t *local_interface UNUSED_PARAM,
  	        coap_address_t *peer UNUSED_PARAM,
  	        coap_pdu_t *request UNUSED_PARAM,
  	        str *token UNUSED_PARAM,
  	        coap_pdu_t *response);

  static void HndGetControlValue(coap_context_t *ctx,
          struct coap_resource_t *resource,
          const coap_endpoint_t *local_interface UNUSED_PARAM,
          coap_address_t *peer UNUSED_PARAM,
          coap_pdu_t *request ,
          str *token UNUSED_PARAM,
          coap_pdu_t *response);

  static void HndPutControlValue(coap_context_t *ctx UNUSED_PARAM,
          struct coap_resource_t *resource UNUSED_PARAM,
          const coap_endpoint_t *local_interface UNUSED_PARAM,
          coap_address_t *peer UNUSED_PARAM,
          coap_pdu_t *request,
          str *token UNUSED_PARAM,
          coap_pdu_t *response);

  static void HndDeleteControlValue(coap_context_t *ctx UNUSED_PARAM,
          struct coap_resource_t *resource UNUSED_PARAM,
          const coap_endpoint_t *local_interface UNUSED_PARAM,
          coap_address_t *peer UNUSED_PARAM,
          coap_pdu_t *request UNUSED_PARAM,
          str *token UNUSED_PARAM,
          coap_pdu_t *response UNUSED_PARAM);

  static void InitializeResources(coap_context_t* ctx, std::vector<coap_resource_t*> resources);
  uint16_t m_port; //!< Port on which we listen for incoming packets.
  Ptr<Socket> m_socket; //!< IPv4 Socket
  Ptr<Socket> m_socket6; //!< IPv6 Socket
  uint32_t m_received; //!< Number of received packets
  uint32_t m_prevPacketSeq;
  Time m_prevPacketTime;
  uint32_t m_sent; //!< Counter for sent packets
  PacketLossCounter m_lossCounter; //!< Lost packet counter
  Time m_processingDelay;   //processing delay 10ms
  Ptr<Packet> m_packet;
  EventId m_sendEvent; //!< Event to send the next packet
  Address m_from;
  Ptr<Socket> m_fromSocket;
  std::vector<coap_resource_t*> m_resourceVectorPtr;
  coap_context_t* m_coapCtx;
  uint32_t m_size;//!< Size of the sent packet (including the SeqTsHeader)

  // Control loop atrributes
  static double m_controlValue;
  static double m_sensorValue;
};

} // namespace ns3

#endif /* COAP_SERVER_H */
