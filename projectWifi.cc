/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
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
 */

// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//
#include <ctime> 
#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/node.h"
#include "ns3/vector.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");

typedef struct agente{
  int estado;
} agent;

agente a;
NodeContainer c;

int changeState(Ptr<Node> actual_node, NodeContainer container){
  uint32_t nodes_size = container.GetN();
  double minimo = 100000;
  int id_minimo = 0;
  for(uint32_t i = 0; i < nodes_size; ++i){
    double distancia = container.Get(i)->GetObject<MobilityModel>()->GetDistanceFrom(container.Get(actual_node->GetId())->GetObject<MobilityModel>());
    NS_LOG_UNCOND ("Mi Distancia Al Nodo "<< i << " Es: " << distancia << "m");
    if(distancia < minimo && distancia > 0.0){
      minimo = distancia;
      id_minimo = container.Get(i)->GetId();
    }
  }
  NS_LOG_UNCOND ("La Distancia Minima Es: " << minimo << "m \n");
  a.estado = id_minimo;
  return id_minimo;
}

void ReceivePacket (Ptr<Socket> socket){
  while (socket->Recv ()){
    if(a.estado != 0){
      NS_LOG_UNCOND("===========================================================================");
      NS_LOG_UNCOND("MI ESTADO ACTUAL ES: " << socket->GetNode()->GetId() << "\n");
      NS_LOG_UNCOND("EL Nodo en el que estoy, recibio un paquete \n");
      changeState(socket->GetNode(), c);
      
    }
  }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval ){
  if (pktCount > 0){
    socket->Send (Create<Packet> (pktSize));
    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,pktCount - 1, pktInterval);
    if(a.estado == 0){
      NS_LOG_UNCOND("===========================================================================");
      NS_LOG_UNCOND("MI ESTADO ACTUAL ES: " << socket->GetNode()->GetId() << "\n");
      NS_LOG_UNCOND("EL Nodo en el que estoy, envio un paquete De tamano " << pktSize << " Bytes \n");
      changeState(socket->GetNode(), c);
      
    }
  }else{
    socket->Close ();
  }
}

int main (int argc, char *argv[]){
  srand((unsigned)time(0));
  std::string phyMode ("DsssRate1Mbps");
  double rss = -80;  // -dBm
  uint32_t packetSize = (rand()%1024)+1; // bytes
  uint32_t numPackets = (rand()%26)+1;
  double interval = 1.0; // seconds
  bool verbose = false;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  c.Create (5);
  
  uint32_t nNodes = c.GetN ();
  uint32_t nodes_id[nNodes];
  
  for(uint32_t i = 0; i < nNodes; ++i) {
    Ptr<ns3::Node> node_p = c.Get (i);
    nodes_id[i] = node_p->GetId ();
    std::printf("En el nodo: %d \n", nodes_id[i] );
  }
  

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose){
    wifi.EnableLogComponents ();  // Turn on all Wifi logging
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (c);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  InternetStackHelper internet;
  internet.Install (c);
  

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);


  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  for(uint32_t i = 1; i < c.GetN(); ++i ){
    Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (i), tid);
    InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
    recvSink->Bind (local);
    recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
  }

  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);
  
  int count = 2;
  while(count < 100){
    // Output what we are doing
    numPackets = (rand()%260)+1;
    a.estado = source->GetNode ()->GetId ();
    Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                    Seconds (count), &GenerateTraffic,
                                    source, packetSize, numPackets, interPacketInterval);
    count = count+10;
  }

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
