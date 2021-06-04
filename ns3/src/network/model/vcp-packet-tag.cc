/*
 * vcp-packet-tag.cc
 */

#include "vcp-packet-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("VcpPacketTag");

VcpPacketTag::VcpPacketTag()
{
  NS_LOG_FUNCTION(this);
}

VcpPacketTag::LoadType
VcpPacketTag::GetLoad() const
{
  NS_LOG_FUNCTION(this);
  return m_load;
}

void
VcpPacketTag::SetLoad(LoadType load)
{
  NS_LOG_FUNCTION(this << load);
  m_load = load;
}

TypeId
VcpPacketTag::GetTypeId()
{
  static TypeId tid = TypeId("ns3::VcpPacketTag")
    .SetParent<Tag>()
    .SetGroupName("Network")
    .AddConstructor<VcpPacketTag>()
  ;
  return tid;
}

TypeId
VcpPacketTag::GetInstanceTypeId() const
{
  NS_LOG_FUNCTION(this);
  return GetTypeId();
}

uint32_t
VcpPacketTag::GetSerializedSize() const
{
  NS_LOG_FUNCTION(this);
  return sizeof(uint8_t);
}

void
VcpPacketTag::Serialize(TagBuffer i) const
{
  NS_LOG_FUNCTION(this << &i);
  i.WriteU8((uint8_t)m_load);
}

void
VcpPacketTag::Deserialize(TagBuffer i)
{
  NS_LOG_FUNCTION(this << &i);
  m_load = (LoadType)i.ReadU8();
}

void
VcpPacketTag::Print(std::ostream &os) const
{
  NS_LOG_FUNCTION(this << &os);
  os << "VcpPacketTag [Load: " << m_load;
  os << "] ";
}

} // namespace ns3
