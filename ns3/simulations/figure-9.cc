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

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Figure1");

static int UTIL_TRACE_INTERVAL_MS = 500;
static int QUEUE_TRACE_INTERVAL_MS = 10;

static int last_bytes_sent = 0;

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
TraceThroughput1 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[1].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput1, flowMonitor, stream);
}

static void
TraceThroughput2 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[3].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput2, flowMonitor, stream);
}

static void
TraceThroughput3 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[5].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput2, flowMonitor, stream);
}

static void
TraceThroughput4 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[7].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput2, flowMonitor, stream);
}

static void
TraceThroughput5 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[9].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput2, flowMonitor, stream);
}

static void
TraceThroughput6 (Ptr<FlowMonitor> flowMonitor, Ptr<OutputStreamWrapper> stream)
{
  FlowMonitor::FlowStatsContainer stats = flowMonitor->GetFlowStats();
  *stream->GetStream() << stats[11].rxBytes << std::endl;

  Simulator::Schedule(Seconds(0.5), &TraceThroughput2, flowMonitor, stream);
}

static void
TraceUtil (Ptr<OutputStreamWrapper> stream, Ptr<QueueDisc> q)
{
  QueueDisc::Stats stats = q->GetStats();
  double util = ((double) (stats.nTotalSentBytes - last_bytes_sent) / ( UTIL_TRACE_INTERVAL_MS * 1e-3)) / (45 * 1e-6); 
  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << ((double) (stats.nTotalSentBytes - last_bytes_sent) / ( UTIL_TRACE_INTERVAL_MS * 1e-3))
                        << std::endl;
  last_bytes_sent = stats.nTotalSentBytes;
  Simulator::Schedule(MilliSeconds(UTIL_TRACE_INTERVAL_MS),
                      &TraceUtil,
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
int
GetMaxQ(int delay, int bw, int numFlows) {
  return (int) std::max((double) numFlows * 2 * 2, bw * 1e-3 * (delay * 6 * 1e-3) / 8);
}

int
main (int argc, char *argv[])
{
  AsciiTraceHelper asciiTraceHelper;

  /* Start by setting default variables. Feel free to play with the input
   * arguments below to see what happens.
   */
  int bwHost = 1000; // Mbps
  int bwNet = 45; // Mbps
  int delay = 20; // milliseconds
  int time = 600; // seconds
  int maxQ = 100; // packets
  int estInterval = 100; // ms
  double xi = 0.0625;
  double alpha = 1.0;
  double beta = 0.875;
  double xiBound = 1.0;
  double maxCwndInc = 1 + xi;
  std::string transport_prot = "Vcp";

  CommandLine cmd (__FILE__);
  cmd.AddValue ("bwHost", "Bandwidth of host links (Mb/s)", bwHost);
  cmd.AddValue ("bwNet", "Bandwidth of bottleneck (network) link (Mb/s)", bwNet);
  cmd.AddValue ("delay", "Link propagation delay (ms)", delay);
  cmd.AddValue ("time", "Duration (sec) to run the experiment", time);
  cmd.AddValue ("maxQ", "Max buffer size of network interface in packets", maxQ);
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
  cmd.Parse (argc, argv);

  maxQ = GetMaxQueue(delay, bwNet, 6)

  /* NS-3 is great when it comes to logging. It allows logging in different
   * levels for different component (scripts/modules). You can read more
   * about that at https://www.nsnam.org/docs/manual/html/logging.html.
   */
  LogComponentEnable("Figure1", LOG_LEVEL_DEBUG);

  // (VCP): lets packets be printed
  Packet::EnablePrinting();
  Packet::EnableChecking();

  std::string bwHostStr = std::to_string(bwHost) + "Mbps";
  std::string bwNetStr = std::to_string(bwNet) + "Mbps";
  std::string delayStr = std::to_string(delay) + "ms";
  std::string maxQStr = std::to_string(maxQ) + "p";
  transport_prot = std::string("ns3::") + transport_prot;

  NS_LOG_DEBUG("Figure1 Simulation for:" <<
               " bwHost=" << bwHostStr << " bwNet=" << bwNetStr <<
               " delay=" << delayStr << " time=" << time << "sec" <<
               " maxQ=" << maxQStr << " protocol=" << transport_prot);

  /******** Declare output files ********/
  /* Traces will be written on these files for postprocessing. */
  std::string dir = "outputs/bb-q" + std::to_string(maxQ) + "/";

  std::string qStreamName = dir + "q.tr";
  Ptr<OutputStreamWrapper> qStream;
  qStream = asciiTraceHelper.CreateFileStream (qStreamName);

  std::string cwndStreamName = dir + "cwnd.tr";
  Ptr<OutputStreamWrapper> cwndStream;
  cwndStream = asciiTraceHelper.CreateFileStream (cwndStreamName);

  std::string rttStreamName = dir + "rtt.tr";
  Ptr<OutputStreamWrapper> rttStream;
  rttStream = asciiTraceHelper.CreateFileStream (rttStreamName);

  std::string throughputStreamName = dir + "throughput.tr";
  Ptr<OutputStreamWrapper> throughputStream;
  throughputStream = asciiTraceHelper.CreateFileStream(throughputStreamName);

  std::string throughputStreamName2 = dir + "throughput2.tr";
  Ptr<OutputStreamWrapper> throughputStream2;
  throughputStream2 = asciiTraceHelper.CreateFileStream(throughputStreamName2);

  std::string throughputStreamName3 = dir + "throughput3.tr";
  Ptr<OutputStreamWrapper> throughputStream3;
  throughputStream3 = asciiTraceHelper.CreateFileStream(throughputStreamName3);

  std::string throughputStreamName4 = dir + "throughput4.tr";
  Ptr<OutputStreamWrapper> throughputStream4;
  throughputStream4 = asciiTraceHelper.CreateFileStream(throughputStreamName4);

  std::string throughputStreamName5 = dir + "throughput5.tr";
  Ptr<OutputStreamWrapper> throughputStream5;
  throughputStream5 = asciiTraceHelper.CreateFileStream(throughputStreamName5);

  std::string throughputStreamName6 = dir + "throughput6.tr";
  Ptr<OutputStreamWrapper> throughputStream6;
  throughputStream6 = asciiTraceHelper.CreateFileStream(throughputStreamName6);

  std::string utilStreamName = dir + "util.tr";
  Ptr<OutputStreamWrapper> utilStream;
  utilStream = asciiTraceHelper.CreateFileStream(utilStreamName);

  *utilStream << "0 0" << endl;


  /* In order to run simulations in NS-3, you need to set up your network all
   * the way from the physical layer to the application layer. But don't worry!
   * NS-3 has very nice classes to help you out at every layer of the network.
   */

  /******** Create Nodes ********/
  /* Nodes are the used for end-hosts, switches etc. */
  NS_LOG_DEBUG("Creating Nodes...");

  NodeContainer nodes;
  // DONE: Read documentation for NodeContainer object and create 3 nodes.
  nodes.Create(8);

  Ptr<Node> h1 = nodes.Get(0);
  Ptr<Node> h2 = nodes.Get(1);
  Ptr<Node> h3 = nodes.Get(2);
  Ptr<Node> h4 = nodes.Get(3);
  Ptr<Node> h5 = nodes.Get(4);
  Ptr<Node> h6 = nodes.Get(5);
  Ptr<Node> s0 = nodes.Get(6);
  Ptr<Node> h7 = nodes.Get(7);

  /******** Create Channels ********/
  /* Channels are used to connect different nodes in the network. There are
   * different types of channels one can use to simulate different environments
   * such as ethernet or wireless. In our case, we would like to simulate a
   * cable that directly connects two nodes. These cables have particular
   * properties (ie. propagation delay and link rate) and they need to be set
   * before installing them on nodes.
   */
  NS_LOG_DEBUG("Configuring Channels...");

  PointToPointHelper host1Link;
  host1Link.SetDeviceAttribute ("DataRate", StringValue (bwHostStr));
  host1Link.SetChannelAttribute ("Delay", StringValue (delayStr));
  host1Link.SetQueue ("ns3::DropTailQueue", "MaxSize", StringValue ("1p"));

  PointToPointHelper host2Link;
  host2Link.SetDeviceAttribute("DataRate", StringValue(bwHostStr));
  host2Link.SetChannelAttribute("Delay", StringValue(delayStr));
  host2Link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

  PointToPointHelper host3Link;
  host2Link.SetDeviceAttribute("DataRate", StringValue(bwHostStr));
  host2Link.SetChannelAttribute("Delay", StringValue(delayStr));
  host2Link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

  PointToPointHelper host4Link;
  host2Link.SetDeviceAttribute("DataRate", StringValue(bwHostStr));
  host2Link.SetChannelAttribute("Delay", StringValue(delayStr));
  host2Link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

  PointToPointHelper host5Link;
  host2Link.SetDeviceAttribute("DataRate", StringValue(bwHostStr));
  host2Link.SetChannelAttribute("Delay", StringValue(delayStr));
  host2Link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

  PointToPointHelper host6Link;
  host2Link.SetDeviceAttribute("DataRate", StringValue(bwHostStr));
  host2Link.SetChannelAttribute("Delay", StringValue(delayStr));
  host2Link.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

  PointToPointHelper bottleneckLink;
  bottleneckLink.SetDeviceAttribute ("DataRate", StringValue (bwNetStr));
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
  NetDeviceContainer h1s0_NetDevices = host1Link.Install(h1, s0);
  NetDeviceContainer h2s0_NetDevices = host2Link.Install(h2, s0);
  NetDeviceContainer h3s0_NetDevices = host3Link.Install(h3, s0);
  NetDeviceContainer h4s0_NetDevices = host4Link.Install(h4, s0);
  NetDeviceContainer h5s0_NetDevices = host5Link.Install(h5, s0);
  NetDeviceContainer h6s0_NetDevices = host6Link.Install(h6, s0);
  NetDeviceContainer s0h7_NetDevices = bottleneckLink.Install(s0, h7);

  /******** Set TCP defaults ********/
  PppHeader ppph;
  Ipv4Header ipv4h;
  TcpHeader tcph;
  uint32_t tcpSegmentSize = h1s0_NetDevices.Get (0)->GetMtu ()
                            - ppph.GetSerializedSize ()
                            - ipv4h.GetSerializedSize ()
                            - tcph.GetSerializedSize ();
  tcpSegmentSize -= 10; // (VCP): TODO
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

  // DONE: Read documentation for PfifoFastQueueDisc and use the correct
  //       attribute name to set the size of the bottleneck queue.
  TrafficControlHelper tchPfifo;
  tchPfifo.SetRootQueueDisc ("ns3::VcpQueueDisc",
                             "MaxSize", StringValue(maxQStr),
                             "LinkBandwidth", StringValue(bwHostStr),
                             "TimeInterval", TimeValue(MilliSeconds(estInterval)));

  tchPfifo.Install(h1s0_NetDevices);
  tchPfifo.Install(h2s0_NetDevices);
  tchPfifo.Install(h3s0_NetDevices);
  tchPfifo.Install(h4s0_NetDevices);
  tchPfifo.Install(h5s0_NetDevices);
  tchPfifo.Install(h6s0_NetDevices);

  TrafficControlHelper tchPfifo2;
  tchPfifo2.SetRootQueueDisc ("ns3::VcpQueueDisc",
                             "MaxSize", StringValue(maxQStr),
                             "LinkBandwidth", StringValue(bwNetStr),
                             "TimeInterval", TimeValue(MilliSeconds(estInterval)));

  QueueDiscContainer s0h7_QueueDiscs = tchPfifo2.Install (s0h7_NetDevices);
  /* Trace Bottleneck Queue Occupancy */

  /* Set IP addresses of the nodes in the network */
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer h1s0_interfaces = address.Assign (h1s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer h2s0_interfaces = address.Assign (h2s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer h3s0_interfaces = address.Assign (h3s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer h4s0_interfaces = address.Assign (h4s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer h5s0_interfaces = address.Assign (h5s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer h6s0_interfaces = address.Assign (h6s0_NetDevices);
  address.NewNetwork ();
  Ipv4InterfaceContainer s0h7_interfaces = address.Assign (s0h7_NetDevices);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  /******** Setting up the Application ********/
  NS_LOG_DEBUG("Setting up the Application...");

  /* The receiver application */
  uint16_t receiverPort = 5001;
  // DONE: Provide the correct IP address below for the receiver
  AddressValue receiverAddress (InetSocketAddress (s0h7_interfaces.GetAddress(1),
                                                   receiverPort));
  PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                   receiverAddress.Get());
  receiverHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));

  // DONE: Install the receiver application on the correct host.
  ApplicationContainer receiverApp = receiverHelper.Install (h7);
  receiverApp.Start (Seconds (0.0));
  receiverApp.Stop (Seconds ((double)time));

  /* The sender Application */
  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  // DONE: Read documentation for BulkSendHelper to figure out the name of the
  //       Attribute for setting the destination address for the sender.
  ftp.SetAttribute ("Remote", receiverAddress);
  ftp.SetAttribute ("SendSize", UintegerValue (tcpSegmentSize));

  // DONE: Install the source application on the correct host.
  ApplicationContainer sourceApp = ftp.Install (h1);
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds ((double)time));

  ApplicationContainer sourceApp2 = ftp.Install (h2);
  sourceApp2.Start (Seconds (100));
  sourceApp2.Stop (Seconds ((double)time));

  ApplicationContainer sourceApp3 = ftp.Install (h3);
  sourceApp3.Start (Seconds (200));
  sourceApp3.Stop (Seconds ((double)time));

  ApplicationContainer sourceApp4 = ftp.Install (h4);
  sourceApp4.Start (Seconds (300));
  sourceApp4.Stop (Seconds ((double)time));

  ApplicationContainer sourceApp5 = ftp.Install (h5);
  sourceApp5.Start (Seconds (400));
  sourceApp5.Stop (Seconds ((double)time));

  ApplicationContainer sourceApp6 = ftp.Install (h6);
  sourceApp6.Start (Seconds (500));
  sourceApp6.Stop (Seconds ((double)time));

  // Flow tracing
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  Simulator::Schedule(MilliSeconds(UTIL_TRACE_INTERVAL_MS), &TraceUtil, utilStream, s0h7_QueueDiscs.Get(0)) ;

  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceThroughput1, flowMonitor, throughputStream);
  Simulator::Schedule (Seconds (100), &TraceThroughput2, flowMonitor, throughputStream2);
  Simulator::Schedule (Seconds (200), &TraceThroughput3, flowMonitor, throughputStream3);
  Simulator::Schedule (Seconds (300), &TraceThroughput4, flowMonitor, throughputStream4);
  Simulator::Schedule (Seconds (400), &TraceThroughput5, flowMonitor, throughputStream5);
  Simulator::Schedule (Seconds (500), &TraceThroughput6, flowMonitor, throughputStream6);
  
  /******** Run the Actual Simulation ********/
  NS_LOG_DEBUG("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  // DONE: Up until now, you have only set up the simulation environment and
  //       described what kind of events you want NS3 to simulate. However
  //       you have actually not run the simulation yet. Complete the command
  //       below to run it.
  Simulator::Run();

  flowMonitor->SerializeToXmlFile(dir + "flow_stats.xml", true, true);

  Simulator::Destroy ();
  return 0;
}
