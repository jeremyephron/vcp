/*
 * vcp.h
 * 
 * Authors: Jeremy Barenholtz <jeremye@cs.stanford.edu>
 *          Mathew Hogan <mhogan1@stanford.edu>
 */

#pragma once

#include <cmath>

#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/timer.h"

namespace ns3 {
  
class Vcp : public TcpCongestionOps {
public:
  static TypeId GetTypeId();

  Vcp();

  Vcp(const Vcp& sock);

  ~Vcp();

  std::string GetName() const;

  void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time &rtt) override;

  void CongControl(
    Ptr<TcpSocketState> tcb,
    const TcpRateOps::TcpRateConnection &rc,
    const TcpRateOps::TcpRateSample &rs);

  bool HasCongControl() const;

  void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
  Ptr<TcpCongestionOps> Fork() override;
  uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
  void CwndEvent (Ptr<TcpSocketState> tcb, const TcpSocketState::TcpCAEvent_t event);

private:
  /* Load state of the current connection. */
  typedef enum {
    LOAD_NOT_SUPPORTED = 0x0,
    LOAD_LOW           = 0x1,
    LOAD_HIGH          = 0x2,
    LOAD_OVERLOAD      = 0x3,
  } LoadState_t;

  /* Multiplicative increase factor: cwnd(t + 1) := cwnd(t) * (1 + xi). */
  double m_xi {0.0625};

  /* Additive increase factor: cwnd(t + 1) := cwnd(t) + alpha. */
  double m_alpha {1.0};

  /* Multiplicative decrease factor: cwnd(t + 1) := cwnd(t) * beta. */
  double m_beta {0.875};

  double m_xiBound {1.0};

  /* Load factor estimation interval in ns. */
  Time m_estInterval {MilliSeconds(200)};

  /* MI, AI, and MD algorithms. */
  void MultiplicativeIncrease(Ptr<TcpSocketState> tcb);
  void AdditiveIncrease(Ptr<TcpSocketState> tcb);
  void MultiplicativeDecrease(Ptr<TcpSocketState> tcb);

  /* Timer functions */
  void Noop();
  void EndMdFreezePeriod();
  void StorePrevCwnd();

  /* Scaled MI and AI params based on flow-specific RTT. */
  inline double GetScaledXi(int64_t rtt) {
    return std::min(
      pow(1 + m_xi, static_cast<double>(rtt) / m_estInterval.GetMilliSeconds()) - 1,
      m_xiBound
    );
  }

  inline double GetScaledAlpha(int64_t rtt) {
    return (m_alpha * (static_cast<double>(rtt) / m_estInterval.GetMilliSeconds()) 
                    * (static_cast<double>(rtt) / m_estInterval.GetMilliSeconds()));
  }

  /* The load state of the connection. */
  LoadState_t m_loadState {LOAD_LOW};

  /* The last recorded RTT for the connection. */
  int64_t m_lastRtt {0};

  /* Timer to freeze cwnd after decreasing. */
  Timer m_mdTimer;
  bool m_mdFreeze {false};

  Timer m_cWndIncreaseTimer;
  uint32_t m_prevCWnd;
  double m_maxCWndIncreasePerRtt {1.0625};

  /* Fractional cwnd. */
  double m_cWndFractional;
  bool m_cWndFractionalInit {false};

  uint32_t m_segSize {948};

};

} // namespace ns3
