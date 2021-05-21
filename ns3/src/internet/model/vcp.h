/*
 * vcp.h
 * 
 * Authors: Jeremy Barenholtz <jeremye@cs.stanford.edu>
 *          Mathew Hogan <mhogan1>
 */

#pragma once

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

private:
  uint32_t SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
  void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  uint32_t m_cWndCnt {0};

};

} // namespace ns3
