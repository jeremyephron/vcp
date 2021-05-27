#include "vcp-queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/vcp-packet-tag.h"
#include "ns3/packet.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/tcp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("VcpQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(VcpQueueDisc);

TypeId VcpQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VcpQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName("TrafficControl")
    .AddConstructor<VcpQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The maximum number of packets accepted by this queue disc",
                   QueueSizeValue (QueueSize ("25p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("LinkBandwidth", 
                   "The VCP link bandwidth",
                   DataRateValue (DataRate ("1.5Mbps")),
                   MakeDataRateAccessor (&VcpQueueDisc::m_linkBandwidth),
                   MakeDataRateChecker ())
    .AddAttribute ("TimeInterval", 
                   "Interval over which to sample load factor",
                   TimeValue (MilliSeconds (200)),
                   MakeTimeAccessor (&VcpQueueDisc::m_timeInterval),
                   MakeTimeChecker ())
     .AddAttribute ("SampleInterval", 
                   "Interval over which to sample load factor",
                   TimeValue (MilliSeconds (10)),
                   MakeTimeAccessor (&VcpQueueDisc::m_QueueSampleInterval),
                   MakeTimeChecker ())
     .AddAttribute ("K_q",
                   "K_q factor used while calculating load factor",
                   DoubleValue (0.5),
                   MakeDoubleAccessor (&VcpQueueDisc::m_kq),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("TargetUtil",
                   "Target utilization of link capacity",
                   DoubleValue(1.0),
                   MakeDoubleAccessor (&VcpQueueDisc::m_target_util),
                   MakeDoubleChecker<double> ()) 
  ;

  return tid;
}

VcpQueueDisc::VcpQueueDisc () :
    FifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
  m_queue_size_sample_timer.SetFunction(&VcpQueueDisc::SampleQueueSize, this);
  m_queue_size_sample_timer.Schedule(m_QueueSampleInterval);
}

VcpQueueDisc::~VcpQueueDisc ()
{
    NS_LOG_FUNCTION (this);
    m_queue_size_sample_timer.Cancel();
}

void
VcpQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  QueueDisc::DoDispose ();
}

bool
VcpQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);

  NS_LOG_DEBUG("(VCP) packet=" << item->GetPacket()->ToString());
  NS_LOG_DEBUG("(VCP) VcpQueueDisc::this=" << this);

  // add to recent arrivals queue
  Time now = Simulator::Now();
  recent_packet_arrivals.push(now);

  // remove old (no longer recent arrivals)
  if (!recent_packet_arrivals.empty()) {
    while (now.GetMilliSeconds() - recent_packet_arrivals.front().GetMilliSeconds()
           > m_timeInterval.GetMilliSeconds()) {
      recent_packet_arrivals.pop();
    }
  }

  double persist_q_size = (double) m_qsizes_sum / recent_queue_sizes.size ();
  int recent_arrivals = recent_packet_arrivals.size ();

  // Use naming convention from paper for clarity
  double lambda_l = recent_arrivals;
  double kappa_q = m_kq;
  double q_tilde_l = persist_q_size;
  double gamma_l = m_target_util;
  double C_l = m_linkBandwidth.GetBitRate() / (1000. * 8.); // TODO: divide by 1000?
  double t_rho = m_timeInterval.GetMilliSeconds() / 1000.;
  double load_factor = (lambda_l + kappa_q * q_tilde_l) / (gamma_l * C_l * t_rho);

  NS_LOG_DEBUG("(VCP) lambda_l=" << lambda_l
               << ", kappa_q=" << kappa_q
               << ", q_tilde_l=" << q_tilde_l
               << ", gamma_l=" << gamma_l
               << ", C_l=" << C_l
               << ", t_rho=" << t_rho
               << ", load_factor=" << load_factor);
  
  // Update vcp tag with worst load (highest load factor bits)
  VcpPacketTag vcpTag;
  VcpPacketTag::LoadType prevLoad = VcpPacketTag::LOAD_NOT_SUPPORTED;
  if (item->GetPacket()->PeekPacketTag(vcpTag)) {
    prevLoad = vcpTag.GetLoad();
  }

  if (load_factor < .8) {
    if (prevLoad == VcpPacketTag::LOAD_NOT_SUPPORTED) {
      vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_LOW); 
      NS_LOG_DEBUG("(VCP) previousLoad=" << prevLoad << ", newLoad=LOW");
    }
  } else if (load_factor < 1.) {
    if (prevLoad <= VcpPacketTag::LOAD_LOW) {
      vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_HIGH);
      NS_LOG_DEBUG("(VCP) previousLoad=" << prevLoad << ", newLoad=HIGH");
    }
  } else if (load_factor >= 1.) {
    vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_OVERLOAD);
    NS_LOG_DEBUG("(VCP) previousLoad=" << prevLoad << ", newLoad=OVERLOAD");
  }

  item->GetPacket ()->ReplacePacketTag (vcpTag);

  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      return false;
    }

  bool retval = GetInternalQueue (0)->Enqueue (item);

  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval; 
}

void
VcpQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  
  m_qsizes_sum = 0;
  recent_queue_sizes.push(0); // add one initial queue size sample
}

void
VcpQueueDisc::SampleQueueSize()
{
  NS_LOG_FUNCTION (this);
  if (recent_queue_sizes.size () >=
          (size_t) (m_timeInterval.ToInteger(Time::Unit::MS) /
          m_QueueSampleInterval.ToInteger(Time::Unit::MS)))
  {
    m_qsizes_sum -= recent_queue_sizes.front ();
    recent_queue_sizes.pop ();
  }

  uint32_t cur_size = GetCurrentSize ().GetValue ();

  NS_LOG_DEBUG ("(VCP) Current queue size=" << cur_size);
  
  recent_queue_sizes.push (cur_size);
  m_qsizes_sum += cur_size;

  m_queue_size_sample_timer.SetFunction(&VcpQueueDisc::SampleQueueSize, this);
  m_queue_size_sample_timer.Cancel();
  m_queue_size_sample_timer.Schedule(m_QueueSampleInterval);
}

} // namespace ns3

