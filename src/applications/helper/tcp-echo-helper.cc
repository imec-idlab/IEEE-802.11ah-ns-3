/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Modified by Frank Helbert <frank@ime.usp.br>,
 * Luiz Arthur Feitosa dos Santos <luizsan@ime.usp.br> and
 * Rodrigo Campiolo <campiolo@ime.usp.br>
 */

#include "tcp-echo-helper.h"
#include "ns3/tcp-echo-server.h"
#include "ns3/tcp-echo-client.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

  
TcpEchoServerHelper::TcpEchoServerHelper (uint16_t port)
{
  m_factory.SetTypeId (TcpEchoServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

void
TcpEchoServerHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
TcpEchoServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpEchoServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpEchoServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
TcpEchoServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<TcpEchoServer> ();
  node->AddApplication (app);

  return app;
}

///ami
/*
TcpEchoClientHelper::TcpEchoClientHelper (Ipv6Address address, uint16_t port)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", Ipv6AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}*/

TcpEchoClientHelper::TcpEchoClientHelper (Ipv4Address address, uint16_t port)
{
  m_factory.SetTypeId (TcpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", Ipv4AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

void
TcpEchoClientHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
TcpEchoClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<TcpEchoClient>()->SetFill (fill);
}

void
TcpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<TcpEchoClient>()->SetFill (fill, dataLength);
}

void
TcpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<TcpEchoClient>()->SetFill (fill, fillLength, dataLength);
}

ApplicationContainer
TcpEchoClientHelper::Install (Ptr<Node> node)
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpEchoClientHelper::Install (std::string nodeName)
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
TcpEchoClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;

      m_client = m_factory.Create<TcpEchoClient> ();
      node->AddApplication (m_client);
      apps.Add (m_client);

    }

  return apps;
}

Ptr<Application>
TcpEchoClientHelper::InstallPriv (Ptr<Node> node)
{
  m_client = m_factory.Create<TcpEchoClient> ();
  node->AddApplication (m_client);

  return m_client;
}

Ptr<TcpEchoClient>
TcpEchoClientHelper::GetClient (void)
{
  return m_client;
}

} // namespace ns3
