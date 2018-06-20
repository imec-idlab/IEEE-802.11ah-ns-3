/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef COAP_HEADER_H
#define COAP_HEADER_H

#include "ns3/header.h"
#include "ns3/nstime.h"
#include "coap/coap.h"

namespace ns3 {
/**
 * \class CoapPdu
 * \brief Protocol Data Unit for CoAP client/server application.
 *
 * The header is made of a 4bytes binary header followed by
 * optional token, options and payload.
 * Max datagram size is 1280. Libcoap limit for PDU is 1400.
 */
class CoapPdu
{
public:
	CoapPdu (uint8_t type, uint8_t code, uint16_t id = 0, size_t size = 4);
	CoapPdu ();

	virtual ~CoapPdu ();

  /**
   * \Message types:
   * 			COAP_MESSAGE_CON
   * 			COAP_MESSAGE_NON
   * 			COAP_MESSAGE_ACK
   * 			COAP_MESSAGE_RST
   */
  void SetType (uint32_t seq);
  uint32_t GetType (void) const;

  void SetTokenLength (uint64_t tkl);

  /**
   * Message Codes: https://tools.ietf.org/html/draft-ietf-core-coap-18#section-12.1
   * */
  uint32_t GetCode (void) const;
  void SetCode (uint32_t code);

  uint64_t GetTokenLength (void) const;
  uint8_t* GetToken(void);
  uint16_t GetId (void) const;
  void SetId (uint16_t id);
  coap_pdu_t* GetPdu (void) const;
  void SetSize (size_t size);
  /**
   * Adds given data to the pdu. Note that the
   * PDU's data is destroyed by coap_add_option(). coap_add_data() must be called
   * only once per PDU, otherwise the result is undefined.
   */
  int AddData (uint32_t size);

  /**
   * Adds token of length @p len to this CoAP header.
   * Adding the token destroys any following contents of the pdu. Hence options
   * and data must be added after AddToken() has been called. In this header,
   * length is set to @p len + @c 4, and max_delta is set to @c 0. This funtion
   * returns @c 0 on error or a value greater than zero on success.
   *
   * @param len  The length of the new token.
   * @param data The token to add.
   *
   * @return     A true on success, or false on error.
   */
  bool SetToken(uint64_t len, const uint8_t *data);

  /**
   * Adds option of given type to this header and destroys the PDU's data, so coap_add_data() must be called
   * after all options have been added. As AddToken() destroys the options
   * following the token, the token must be added before AddOption() is
   * called. This function returns the number of bytes written or @c 0 on error.
   */
  uint64_t AddOption (uint16_t type, uint32_t len, const uint8_t *data);

  /**
   * Adds given data to this CoAP header. Note that the
   * PDU's data is destroyed by AddOption(). AddData() must be called
   * only once per PDU, otherwise the result is undefined.
   */
  bool AddData(uint32_t len, const uint8_t *data);

  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;

private:
  coap_pdu_t* m_coapPdu;
};

} // namespace ns3

#endif /* COAP_PDU_H */
