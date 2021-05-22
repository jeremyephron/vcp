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
#include "ns3/timer.h"

namespace ns3 {
  
class Vcp : public TcpNewReno {
public:
  static TypeId GetTypeId();

  Vcp();

  Vcp(const Vcp& sock);

  ~Vcp();

  std::string GetName() const;

  void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt) override;

  void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
  Ptr<TcpCongestionOps> Fork() override;

private:
  /* Load state of the current connection. */
  typedef enum {
    LOAD_NOT_SUPPORTED = 0x0,
    LOAD_LOW           = 0x1,
    LOAD_HIGH          = 0x2,
    LOAD_OVERLOAD      = 0x3,
  } LoadState_t;

  /* Additive increase factor: cwnd(t + 1) := cwnd(t) + alpha. */
  const float m_alpha {1.0};

  /* Multiplicative decrease factor: cwnd(t + 1) := cwnd(t) * beta. */
  const float m_beta {0.875};

  /* Multiplicative increase factor: cwnd(t + 1) := cwnd(t) * (1 + xi). */
  const float m_xi {0.0625};

  /* Load factor estimation interval in ms. */
  const int64_t m_estInterval {200};

  /* MI, AI, and MD algorithms. */
  void MultiplicativeIncrease(Ptr<TcpSocketState> tcb);
  void AdditiveIncrease(Ptr<TcpSocketState> tcb);
  void MultiplicativeDecrease(Ptr<TcpSocketState> tcb);

  /* Scaled MI and AI params based on flow-specific RTT. */
  inline float GetScaledXi(int64_t rtt) {
    return pow(1 + m_xi, rtt / m_estInterval) - 1;
  }

  inline float GetScaledAlpha(int64_t rtt) {
    return m_alpha * (rtt / m_estInterval) * (rtt / m_estInterval);
  }

  /* The load state of the connection. */
  LoadState_t m_loadState {LOAD_LOW};

  /* The last recorded RTT for the connection. */
  int64_t m_lastRtt {m_estInterval};

  /* Timer to freeze cwnd after decreasing. */
  Timer m_mdTimer;
  bool m_mdFreeze {false};

  uint32_t m_cWndCnt {0};


};

} // namespace ns3
