/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

// Network topology
//
//                Wifi 10.1.2.0
//                 *    *    *
//                 |    |    | 
//       n0   n1   n2   n3   n4
//       |    |    |
//       *    *    *
//       Wifi 10.1.1.0
//
// - Multicast source is at node n0;
// - Multicast forwarded by node n2 onto LAN1;
// - Nodes n0, n1, n2, n3, and n4 receive the multicast frame.
// - Node n4 listens for the data 

#include <iostream>
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiMulticast");

int 
main (int argc, char *argv[])
{
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
  LogComponentEnable ("OnOffApplication", LOG_LEVEL_INFO);

  //
  // Set up default values for the simulation.
  //
  // Select DIX/Ethernet II-style encapsulation (no LLC/Snap header)
  // Config::SetDefault ("ns3::CsmaNetDevice::EncapsulationMode", StringValue ("Dix"));

  CommandLine cmd;
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer c;
  c.Create (5);

  // We will later want two subcontainers of these nodes, for the two LANs
  NodeContainer c0 = NodeContainer (c.Get (0), c.Get (1));
  NodeContainer ap = c.Get (2);
  NodeContainer c1 = NodeContainer (c.Get (3), c.Get (4));

  NS_LOG_INFO ("Build Topology.");
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
 
  WifiMacHelper mac;

  phy.SetChannel (channel.Create ());
  Ssid ssid1 = Ssid ("ns-3-ssid-1");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid1),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer nd0 = wifi.Install (phy, mac, c0);  // First LAN

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid1));
  NetDeviceContainer apDevices1 = wifi.Install (phy, mac, ap);

  phy.SetChannel (channel.Create ());
  Ssid ssid2 = Ssid ("ns-3-ssid-2");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2),
               "ActiveProbing", BooleanValue (false));
  NetDeviceContainer nd1 = wifi.Install (phy, mac, c1);  // Second LAN

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));
  NetDeviceContainer apDevices2 = wifi.Install (phy, mac, ap);

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);


  // NS_LOG_INFO ("Create IPv4 and routing");
  // RipHelper ripRouting;
  // Ipv4ListRoutingHelper listRH;
  // listRH.Add (ripRouting, 0);

  NS_LOG_INFO ("Add IP Stack.");
  InternetStackHelper internet;
  // internet.SetRoutingHelper (listRH);
  internet.Install (c);

  NS_LOG_INFO ("Assign IP Addresses.");
  Ipv4AddressHelper ipv4Addr;
  ipv4Addr.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4Addr.Assign (nd0);
  ipv4Addr.Assign (apDevices1);
  ipv4Addr.SetBase ("10.1.2.0", "255.255.255.0");
  ipv4Addr.Assign (nd1);
  ipv4Addr.Assign (apDevices2);

  NS_LOG_INFO ("Routing.");

  Ptr<Ipv4> ipv4A = ap.Get(0)->GetObject<Ipv4> ();
  ipv4A->SetMetric (1, 2);
  ipv4A->SetUp (1);


  // Ptr<Node> multicastRouter = c.Get (2);  // The node in question
  // Ptr<NetDevice> inputIf = apDevices1.Get (0);  // The input NetDevice
  // NetDeviceContainer outputDevices;  // A container of output NetDevices
  // outputDevices.Add (apDevices2.Get (0));  // (we only need one NetDevice here)

  // routing.AddHostRouteTo (multicastRouter, multicastSource, 
  //                    "10.1.2.1", inputIf, outputDevices);

  // Ptr<Node> sender = c.Get (0);
  // Ptr<NetDevice> senderIf = nd0.Get (0);
  // routing.SetDefaultMulticastRoute (sender, senderIf);

  NS_LOG_INFO ("Create Applications.");
  uint16_t multicastPort = 9;   // Discard port (RFC 863)

  OnOffHelper onoff ("ns3::UdpSocketFactory", 
                     Address (InetSocketAddress ("10.1.2.1", multicastPort)));
  onoff.SetConstantRate (DataRate ("255b/s"));
  onoff.SetAttribute ("PacketSize", UintegerValue (128));
  ApplicationContainer srcC = onoff.Install (c0.Get (0));

  OnOffHelper onoff2 ("ns3::UdpSocketFactory", 
                     Address (InetSocketAddress ("10.1.2.2", multicastPort)));
  onoff2.SetConstantRate (DataRate ("255b/s"));
  onoff2.SetAttribute ("PacketSize", UintegerValue (128));
  ApplicationContainer srcD = onoff2.Install (c0.Get (0));

  srcC.Start (Seconds (1.));
  srcC.Stop (Seconds (10.));
  srcD.Start (Seconds (1.));
  srcD.Stop (Seconds (10.));


  PacketSinkHelper sink1 ("ns3::UdpSocketFactory",
                         InetSocketAddress ("10.1.2.1", multicastPort));
  ApplicationContainer sinkD = sink1.Install (c1.Get (0)); 
  sinkD.Start (Seconds (1.0));
  sinkD.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  PacketSinkHelper sink2 ("ns3::UdpSocketFactory",
                         InetSocketAddress ("10.1.2.2", multicastPort));
  ApplicationContainer sinkE = sink2.Install (c1.Get (1)); // Node n4 
  sinkE.Start (Seconds (1.0));
  sinkE.Stop (Seconds (10.0));

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
