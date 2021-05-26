#ifndef VCP_QUEUE_DISC_H
#define VCP_QUEUE_DISC_H

#include "ns3/fifo-queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/data-rate.h"
#include "ns3/timer.h"

#include <queue>

namespace ns3 {

/**
 * \ingroup traffic-control
 *
 * \brief TODO
 */
class VcpQueueDisc : public FifoQueueDisc
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief VcpQueueDisc Constructor
   *
   * Create a VCP queue disc
   */
  VcpQueueDisc ();

  /**
   * \brief Destructor
   *
   * Destructor
   */ 
  virtual ~VcpQueueDisc ();

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);

  /**
   * \brief Initialize the queue parameters.
   *
   * Note: if the link bandwidth changes in the course of the
   * simulation, the bandwidth-dependent RED parameters do not change.
   * This should be fixed, but it would require some extra parameters,
   * and didn't seem worth the trouble...
   */
  virtual void InitializeParams (void);

  void SampleQueueSize();

  // ** Variables supplied by user
  DataRate m_linkBandwidth; //!< Link bandwidth
  Time m_timeInterval;      //!< time interval throughout which to sample load factor vars
  Time m_QueueSampleInterval; //!< Interval at which to sample queue size
  double m_kq;              //!< K_q constant used while calculating VCP load factor
  double m_target_util;     //!< Target utilization of link capacity (set close to 1)

  // ** Variables maintained by RED
  uint32_t m_qsizes_sum;            //!< Sum of queue sizes queue
  std::queue<uint32_t> recent_queue_sizes; //!< recent samples of queue size
  std::queue<Time> recent_packet_arrivals; //!< times of recent packet arrivals 
  Timer m_queue_size_sample_timer;

};

}; // namespace ns3

#endif // VCP_QUEUE_DISC_H
