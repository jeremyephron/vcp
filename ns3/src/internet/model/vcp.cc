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
    .SetParent<TcpNewReno>()
    .SetGroupName("Internet")
    .AddConstructor<Vcp>()
  ;

  return tid;
}

Vcp::Vcp() : TcpNewReno()
{
  NS_LOG_FUNCTION(this);
}

Vcp::Vcp(const Vcp& sock) : TcpNewReno(sock)
{
  NS_LOG_FUNCTION(this);
}

Vcp::~Vcp() {}

void
Vcp::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  // If the load bits are not supported, fall back to TCP New Reno
  if (m_loadState == LOAD_NOT_SUPPORTED) {
    // TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
  } else {
    // Do nothing. Window is increased when receiving an ACK. See PktsAcked()
  }
}

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

void
Vcp::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt)
{
  NS_LOG_FUNCTION(this << tcb << segmentsAcked << rtt);
  tcb->m_cWnd = 14580*15;
  return;

  // Update load state
  m_loadState = (LoadState_t)tcb->m_vcpLoad;
  NS_LOG_DEBUG("(VCP) m_loadState=" << m_loadState);

  if (!m_cWndFractionalInit) {
    m_cWndFractional = static_cast<double>(tcb->m_cWnd);
    m_cWndFractionalInit = true;
  }

  // If the load bits are not supported, fall back to TCP New Reno
  if (m_loadState == LOAD_NOT_SUPPORTED) {
    // TODO: What to do if not supported?
    // TcpNewReno::PktsAcked(tcb, segmentsAcked, rtt);
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
  }

  // Perform AI for one RTT after 
  if (!m_mdFreeze && m_mdTimer.IsRunning()) {
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
      return;
    default:
      NS_LOG_DEBUG("loadState = " << m_loadState << ", something went wrong.");
      return;
  }
}

void
Vcp::MultiplicativeIncrease(Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION(this << tcb);
  NS_LOG_DEBUG("Previous cwnd = " << tcb->m_cWnd);

  m_cWndFractional = m_cWndFractional * (1.f + GetScaledXi(m_lastRtt));
  tcb->m_cWnd = static_cast<uint32_t>(m_cWndFractional);

  NS_LOG_DEBUG("New cwnd = " << tcb->m_cWnd);
}

void
Vcp::AdditiveIncrease(Ptr<TcpSocketState> tcb)
{
  NS_LOG_FUNCTION(this << tcb);
  NS_LOG_DEBUG("Previous cwnd = " << tcb->m_cWnd);
  NS_LOG_DEBUG("Previous cwndFrac = " << m_cWndFractional);

  m_cWndFractional = m_cWndFractional + GetScaledAlpha(m_lastRtt);
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
