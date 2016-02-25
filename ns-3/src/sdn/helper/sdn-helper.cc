/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Haoliang Chen
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
 * Author: Haoliang Chen <chl41993@gmail.com>
 */
#include "sdn-helper.h"
#include "../model/sdn-routing-protocol.h"
#include "ns3/node-list.h"
#include "ns3/names.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-list-routing.h"

namespace ns3 {

SdnHelper::SdnHelper ()
{
  m_agentFactory.SetTypeId ("ns3::sdn::RoutingProtocol");
}

SdnHelper::SdnHelper (const SdnHelper &o)
  : m_agentFactory (o.m_agentFactory)
{
  m_mobility = o.m_mobility;
  m_interfaceExclusions = o.m_interfaceExclusions;
}

SdnHelper*
SdnHelper::Copy () const
{
  return new SdnHelper (*this);
}

void
SdnHelper::ExcludeInterface (Ptr<Node> node, uint32_t interface)
{
  std::map< Ptr<Node>, std::set<uint32_t> >::iterator it = m_interfaceExclusions.find (node);

  if(it == m_interfaceExclusions.end ())
    {
      std::set<uint32_t> interfaces;
      interfaces.insert (interface);

      m_interfaceExclusions.insert (std::make_pair (node, std::set<uint32_t> (interfaces) ));
    }
  else
    {
      it->second.insert (interface);
    }
}

Ptr<Ipv4RoutingProtocol>
SdnHelper::Create (Ptr<Node> node) const
{
  Ptr<sdn::RoutingProtocol> agent = m_agentFactory.Create<sdn::RoutingProtocol> ();

  std::map<Ptr<Node>, std::set<uint32_t> >::const_iterator it = m_interfaceExclusions.find (node);

  if(it != m_interfaceExclusions.end ())
    {
      agent->SetInterfaceExclusions (it->second);
    }

  std::map< Ptr<Node>, Ptr<MobilityModel> >::const_iterator it2 = m_mobility.find (node);
  agent->SetMobility (it2->second);

  node->AggregateObject (agent);
  return agent;
}

void
SdnHelper::Set (std::string name, const AttributeValue &value)
{
  m_agentFactory.Set (name, value);
}

int64_t
SdnHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
      NS_ASSERT_MSG (ipv4, "Ipv4 not installed on node");
      Ptr<Ipv4RoutingProtocol> proto = ipv4->GetRoutingProtocol ();
      NS_ASSERT_MSG (proto, "Ipv4 routing not installed on node");
      Ptr<sdn::RoutingProtocol> sdn = DynamicCast<sdn::RoutingProtocol> (proto);
      if (sdn)
        {
          currentStream += sdn->AssignStreams (currentStream);
          continue;
        }
      // Sdn may also be in a list
      Ptr<Ipv4ListRouting> list = DynamicCast<Ipv4ListRouting> (proto);
      if (list)
        {
          int16_t priority;
          Ptr<Ipv4RoutingProtocol> listProto;
          Ptr<sdn::RoutingProtocol> listSdn;
          for (uint32_t i = 0; i < list->GetNRoutingProtocols (); i++)
            {
              listProto = list->GetRoutingProtocol (i, priority);
              listSdn = DynamicCast<sdn::RoutingProtocol> (listProto);
              if (listSdn)
                {
                  currentStream += listSdn->AssignStreams (currentStream);
                  break;
                }
            }
        }
    }
  return (currentStream - stream);

}

void
SdnHelper::SetMobility (Ptr<Node> node, Ptr<MobilityModel> mo)
{
  std::map< Ptr<Node>, Ptr<MobilityModel> >::iterator it = m_mobility.find (node);

  if (it != m_mobility.end())
    {
      std::cout<<"Duplicate Mobility on Node"<< node->GetId ()
          <<". The new one will be used."<<std::endl;
    }
  m_mobility[node] = mo;
}



} // namespace ns3
