/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 Stanford University
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
 * Author: Serhat Arslan <sarslan@stanford.edu>
 *
 * Bufferbloat Topology
 *
 *    h1-----------------s0-----------------h2
 *
 *  Usage (e.g.):
 *    ./waf --run 'bufferbloat --time=60 --bwNet=10 --delay=1 --maxQ=100'
 */

// DONE: Don't just read the TODO sections in this code.  Remember that one of
//       the goals of this assignment is for you to learn how to use NS-3 :-)

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SingleBottleneckDatapoint");

static double QUEUE_TRACE_INTERVAL_MS = 10;


/* Tracing is one of the most valuable features of a simulation environment.
 * It means we can get to see evolution of any value / state we are interested
 * throughout the simulation. Basically, in NS-3, you set some tracing options
 * for pre-defined TraceSources and provide a function that defines what to do
 * when the traced value changes.
 * The helper functions below are used to trace particular values throughout
 * the simulation. Make sure to take a look at how they are used in the main
 * function. You can learn more about tracing at
 * https://www.nsnam.org/docs/tutorial/html/tracing.html If you are going to
 * work with NS-3 in the future, you will definitely need to read this page.
 */
/*
static void
QueueOccupancyTracer (Ptr<OutputStreamWrapper> stream,
                     uint32_t oldval, uint32_t newval)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
} */

static void
QueueOccupancyTracer (Ptr<OutputStreamWrapper> stream,
                      Ptr<QueueDisc> q)
{
 *stream->GetStream() << Simulator::Now().GetSeconds () << " "
                      << (double) q->GetCurrentSize().GetValue () / q->GetMaxSize().GetValue() << std::endl;
  Simulator::Schedule (MilliSeconds(QUEUE_TRACE_INTERVAL_MS), 
                       &QueueOccupancyTracer,
                       stream,
                       q);
}

static void
BytesSentTracer (Ptr<OutputStreamWrapper> stream, Ptr<QueueDisc> q)
{
  QueueDisc::Stats stats = q->GetStats();
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                       << stats.nTotalSentBytes << std::endl;
}

static void
PacketDropsTracer (Ptr<OutputStreamWrapper> stream, Ptr<QueueDisc> q)
{
  QueueDisc::Stats stats = q->GetStats();
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                       << stats.nTotalDroppedPackets << " drops" << std::endl;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                       << stats.nTotalDroppedPackets + stats.nTotalSentPackets
                       << " total" << std::endl;
}

