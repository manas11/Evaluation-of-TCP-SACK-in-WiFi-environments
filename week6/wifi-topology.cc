/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
 * Network topology:
 *
 *            STA01
 * 
 *   STA00                  STA10
 *   
 *   AP0                    AP1
 * In this example, an HT station sends TCP packets to the access point.
 * We report the total throughput received during a window of 100ms.
 * The user can specify the application data rate and choose the variant
 * of TCP i.e. congestion control algorithm to use.
 * --sack command line option to set Sack ON or OFF
 */
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/yans-error-rate-model.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/table-based-error-rate-model.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/enum.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

NS_LOG_COMPONENT_DEFINE ("wifi-tcp");

using namespace ns3;

Ptr<PacketSink> sink0; /* Pointer to the packet sink application */
Ptr<PacketSink> sink1; /* Pointer to the packet sink application */

uint64_t lastTotalRx0 = 0; /* The value of the last total received bytes */
uint64_t lastTotalRx1 = 0; /* The value of the last total received bytes */

// setting parameters
std::string tcpVariant = "TcpVegas";
// TcpNewReno, TcpHybla,TcpHighSpeed,TcpHtcp, TcpVegas, TcpScalable, TcpVeno,TcpBic, TcpYeah,
//  TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat
double simulationTime = 20;
std::string prefix_file_name = tcpVariant + "-sackOn";
bool sack = true;
// std::string prefix_file_name = tcpVariant + "-sackOff";
// bool sack = false;

static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{

  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd
                        << std::endl;
}

static void
TraceCwnd (std::string cwnd_tr_file_name)
{
  AsciiTraceHelper ascii;
  std::string cwnd_file = cwnd_tr_file_name;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (cwnd_file.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                 MakeBoundCallback (&CwndChange, stream));
}

static void
NextTxTracer (Ptr<OutputStreamWrapper> stream, SequenceNumber32 old, SequenceNumber32 nextTx)
{
  NS_UNUSED (old);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " " << nextTx << std::endl;
}

static void
TraceNextTx (std::string &next_tx_seq_file_name)
{
  AsciiTraceHelper ascii;
  std::string cwnd_file = next_tx_seq_file_name;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream (cwnd_file.c_str ());
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence",
                                 MakeBoundCallback (&NextTxTracer, stream));
}

