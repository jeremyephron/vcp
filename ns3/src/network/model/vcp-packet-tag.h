/*
 * vcp-packet-tag.h
 */

#pragma once

#include "ns3/tag.h"
#include "ns3/ipv4-header.h"

namespace ns3 {

class VcpPacketTag : public Tag
{
public:
  VcpPacketTag ();

  LoadType GetLoad() const;
  void SetLoad(LoadType load);

  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (TagBuffer i) const;
  void Deserialize (TagBuffer i);
  void Print (std::ostream &os) const;

private:
  enum LoadType {
    LOAD_NOT_SUPPORTED = 0,
    LOAD_LOW,
    LOAD_HIGH,
    LOAD_OVERLOAD
  };

  LoadType m_load {LOAD_NOT_SUPPORTED};

};

} // namespace ns3
