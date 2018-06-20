/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "coap-client-server-helper.h"
#include "ns3/coap-server.h"
#include "ns3/coap-client.h"
#include "ns3/coap-trace-client.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

CoapServerHelper::CoapServerHelper ()
{
}

CoapServerHelper::CoapServerHelper (uint16_t port)
{
  m_factory.SetTypeId (CoapServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
CoapServerHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
CoapServerHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_server = m_factory.Create<CoapServer> ();
      node->AddApplication (m_server);
      apps.Add (m_server);

    }
  return apps;
}

Ptr<CoapServer>
CoapServerHelper::GetServer (void)
{
  return m_server;
}

CoapClientHelper::CoapClientHelper ()
{
}

CoapClientHelper::CoapClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (CoapClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

CoapClientHelper::CoapClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (CoapClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

CoapClientHelper::CoapClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (CoapClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address(address)));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
CoapClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
CoapClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<CoapClient> client = m_factory.Create<CoapClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

CoapTraceClientHelper::CoapTraceClientHelper ()
{
}

CoapTraceClientHelper::CoapTraceClientHelper (Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (CoapTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

CoapTraceClientHelper::CoapTraceClientHelper (Ipv4Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (CoapTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address (address)));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

CoapTraceClientHelper::CoapTraceClientHelper (Ipv6Address address, uint16_t port, std::string filename)
{
  m_factory.SetTypeId (CoapTraceClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (Address (address)));
  SetAttribute ("RemotePort", UintegerValue (port));
  SetAttribute ("TraceFilename", StringValue (filename));
}

void
CoapTraceClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
CoapTraceClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<CoapTraceClient> client = m_factory.Create<CoapTraceClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3
