/*
 * vcp.h
 * 
 * Authors: Jeremy Barenholtz <jeremye@cs.stanford.edu>
 *          Mathew Hogan <mhogan1>
 */

#pragma once

#include <cmath>

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-state.h"

namespace ns3 {
  
class Vcp : public TcpCongestionOps {
public:
  static TypeId GetTypeId();

  Vcp();

  Vcp(const Vcp& sock);

  ~Vcp();

  std::string GetName() const;

  void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight);
  Ptr<TcpCongestionOps> Fork();
  void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt);
  void CwndEvent(Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

private:
  /* Load state of the current connection. */
  typedef enum {
    LOAD_LOW = 0,
    LOAD_HIGH,
    LOAD_OVERLOAD
  } LoadState_t;

  LoadState_t m_loadState {LOAD_LOW};

  /* Additive increase factor: cwnd(t + 1) := cwnd(t) + alpha. */
  float m_alpha {1.0};

  /* Multiplicative decrease factor: cwnd(t + 1) := cwnd(t) * beta. */
  float m_beta {0.875};

  /* Multiplicative increase factor: cwnd(t + 1) := cwnd(t) * (1 + xi). */
  float m_xi {0.0625};

  /* Load factor estimation interval in ms. */
  uint32_t m_estInterval {200};

  /* MI, AI, and MD algorithms. */
  void MultiplicativeIncrease();
  void AdditiveIncrease();
  void MultiplicativeDecrease();

  /* Scaled MI and AI params based on flow-specific RTT. */
  inline float GetScaledXi(uint32_t rtt) {
    return pow(1 + m_xi, rtt / m_estInterval) - 1;
  }

  inline float GetScaledAlpha(uint32_t rtt) {
    return m_alpha * (rtt / m_estInterval) * (rtt / m_estInterval);
  }

  uint32_t SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  uint32_t m_cWndCnt {0};

};

} // namespace ns3
