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

#include "coap-pdu.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include <iomanip>


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CoapPdu");

CoapPdu::CoapPdu (uint8_t type, uint8_t code, uint16_t id, size_t size){
	NS_LOG_FUNCTION (this);
	m_coapPdu = coap_pdu_init(type, code, id, size); // only base header
	//m_coapOpt = NULL;
}

CoapPdu::CoapPdu () : CoapPdu(COAP_MESSAGE_NON, 0) {} //non-confirmable empty base header

CoapPdu::~CoapPdu (){
	if (m_coapPdu)
		coap_delete_pdu(m_coapPdu);
}

void
CoapPdu::SetType (uint32_t type)
{
  NS_LOG_FUNCTION (this << type);
  m_coapPdu->hdr->type = type;
}
uint32_t
CoapPdu::GetType (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coapPdu->hdr->type;
}

void
CoapPdu::SetTokenLength (uint64_t tkl)
{
  NS_LOG_FUNCTION (this << tkl);
  m_coapPdu->hdr->token_length = tkl;
}

uint32_t
CoapPdu::GetCode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coapPdu->hdr->code;
}

void
CoapPdu::SetCode (uint32_t code)
{
	NS_LOG_FUNCTION (this);
	m_coapPdu->hdr->code = code;
}

uint16_t
CoapPdu::GetId (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coapPdu->hdr->id;
}

void
CoapPdu::SetId (uint16_t id)
{
	NS_LOG_FUNCTION (this);
	m_coapPdu->hdr->id = id;
}

uint64_t
CoapPdu::GetTokenLength (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coapPdu->hdr->token_length;
}

coap_pdu_t*
CoapPdu::GetPdu (void) const{
	NS_LOG_FUNCTION (this);
	return m_coapPdu;
}

void
CoapPdu::SetSize (size_t size)
{
  NS_LOG_FUNCTION (this << size);
  if (size <= COAP_MAX_PDU_SIZE && size >= 4)
  {
	  m_coapPdu = coap_pdu_init(GetType(), GetCode(), GetId(), size); // only base header - needs coap_delete_pdu(pdu)
  }
  else
	  NS_LOG_WARN ("Size of " << size << " cannot be set. CoAP pdu size is [4, 1400]. Current setting is basic header size 4.");
}

// Not sure if works - not tested
int
CoapPdu::AddData (uint32_t size){
	//make a string of zerofilled data for payload of size m_size
	std::string payload(size, '0');
	return coap_add_data(m_coapPdu, size, (const unsigned char*)payload.c_str());
}

void
CoapPdu::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "	length: " << GetSerializedSize()
	 << "	\tmessage type: " << GetType()
	 << "	\tcode:" << GetCode()
	 << "	\tmessage id:" << GetId();
}

// Pdu length including header, options and data
uint32_t
CoapPdu::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_coapPdu->length;
}

/**
 * Adds token of length @p len to this header.
 * Adding the token destroys any following contents of the pdu. Hence options
 * and data must be added after coap_add_token() has been called. In this header,
 * length is set to @p len + @c 4, and max_delta is set to @c 0. This funtion
 * returns @c 0 on error or a value greater than zero on success.
 *
 * @param len  The length of the new token.
 * @param data The token to add.
 *
 * @return     A value greater than zero on success, or @c 0 on error.
 */
bool
CoapPdu::SetToken(uint64_t len, const uint8_t *data){
	if (coap_add_token(m_coapPdu, len, data) == 0){
		NS_LOG_INFO("Token not added to CoAP header. Bad Format. Current token is " << this->GetTokenLength() << ".");
		return false;
	}
	else {
		NS_LOG_INFO("Token " << *data << " added to CoAP header.");
		return true;
	}
}

uint8_t*
CoapPdu::GetToken(void){
	if (m_coapPdu->hdr->token){
		NS_LOG_INFO("Token does not exist.");
		return NULL;
	}
	else {
		std::cout << *m_coapPdu->hdr->token <<std::endl;
		return m_coapPdu->hdr->token;
	}
}

/**
 * Adds option and destroys the PDU's data, so coap_add_data() must be called
 * after all options have been added. As coap_add_token() destroys the options
 * following the token, the token must be added before coap_add_option() is
 * called. This function returns the number of bytes written or @c 0 on error.
 */
uint64_t
CoapPdu::AddOption (uint16_t type, uint32_t len, const uint8_t *data){
	uint64_t optSize;
	if((optSize = coap_add_option(m_coapPdu, type, len, data)) == 0){
		NS_LOG_INFO("Option not added to CoAP header.");
		return 0;
	}
	else{
		NS_LOG_INFO("Option "<< type <<" of size " << optSize <<" added to CoAP header.");
		return optSize;
	}
}

bool
CoapPdu::AddData(uint32_t len, const uint8_t *data){
	if (coap_add_data(m_coapPdu, len, data) == 0){
		NS_LOG_INFO("Data too large for CoAP header.");
		return false;
	}
	else {
		NS_LOG_INFO("Data added to CoAP header.");
		return true;
	}
}

} // namespace ns3