void
CalculateThroughput ()
{
  Time now = Simulator::Now (); /* Return the simulator's virtual time. */
  double cur0 = (sink0->GetTotalRx () - lastTotalRx0) * (double) 8 /
                1e5; /* Convert Application RX Packets to MBits. */
  std::cout << "Sink0  " << now.GetSeconds () << "s: \t" << cur0 << " Mbit/s" << std::endl;
  lastTotalRx0 = sink0->GetTotalRx ();

  double cur1 = (sink1->GetTotalRx () - lastTotalRx1) * (double) 8 /
                1e5; /* Convert Application RX Packets to MBits. */
  std::cout << "Sink1  " << now.GetSeconds () << "s: \t" << cur1 << " Mbit/s" << std::endl;
  lastTotalRx1 = sink1->GetTotalRx ();

  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472; /* Transport layer payload size in bytes. */
  std::string dataRate = "50Mbps"; /* Application layer datarate. */
  std::string phyRate = "HtMcs7"; /* Physical layer bitrate. */
  bool pcapTracing = true; /* PCAP Tracing is enabled or not. */

  /* Command line argument parser setup. */
  CommandLine cmd (__FILE__);
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("dataRate", "Application data ate", dataRate);
  cmd.AddValue ("tcpVariant",
                "Transport protocol to use: TcpNewReno, "
                "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat ",
                tcpVariant);
  cmd.AddValue ("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue ("sack", "Enable or disable SACK option", sack);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("pcap", "Enable/disable PCAP Tracing", pcapTracing);
  cmd.Parse (argc, argv);

  tcpVariant = std::string ("ns3::") + tcpVariant;
  // Select TCP variant
  if (tcpVariant.compare ("ns3::TcpWestwoodPlus") == 0)
    {
      // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TcpWestwood::GetTypeId ()));
      // the default protocol type in ns3::TcpWestwood is WESTWOOD
      Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));
    }
  else
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcpVariant, &tcpTid),
                           "TypeId " << tcpVariant << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                          TypeIdValue (TypeId::LookupByName (tcpVariant)));
    }

  /* Configure TCP Options */
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  // Setting SACK as ON or OFF
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (sack));

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n_5GHZ);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy0;
  wifiPhy0.SetChannel (wifiChannel.Create ());
  wifiPhy0.SetErrorRateModel ("ns3::YansErrorRateModel");

  YansWifiPhyHelper wifiPhy1;
  wifiPhy1.SetChannel (wifiChannel.Create ());
  wifiPhy1.SetErrorRateModel ("ns3::YansErrorRateModel");

  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyRate), "ControlMode", StringValue ("HtMcs0"));

  ////////////// Topology Begin //////////////////////////////////////
  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (3);

  NodeContainer wifiApNodes;
  wifiApNodes.Create (2);

  ////////////////// AP0 - STA00 -STA01 ///////////////////////////
  Ptr<Node> apWifiNode0 = wifiApNodes.Get (0);
  Ptr<Node> staWifiNode00 = wifiStaNodes.Get (0);
  Ptr<Node> staWifiNode01 = wifiStaNodes.Get (1);

  Ssid ssidA = Ssid ("A");
  ////// STA00
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssidA));
  NetDeviceContainer staDevice00 = wifiHelper.Install (wifiPhy0, wifiMac, staWifiNode00);

  ////// STA01
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssidA));
  NetDeviceContainer staDevice01 = wifiHelper.Install (wifiPhy0, wifiMac, staWifiNode01);

  /////// AP0
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssidA));
  NetDeviceContainer apDevice0 = wifiHelper.Install (wifiPhy0, wifiMac, apWifiNode0);

  ////////////////// AP1 - STA10 ///////////////////////////
  Ptr<Node> apWifiNode1 = wifiApNodes.Get (1);
  Ptr<Node> staWifiNode10 = wifiStaNodes.Get (2);

  Ssid ssidB = Ssid ("B");
  ////// STA10
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssidB));
  NetDeviceContainer staDevice10 = wifiHelper.Install (wifiPhy1, wifiMac, staWifiNode10);

  /////// AP1
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssidB));
  NetDeviceContainer apDevice1 = wifiHelper.Install (wifiPhy1, wifiMac, apWifiNode1);
  ///////////////////////// Topology End ///////////

  /////////////////
  /* Mobility model */
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0)); // AP0
  positionAlloc->Add (Vector (30, 0.0, 0.0)); // AP1
  positionAlloc->Add (Vector (0.0, 120, 0.0)); // STA00
  positionAlloc->Add (Vector (0.0, 130, 0.0)); // STA01
  positionAlloc->Add (Vector (30, 140, 0.0)); // STA10

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNodes);
  mobility.Install (wifiStaNodes);
  ////////////////

  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (wifiApNodes);
  stack.Install (wifiStaNodes);

  ////////////////////// IP Address ////////////
  Ipv4AddressHelper address;

  address.SetBase ("191.167.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface0;
  apInterface0 = address.Assign (apDevice0);
  Ipv4InterfaceContainer staInterface00;
  staInterface00 = address.Assign (staDevice00);
  Ipv4InterfaceContainer staInterface01;
  staInterface01 = address.Assign (staDevice01);

  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface1;
  apInterface1 = address.Assign (apDevice1);
  Ipv4InterfaceContainer staInterface10;
  staInterface10 = address.Assign (staDevice10);
  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ////////////////////// IP Address End ////////////////

  /////////////// Application //////////////////
  /* Install TCP Receiver on the access point */
  PacketSinkHelper sinkHelper0 ("ns3::TcpSocketFactory",
                                InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp0 = sinkHelper0.Install (apWifiNode0);
  sink0 = StaticCast<PacketSink> (sinkApp0.Get (0));

  PacketSinkHelper sinkHelper1 ("ns3::TcpSocketFactory",
                                InetSocketAddress (Ipv4Address::GetAny (), 9));
  ApplicationContainer sinkApp1 = sinkHelper1.Install (apWifiNode1);
  sink1 = StaticCast<PacketSink> (sinkApp1.Get (0));

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server00 ("ns3::TcpSocketFactory",
                        (InetSocketAddress (apInterface0.GetAddress (0), 9)));
  server00.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server00.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server00.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server00.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp00 = server00.Install (staWifiNode00);

  /* Install TCP/UDP Transmitter on the station */
  OnOffHelper server01 ("ns3::TcpSocketFactory",
                        (InetSocketAddress (apInterface0.GetAddress (0), 9)));
  server01.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server01.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server01.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server01.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp01 = server01.Install (staWifiNode01);

  OnOffHelper server1 ("ns3::TcpSocketFactory",
                       (InetSocketAddress (apInterface1.GetAddress (0), 9)));
  server1.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  server1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  server1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  server1.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  ApplicationContainer serverApp10 = server1.Install (staWifiNode10);

  /* Start STA00 application at t=1s and STA01 application at t=2s, STA10 at t=3s*/
  /* Start Applications */
  sinkApp0.Start (Seconds (0.0));
  sinkApp1.Start (Seconds (0.0));

  serverApp00.Start (Seconds (1.0));
  serverApp01.Start (Seconds (1.0));
  serverApp10.Start (Seconds (1.0));

  Simulator::Schedule (Seconds (1.1), &TraceCwnd, prefix_file_name + "-cwnd.data");
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);
  Simulator::Schedule (Seconds (1.1), &TraceNextTx, prefix_file_name + "-next-tx.data");

  //////////////////  Applciation end ///////////////

  /* Enable Traces */
  if (pcapTracing)
    {
      wifiPhy0.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
      wifiPhy0.EnablePcap (prefix_file_name + "AP_0", apDevice0);
      wifiPhy0.EnablePcap (prefix_file_name + "STA_00", staDevice00);
      wifiPhy0.EnablePcap (prefix_file_name + "STA_01", staDevice01);
      wifiPhy1.EnablePcap (prefix_file_name + "AP_1", apDevice1);
      wifiPhy1.EnablePcap (prefix_file_name + "STA_10", staDevice10);
    }

  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();

  double averageThroughput0 = ((sink0->GetTotalRx () * 8) / (1e6 * simulationTime));
  double averageThroughput1 = ((sink1->GetTotalRx () * 8) / (1e6 * simulationTime));

  Simulator::Destroy ();

  std::cout << "\nAverage throughput of 0: " << averageThroughput0 << " Mbit/s" << std::endl;
  std::cout << "\nAverage throughput of 1: " << averageThroughput1 << " Mbit/s" << std::endl;

  return 0;
}