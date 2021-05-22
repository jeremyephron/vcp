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
    TcpNewReno::IncreaseWindow(tcb, segmentsAcked);
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

  // Update load state
  m_loadState = (LoadState_t)tcb->m_ectCodePoint;

  // If the load bits are not supported, fall back to TCP New Reno
  if (m_loadState == LOAD_NOT_SUPPORTED) {
    TcpNewReno::PktsAcked(tcb, segmentsAcked, rtt);
    return;
  }

  // Update RTT
  m_lastRtt = rtt.GetMilliSeconds();
  NS_LOG_DEBUG("before schedule " << m_mdTimer.IsExpired());
  m_mdTimer.Schedule(rtt);
  NS_LOG_DEBUG("after schedule " << m_mdTimer.IsExpired());

  // Freeze cwnd after MD
  if (m_mdFreeze && !m_mdTimer.IsExpired()) {
    return;
  } else if (m_mdFreeze && m_mdTimer.IsExpired()) {
    m_mdFreeze = false;
    m_mdTimer.Schedule(rtt);
    AdditiveIncrease(tcb);
    return;
  }

  // Perform AI for one RTT after 
  if (!m_mdFreeze && !m_mdTimer.IsExpired()) {
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
      m_mdTimer.Schedule(Time(m_estInterval * 1000000));
      m_mdFreeze = true;
      return;
    default:
      NS_LOG_DEBUG("loadState = " << m_loadState << ", something went wrong.");
      return;
  }
}

void
Vcp::MultiplicativeIncrease(Ptr<TcpSocketState> tcb)
{
  tcb->m_cWnd = tcb->m_cWnd * (1 + GetScaledXi(m_lastRtt));
}

void
Vcp::AdditiveIncrease(Ptr<TcpSocketState> tcb)
{
  tcb->m_cWnd = tcb->m_cWnd + GetScaledAlpha(m_lastRtt);
}

void
Vcp::MultiplicativeDecrease(Ptr<TcpSocketState> tcb)
{
  tcb->m_cWnd = tcb->m_cWnd * m_beta;
}

} // namespace ns3
