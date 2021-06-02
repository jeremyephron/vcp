/*
 * vcp.cpp
 * 
 * Authors: Jeremy Barenholtz <jeremye@cs.stanford.edu>
 *          Mathew Hogan <mhogan1>
 */

#include "vcp.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("Vcp");
NS_OBJECT_ENSURE_REGISTERED(Vcp);

TypeId
Vcp::GetTypeId()
{
  static TypeId tid = TypeId("ns3::Vcp")
    .SetParent<TcpCongestionOps>()
    .SetGroupName("Internet")
    .AddConstructor<Vcp>()
    .AddAttribute("EstimationInterval",
                  "Interval over which load factor is estimated.",
                  TimeValue(Nanoseconds(kEstIntervalDefault)),
                  MakeTimeAccessor(&Vcp::m_estInterval),
                  MakeTimeChecker())
    .AddAttribute("MIFactor",
                  "The multiplicative increase factor.",
                  DoubleValue(kDefaultXi),
                  MakeDoubleAccessor(&Vcp::m_xi),
                  MakeDoubleChecker())
    .AddAttribute("AIFactor",
                  "The additive increase factor.",
                  DoubleValue(kDefaultAlpha),
                  MakeDoubleAccessor(&Vcp::m_alpha),
                  MakeDoubleChecker())
    .AddAttribute("MDFactor",
                  "The multiplicate decrease factor.",
                  DoubleValue(kDefaultBeta),
                  MakeDoubleAccessor(&Vcp::m_beta),
                  MakeDoubleChecker())
    .AddAttribute("ScaledMIBound",
                  "The upper bound on the scaled MI factor.",
                  DoubleValue(kDefaultXiBound),
                  MakeDoubleAccessor(&Vcp::m_xiBound),
                  MakeDoubleChecker())
  ;

  return tid;
}

Vcp::Vcp() : TcpCongestionOps()
{
  NS_LOG_FUNCTION(this);
}

Vcp::Vcp(const Vcp& sock) : TcpCongestionOps(sock)
{
  NS_LOG_FUNCTION(this);
}

Vcp::~Vcp() {
  m_mdTimer.Cancel();
}

//void
//Vcp::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
//{
//  // If the load bits are not supported, fall back to TCP New Reno
//  if (m_loadState == LOAD_NOT_SUPPORTED) {
//    TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
//  } else {
//    // Do nothing. Window is increased when receiving an ACK. See PktsAcked()
//  }
//}

std::string
Vcp::GetName() const
{
  return "Vcp";
}

Ptr<TcpCongestionOps>
Vcp::Fork()
{
  return CopyObject<Vcp>(this);
}

uint32_t
Vcp::GetSsThresh (Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << state << bytesInFlight);

  return static_cast<uint32_t>(m_cWndFractional);
}

void
Vcp::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  // (VCP) TODO: potential place to do CC
  return;
}

bool
Vcp::HasCongControl() const
{
  return true;
}

void
Vcp::CongControl(
    Ptr<TcpSocketState> tcb,
    const TcpRateOps::TcpRateConnection &rc,
    const TcpRateOps::TcpRateSample &rs)
{
  // return;
  // (VCP) TODO: potential place to do CC
  Time rtt = tcb->m_lastRtt.Get();

  NS_LOG_FUNCTION(this << tcb << &rc << &rs);
  NS_LOG_DEBUG("(VCP) tcb->m_cWnd=" << tcb->m_cWnd);

  // Update load state
  m_loadState = (LoadState_t)tcb->m_vcpLoadIn;
  NS_LOG_DEBUG("(VCP) m_loadState=" << m_loadState);

  if (!m_cWndFractionalInit) {
    m_cWndFractional = static_cast<double>(tcb->m_cWnd);
    m_cWndFractionalInit = true;
  }

  // If the load bits are not supported, fall back to TCP New Reno
  if (m_loadState == LOAD_NOT_SUPPORTED) {
    // TODO: What to do if not supported?
    // m_cWndFractional = static_cast<double>(tcb->m_cWnd);
    return;
  }

  // Update RTT
  m_lastRtt = rtt.GetMilliSeconds();

  // Freeze cwnd after MD
  if (m_mdFreeze && m_mdTimer.IsRunning()) {
    NS_LOG_DEBUG("(VCP) freezing cwnd after MD");
    return;
  } else if (m_mdFreeze && m_mdTimer.IsExpired()) {
    m_mdFreeze = false;
    m_mdTimer.SetFunction(&Vcp::Noop, this);
    m_mdTimer.Schedule(rtt);
    NS_LOG_DEBUG("(VCP) set additive increase RTT=" << rtt);
  }

  // Perform AI for one RTT after 
  if (!m_mdFreeze && m_mdTimer.IsRunning()) {
    NS_LOG_DEBUG("(VCP) one RTT of additive increase after MD freeze period");
    AdditiveIncrease(tcb);
    return;
  } 

  switch (m_loadState) {
    case LOAD_LOW:
      MultiplicativeIncrease(tcb);
      break;
    case LOAD_HIGH:
      AdditiveIncrease(tcb);
      break;
    case LOAD_OVERLOAD:
      MultiplicativeDecrease(tcb);
      m_mdFreeze = true;
      m_mdTimer.SetFunction(&Vcp::Noop, this);
      m_mdTimer.Schedule(Time(m_estInterval * 1000000));
      break;
    default:
      NS_LOG_DEBUG("loadState = " << m_loadState << ", something went wrong.");
      break;
  }
}

void
Vcp::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  // (VCP) TODO: potential place to do CC
  return;
}

void
Vcp::MultiplicativeIncrease(Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION(this << tcb);
  NS_LOG_DEBUG("Previous cwnd = " << tcb->m_cWnd);

  double tmp = m_cWndFractional * (1.f + GetScaledXi(m_lastRtt));
  
  // Avoid overflow
  if (static_cast<uint32_t>(tmp) < tcb->m_cWnd) {
    NS_LOG_DEBUG("(VCP) cwnd overflow!");
    return;
  }
  
  m_cWndFractional = tmp;
  tcb->m_cWnd = static_cast<uint32_t>(m_cWndFractional);

  NS_LOG_DEBUG("New cwnd = " << tcb->m_cWnd);
}

void
Vcp::AdditiveIncrease(Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION(this << tcb);
  NS_LOG_DEBUG("Previous cwnd = " << tcb->m_cWnd);
  NS_LOG_DEBUG("Previous cwndFrac = " << m_cWndFractional);

  double tmp = m_cWndFractional + GetScaledAlpha(m_lastRtt);

  // Avoid overflow
  if (static_cast<uint32_t>(tmp) < tcb->m_cWnd) {
    NS_LOG_DEBUG("(VCP) cwnd overflow!");
    return;
  }

  m_cWndFractional = tmp;
  NS_LOG_DEBUG("New cwndFrac = " << m_cWndFractional);
  tcb->m_cWnd = static_cast<uint32_t>(m_cWndFractional);

  NS_LOG_DEBUG("New cwnd = " << tcb->m_cWnd);
}

void
Vcp::MultiplicativeDecrease(Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION(this << tcb);
  NS_LOG_DEBUG("Previous cwnd = " << tcb->m_cWnd);

  m_cWndFractional = m_cWndFractional * m_beta;
  tcb->m_cWnd = static_cast<uint32_t>(m_cWndFractional);

  NS_LOG_DEBUG("New cwnd = " << tcb->m_cWnd);
}

void
Vcp::Noop()
{
}

void
Vcp::EndMdFreezePeriod()
{
}

} // namespace ns3
