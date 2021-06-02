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


private:
  /* Load state of the current connection. */
  typedef enum {
    LOAD_NOT_SUPPORTED = 0x0,
    LOAD_LOW           = 0x1,
    LOAD_HIGH          = 0x2,
    LOAD_OVERLOAD      = 0x3,
  } LoadState_t;

  const int64_t kDefaultEstInterval {200000000}; // ns
  const double kDefaultAlpha {1.0};
  const double kDefaultXi {0.0625};
  const double kDefaultBeta {0.875};
  const double kDefaultXiBound {1.0};

  /* Multiplicative increase factor: cwnd(t + 1) := cwnd(t) * (1 + xi). */
  double m_xi {kDefaultXi};

  /* Additive increase factor: cwnd(t + 1) := cwnd(t) + alpha. */
  double m_alpha {kDefaultAlpha};

  /* Multiplicative decrease factor: cwnd(t + 1) := cwnd(t) * beta. */
  double m_beta {kDefaultBeta};

  double m_xiBound {kDefaultXiBound};

  /* Load factor estimation interval in ns. */
  Time m_estInterval {Time(kEstIntervalDefault)};

  /* MI, AI, and MD algorithms. */
  void MultiplicativeIncrease(Ptr<TcpSocketState> tcb);
  void AdditiveIncrease(Ptr<TcpSocketState> tcb);
  void MultiplicativeDecrease(Ptr<TcpSocketState> tcb);

  /* Timer functions */
  void Noop();
  void EndMdFreezePeriod();

  /* Scaled MI and AI params based on flow-specific RTT. */
  inline double GetScaledXi(int64_t rtt) {
    return std::min(
      pow(1 + m_xi, static_cast<double>(rtt) / m_estInterval) - 1,
      m_xiBound
    );
  }

  inline double GetScaledAlpha(int64_t rtt) {
    return (m_alpha * (static_cast<double>(rtt) / m_estInterval) 
                    * (static_cast<double>(rtt) / m_estInterval));
  }

  /* The load state of the connection. */
  LoadState_t m_loadState {LOAD_LOW};

  /* The last recorded RTT for the connection. */
  int64_t m_lastRtt {m_estInterval};

  /* Timer to freeze cwnd after decreasing. */
  Timer m_mdTimer;
  bool m_mdFreeze {false};

  /* Fractional cwnd. */
  double m_cWndFractional;
  bool m_cWndFractionalInit {false};

};

} // namespace ns3
