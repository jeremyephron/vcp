#include "vcp-queue-disc.h"
#include "ns3/simulator.h"
#include "ns3/vcp-packet-tag.h"
#include "ns3/packet.h"
#include "ns3/drop-tail-queue.h"

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

  // add to recent arrivals queue
  Time now = Simulator::Now();
  recent_packet_arrivals.push(now);

  // remove old (no longer recent arrivals)
  while (!recent_packet_arrivals.empty() &&
          (now.ToInteger(Time::Unit::MS) -
           recent_packet_arrivals.front().ToInteger(Time::Unit::MS) > 
           m_timeInterval.ToInteger(Time::Unit::MS)))
  {
    recent_packet_arrivals.pop();
  }

  double persist_q_size = (double) m_qsizes_sum / recent_queue_sizes.size ();
  int recent_arrivals = recent_packet_arrivals.size ();

  double load_factor = recent_arrivals + m_kq * persist_q_size /
                       ((m_target_util * m_linkBandwidth.GetBitRate () / 1000 * 8) *
                        (m_timeInterval.ToInteger(Time::Unit::MS) / 1000));
  
  VcpPacketTag vcpTag;
  if (load_factor < 0.8) {
    vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_LOW);
  } else if (load_factor < 1) {
    vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_HIGH);
  } else {
    vcpTag.SetLoad (VcpPacketTag::LoadType::LOAD_OVERLOAD);
  }

  // (VCP): TODO: is there an issue if the tag already exists?
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
  if (recent_queue_sizes.size () >=
          (size_t) (m_timeInterval.ToInteger(Time::Unit::MS) /
          m_QueueSampleInterval.ToInteger(Time::Unit::MS)))
  {
    m_qsizes_sum -= recent_queue_sizes.front ();
    recent_queue_sizes.pop ();
  }

  uint32_t cur_size = GetCurrentSize ().GetValue ();
  recent_queue_sizes.push (cur_size);
  m_qsizes_sum += cur_size;

  m_queue_size_sample_timer.SetFunction(&VcpQueueDisc::SampleQueueSize, this);
  m_queue_size_sample_timer.Cancel();
  m_queue_size_sample_timer.Schedule(m_QueueSampleInterval);
}

} // namespace ns3