// bw in Kb/s
int
GetMaxQ(int delay, int bw, int numFlows) {
  return (int) std::max((double) numFlows * 2 * 2, bw * (delay * 6 * 1e-9) / 8);
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;

  /* Start by setting default variables. Feel free to play with the input
   * arguments below to see what happens.
   */
  int bwBottleneck = 150; // Kbps
  int delay = 13333333; // nanoseconds
  int time = 120; // seconds
  int numFlows = 50;
  int estInterval = 200; // ms
  double xi = 0.0625;
  double alpha = 1.0;
  double beta = 0.875;
  double xiBound = 1.0;
  double maxCwndInc = 1 + xi;
  std::string transport_prot = "Vcp";
  std::string dir = "outputs/single-bottle/";

  CommandLine cmd (__FILE__);
  // varied in each of Figure 3, 4, 5:
  cmd.AddValue ("bwBottleneck", "Bandwidth of bottleneck links (Kb/s)", bwBottleneck);
  cmd.AddValue("numFlows", "Number of forward and reverse flows (each)", numFlows);
  cmd.AddValue ("delay", "Link propagation delay (ms)", delay);
  
  // Stays the same ine each of figure 3, 4, 5:
  cmd.AddValue ("time", "Duration (sec) to run the experiment", time);
  cmd.AddValue ("transport_prot", "Transport protocol to use: TcpNewReno, "
                "TcpLinuxReno, TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, "
                "TcpScalable, TcpVeno, TcpBic, TcpYeah, TcpIllinois, "
                "TcpWestwood, TcpLedbat, TcpLp, TcpDctcp, Vcp",
                transport_prot);
  cmd.AddValue ("estInterval", "VCP estimation interval (ms)", estInterval);
  cmd.AddValue ("xi", "MI factor", xi);
  cmd.AddValue ("alpha", "AI factor", alpha);
  cmd.AddValue ("beta", "MD factor", beta);
  cmd.AddValue ("xiBound", "Upper bound on scaled MI factor", xiBound);
  cmd.AddValue ("maxCwndInc", "Maximum fraction by which cwnd can increase per RTT", maxCwndInc);
  cmd.AddValue ("dir", "The directory to write outputs to", dir);
  cmd.Parse (argc, argv);

  // calculate max queue size according to formula from paper: 
  int maxQ = GetMaxQ(delay, bwBottleneck, numFlows); // packets

  int bwNonBottleneck = bwBottleneck * 10; // Kbps

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("SingleBottleneckDatapoint", LOG_LEVEL_DEBUG);

  // (VCP): lets packets be printed
  Packet::EnablePrinting();
  Packet::EnableChecking();

  std::string bwBottleneckStr = std::to_string(bwBottleneck) + "Kbps";
  std::string delayStr = std::to_string(delay) + "ns";
  std::string maxQStr = std::to_string(maxQ) + "p";
  std::string bwNonBottleneckStr = std::to_string(bwNonBottleneck) + "Kbps";
  transport_prot = std::string("ns3::") + transport_prot;

  NS_LOG_DEBUG("Sinlge Bottleneck Simulation for:" <<
               " bwBottleneck=" << bwBottleneckStr <<
               " delay=" << delayStr << 
               " numFlows=" << numFlows <<
               " time=" << time << "sec" <<
               " maxQ=" << maxQStr << " protocol=" << transport_prot);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  //std::string dir = "outputs/single-bottle/"; //TODO what is the right name here

  std::string qStreamName = dir + "q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  std::string bytesSentStreamName = dir + "bytesSent.tr";
  Ptr<OutputStreamWrapper> bytesSentStream;
  bytesSentStream = asciiTraceHelper.CreateFileStream(bytesSentStreamName);

  std::string packetDropsStreamName = dir + "drops.tr";
  Ptr<OutputStreamWrapper> packetDropsStream;
  packetDropsStream = asciiTraceHelper.CreateFileStream(packetDropsStreamName);

  /* In order to run simulations in NS-3, you need to set up your network all
   * the way from the physical layer to the application layer. But don't worry!
   * NS-3 has very nice classes to help you out at every layer of the network.
   */

  /******** Create Nodes ********/
  /* Nodes are the used for end-hosts, switches etc. */
  NS_LOG_DEBUG("Creating Nodes...");

  NodeContainer nodes;
  // DONE: Read documentation for NodeContainer object and create 3 nodes.
  nodes.Create(numFlows * 2 + 2);

  Ptr<Node> s0 = nodes.Get(0);
  Ptr<Node> s1 = nodes.Get(1);

  /******** Create Channels ********/
  /* Channels are used to connect different nodes in the network. There are
   * different types of channels one can use to simulate different environments
   * such as ethernet or wireless. In our case, we would like to simulate a
   * cable that directly connects two nodes. These cables have particular
   * properties (ie. propagation delay and link rate) and they need to be set
   * before installing them on nodes.
   */
  NS_LOG_DEBUG("Configuring Channels...");

  std::vector<NetDeviceContainer> netDevices; 
  uint32_t tcpSegmentSize;
  for (int i = 2; i < numFlows * 2 + 2; i++) {
    Ptr<Node> h1 = nodes.Get(i); // sender
    PointToPointHelper hostLink;
    hostLink.SetDeviceAttribute ("DataRate", StringValue (bwNonBottleneckStr));
    hostLink.SetChannelAttribute ("Delay", StringValue (delayStr));
    hostLink.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

    if (i % 2 == 1) {
      NetDeviceContainer h1s0_NetDevices = hostLink.Install(h1, s0);
      netDevices.push_back(h1s0_NetDevices);
    } else {
      NetDeviceContainer h1s0_NetDevices = hostLink.Install(s1, h1);
      netDevices.push_back(h1s0_NetDevices);
    }
    
    PppHeader ppph;
    Ipv4Header ipv4h;
    TcpHeader tcph;
    
    tcpSegmentSize = netDevices[i - 2].Get (0)->GetMtu ()
                            - ppph.GetSerializedSize ()
                            - ipv4h.GetSerializedSize ()
                            - tcph.GetSerializedSize ();

  } 

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bwBottleneckStr));
  bottleneckLink.SetChannelAttribute ("Delay", StringValue (delayStr));
  bottleneckLink.SetQueue ("ns3::DropTailQueue",
                           "MaxSize", StringValue ("1p"));

  /******** Create NetDevices ********/
  NS_LOG_DEBUG("Creating NetDevices...");

  /* When channels are installed in between nodes, NS-3 creates NetDevices for
   * the associated nodes that simulate the Network Interface Cards connecting
   * the channel and the node.
   */
  // DONE: Read documentation for PointToPointHelper object and install the
  //       links created above in between correct nodes.
  NetDeviceContainer s0h2_NetDevices = bottleneckLink.Install(s0, s1);

  /******** Set TCP defaults ********/
  tcpSegmentSize -= 10; // (VCP): TODO
  // TODO what is this
  tcpSegmentSize -= 448;
  tcpSegmentSize -= 52;

  NS_LOG_DEBUG("(VCP) tcpSegSize=" << tcpSegmentSize);

  Config::SetDefault ("ns3::TcpSocket::SegmentSize",
                      UintegerValue (tcpSegmentSize));
  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 21));
  Config::SetDefault ("ns3::TcpSocketBase::Sack", BooleanValue (false));
  Config::SetDefault ("ns3::TcpL4Protocol::RecoveryType",
                      TypeIdValue (TypeId::LookupByName ("ns3::TcpClassicRecovery")));
  Config::SetDefault ("ns3::TcpSocket::DelAckCount", UintegerValue (0)); 
  Config::SetDefault ("ns3::TcpSocket::DelAckTimeout", TimeValue (Time (0)));
  Config::SetDefault ("ns3::TcpSocket::InitialCwnd", UintegerValue (1));
  Config::SetDefault ("ns3::TcpSocketState::EnablePacing", BooleanValue (false));

  // Select TCP variant
  TypeId tcpTid;
  NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (transport_prot, &tcpTid),
                       "TypeId " << transport_prot << " not found");
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType",
                      TypeIdValue (TypeId::LookupByName (transport_prot)));

  // Set VCP params
  Config::SetDefault ("ns3::Vcp::EstimationInterval",
                      TimeValue(MilliSeconds(estInterval)));
  Config::SetDefault ("ns3::Vcp::MIFactor",
                      DoubleValue(xi));
  Config::SetDefault ("ns3::Vcp::AIFactor",
                      DoubleValue(alpha));
  Config::SetDefault ("ns3::Vcp::MDFactor",
                      DoubleValue(beta));
  Config::SetDefault ("ns3::Vcp::ScaledMIBound",
                      DoubleValue(xiBound));
  Config::SetDefault ("ns3::Vcp::MaxCWndIncreasePerRtt",
                      DoubleValue(maxCwndInc));
  Config::SetDefault ("ns3::Vcp::SegSize",
                      UintegerValue(tcpSegmentSize));

  /******** Install Internet Stack ********/
  NS_LOG_DEBUG("Installing Internet Stack...");

  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  std::vector<Ipv4InterfaceContainer> interfaces;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  for (int i = 2; i < numFlows * 2 + 2; i++) {
    // DONE: Read documentation for PfifoFastQueueDisc and use the correct
    //       attribute name to set the size of the bottleneck queue.
    TrafficControlHelper tchPfifo;
    tchPfifo.SetRootQueueDisc ("ns3::VcpQueueDisc",
                               "MaxSize", StringValue(maxQStr),
                               "LinkBandwidth", StringValue(bwNonBottleneckStr),
                               "TimeInterval", TimeValue(MilliSeconds(estInterval)));
    tchPfifo.Install(netDevices[i]);

    Ipv4InterfaceContainer h1s0_interfaces = address.Assign (netDevices[i - 2]);
    address.NewNetwork ();
    interfaces.push_back(h1s0_interfaces);
  } 


  TrafficControlHelper tchPfifo2;
  tchPfifo2.SetRootQueueDisc ("ns3::VcpQueueDisc",
                             "MaxSize", StringValue(maxQStr),
                             "LinkBandwidth", StringValue(bwBottleneckStr),
                             "TimeInterval", TimeValue(MilliSeconds(estInterval)));

  QueueDiscContainer s0h2_QueueDiscs = tchPfifo2.Install (s0h2_NetDevices);
  /* Trace Bottleneck Queue Occupancy */
  //s0h2_QueueDiscs.Get(0)->TraceConnectWithoutContext ("PacketsInQueue",
  //                          MakeBoundCallback (&QueueOccupancyTracer, qStream));

  /* Set IP addresses of the nodes in the network */
  Ipv4InterfaceContainer s0h2_interfaces = address.Assign (s0h2_NetDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /******** Setting up the Application ********/
  NS_LOG_DEBUG("Setting up the Applications...");

  // DONE: Install the source application on the correct host.
  for (int i = 2; i < numFlows * 2 + 2; i += 2) {

    /* The receiver application */
    uint16_t receiverPort = 5001;
    // DONE: Provide the correct IP address below for the receiver
    AddressValue receiverAddress (InetSocketAddress (interfaces[i - 2].GetAddress(1),
                                                     receiverPort));
    PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                     receiverAddress.Get());
    receiverHelper.SetAttribute ("Protocol",
                                 TypeIdValue (TcpSocketFactory::GetTypeId ()));

    // DONE: Install the receiver application on the correct host.
    ApplicationContainer receiverApp = receiverHelper.Install (nodes.Get(i));
    receiverApp.Start (Seconds (0.0));
    receiverApp.Stop (Seconds ((double)time));

    /* The sender Application */
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    // DONE: Read documentation for BulkSendHelper to figure out the name of the
    //       Attribute for setting the destination address for the sender.
    ftp.SetAttribute ("Remote", receiverAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));


    ApplicationContainer sourceApp = ftp.Install (nodes.Get(i + 1));
    sourceApp.Start (Seconds ((double) (delay * 4 / 1000) * ((i - 2) / numFlows)));
    sourceApp.Stop (Seconds ((double)time));
  } 

  for (int i = 2; i < numFlows * 2 + 2; i += 2) {
    /* The receiver application */
    uint16_t receiverPort = 5001;
    // DONE: Provide the correct IP address below for the receiver
    AddressValue receiverAddress (InetSocketAddress (interfaces[i - 2 + 1].GetAddress(0),
                                                     receiverPort));
    PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                     receiverAddress.Get());
    receiverHelper.SetAttribute ("Protocol",
                                 TypeIdValue (TcpSocketFactory::GetTypeId ()));

    // DONE: Install the receiver application on the correct host.
    ApplicationContainer receiverApp = receiverHelper.Install (nodes.Get(i));
    receiverApp.Start (Seconds (0.0));
    receiverApp.Stop (Seconds ((double)time));

    /* The sender Application */
    BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
    // DONE: Read documentation for BulkSendHelper to figure out the name of the
    //       Attribute for setting the destination address for the sender.
    ftp.SetAttribute ("Remote", receiverAddress);
    ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));


    ApplicationContainer sourceApp = ftp.Install (nodes.Get(i));
    sourceApp.Start (Seconds ((double) (delay * 4 / 1000) * ((i - 2) / numFlows)));
    sourceApp.Stop (Seconds ((double)time));
  } 

  Simulator::Schedule (Seconds (time / 5), &BytesSentTracer, bytesSentStream, s0h2_QueueDiscs.Get(0));
  Simulator::Schedule (Seconds (time), &BytesSentTracer, bytesSentStream, s0h2_QueueDiscs.Get(0));
  Simulator::Schedule (Seconds (time), &PacketDropsTracer, packetDropsStream, s0h2_QueueDiscs.Get(0));
  Simulator::Schedule (MilliSeconds(QUEUE_TRACE_INTERVAL_MS), 
                       &QueueOccupancyTracer,
                       qStream,
                       s0h2_QueueDiscs.Get(0));


  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  // DONE: Up until now, you have only set up the simulation environment and
  //       described what kind of events you want NS3 to simulate. However
  //       you have actually not run the simulation yet. Complete the command
  //       below to run it.
  Simulator::Run();

  Simulator::Destroy ();
  return 0;
}
